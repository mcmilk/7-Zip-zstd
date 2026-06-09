/*
Copyright 2011-2026 Frederic Langlet
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
you may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <algorithm>
#include <sstream>
#include <stdio.h>
#include "CompressedOutputStream.hpp"
#include "IOException.hpp"
#include "../Error.hpp"
#include "../Magic.hpp"
#include "../entropy/EntropyEncoderFactory.hpp"
#include "../entropy/EntropyUtils.hpp"
#include "../transform/TransformFactory.hpp"
#include "../util/fixedbuf.hpp"

using namespace kanzi;
using namespace std;


const int CompressedOutputStream::BITSTREAM_TYPE = 0x4B414E5A; // "KANZ"
const int CompressedOutputStream::BITSTREAM_FORMAT_VERSION = 6;
const int CompressedOutputStream::DEFAULT_BUFFER_SIZE = 256 * 1024;
const kanzi::byte CompressedOutputStream::COPY_BLOCK_MASK = kanzi::byte(0x80);
const kanzi::byte CompressedOutputStream::TRANSFORMS_MASK = kanzi::byte(0x10);
const int CompressedOutputStream::MIN_BITSTREAM_BLOCK_SIZE = 1024;
const int CompressedOutputStream::MAX_BITSTREAM_BLOCK_SIZE = 1024 * 1024 * 1024;
const int CompressedOutputStream::SMALL_BLOCK_SIZE = 15;
const int CompressedOutputStream::CANCEL_TASKS_ID = -1;
const int CompressedOutputStream::MAX_CONCURRENCY = 64;


CompressedOutputStream::CompressedOutputStream(OutputStream& os,
                   int tasks,
                   const string& entropy,
                   const string& transform,
                   int blockSize,
                   int checksum,
                   uint64 fileSize,
#ifdef CONCURRENCY_ENABLED
                   ThreadPool* pool,
#endif
                   bool headerless)
    : OutputStream(os.rdbuf())
{
#ifdef CONCURRENCY_ENABLED
    if ((tasks <= 0) || (tasks > MAX_CONCURRENCY)) {
        stringstream ss;
        ss << "The number of jobs must be in [1.." << MAX_CONCURRENCY << "], got " << tasks;
        throw invalid_argument(ss.str());
    }

    _pool = pool; // can be null
#else
    if (tasks != 1)
        throw invalid_argument("The number of jobs is limited to 1 in this version");
#endif

    if (blockSize > MAX_BITSTREAM_BLOCK_SIZE) {
        std::stringstream ss;
        ss << "The block size must be at most " << (MAX_BITSTREAM_BLOCK_SIZE >> 20) << " MB";
        throw invalid_argument(ss.str());
    }

    if (blockSize < MIN_BITSTREAM_BLOCK_SIZE) {
        std::stringstream ss;
        ss << "The block size must be at least " << MIN_BITSTREAM_BLOCK_SIZE;
        throw invalid_argument(ss.str());
    }

    if ((blockSize & -16) != blockSize)
        throw invalid_argument("The block size must be a multiple of 16");

    _blockId = 0;
    _inputBlockId = 0;
    _bufferId = 0;
    _blockSize = blockSize;
    _bufferThreshold = blockSize;
    _inputSize = fileSize;
    const int nbBlocks = (_inputSize == 0) ? 0 : int((_inputSize + int64(blockSize - 1)) / int64(blockSize));
    _nbInputBlocks = min(nbBlocks, MAX_CONCURRENCY - 1);
    _headless = headerless;
    _initialized = 0;
    _closed = 0;
    _obs = new DefaultOutputBitStream(os, DEFAULT_BUFFER_SIZE);
    _entropyType = EntropyEncoderFactory::getType(entropy.c_str());
    _transformType = TransformFactory<kanzi::byte>::getType(transform.c_str());

    if (checksum == 0) {
       _hasher32 = nullptr;
       _hasher64 = nullptr;
    }
    else if (checksum == 32) {
       _hasher32 = new XXHash32(BITSTREAM_TYPE);
       _hasher64 = nullptr;
    }
    else if (checksum == 64) {
       _hasher32 = nullptr;
       _hasher64 = new XXHash64(BITSTREAM_TYPE);
    }
    else {
        throw invalid_argument("The block checksum size must be 0, 32 or 64");
    }

    _jobs = tasks;
    _ctx.putInt("blockSize", _blockSize);
    _ctx.putInt("checksum", checksum);
    _ctx.putString("entropy", entropy);
    _ctx.putString("transform", transform);
    _ctx.putInt("bsVersion", BITSTREAM_FORMAT_VERSION);

#ifdef CONCURRENCY_ENABLED
    _futures.resize(_jobs);
#endif

    _jobsPerTask.resize(_jobs);

    // Assign optimal number of tasks and jobs per task (if the number of blocks is available)
    if (_jobs > 1) {
        // Limit the number of tasks if there are fewer blocks that _jobs
        // It allows more jobs per task and reduces memory usage.
        int nbTasks = (_nbInputBlocks != 0) ? min(_nbInputBlocks, _jobs) : _jobs;
        Global::computeJobsPerTask(&_jobsPerTask[0], _jobs, nbTasks);
    }
    else {
        _jobsPerTask[0] = 1;
    }

    // Allocate first buffer and add padding for incompressible blocks
    _buffers = new SliceArray<kanzi::byte>*[2 * _jobs];
    const int bufSize = max(_blockSize + (_blockSize >> 3), DEFAULT_BUFFER_SIZE);
    _buffers[0] = new SliceArray<kanzi::byte>(new kanzi::byte[bufSize], bufSize, 0);

    for (int i = 1; i < 2 * _jobs; i++)
       _buffers[i] = new SliceArray<kanzi::byte>(nullptr, 0, 0);
}

CompressedOutputStream::CompressedOutputStream(OutputStream& os, Context& ctx, bool headerless)
    : OutputStream(os.rdbuf())
    , _ctx(ctx)
{
    int tasks = ctx.getInt("jobs", 1);

#ifdef CONCURRENCY_ENABLED
    if ((tasks <= 0) || (tasks > MAX_CONCURRENCY)) {
        stringstream ss;
        ss << "The number of jobs must be in [1.." << MAX_CONCURRENCY << "], got " << tasks;
        throw invalid_argument(ss.str());
    }

    _pool = _ctx.getPool(); // can be null
#else
    if (tasks != 1)
        throw invalid_argument("The number of jobs is limited to 1 in this version");
#endif

    int blockSize = ctx.getInt("blockSize");

    if (blockSize > MAX_BITSTREAM_BLOCK_SIZE) {
        std::stringstream ss;
        ss << "The block size must be at most " << (MAX_BITSTREAM_BLOCK_SIZE >> 20) << " MB";
        throw invalid_argument(ss.str());
    }

    if (blockSize < MIN_BITSTREAM_BLOCK_SIZE) {
        std::stringstream ss;
        ss << "The block size must be at least " << MIN_BITSTREAM_BLOCK_SIZE;
        throw invalid_argument(ss.str());
    }

    if ((blockSize & -16) != blockSize)
        throw invalid_argument("The block size must be a multiple of 16");

    _inputSize = ctx.getLong("fileSize", 0);
    const int nbBlocks = (_inputSize == 0) ? 0 : int((_inputSize + int64(blockSize - 1)) / int64(blockSize));
    _nbInputBlocks = min(nbBlocks, MAX_CONCURRENCY - 1);
    _jobs = tasks;
    _blockId = 0;
    _inputBlockId = 0;
    _bufferId = 0;
    _blockSize = blockSize;
    _bufferThreshold = blockSize;
    _initialized = 0;
    _closed = 0;
    _headless = headerless;
    _obs = new DefaultOutputBitStream(os, DEFAULT_BUFFER_SIZE);
    _ctx.putInt("bsVersion", BITSTREAM_FORMAT_VERSION);
    string entropyCodec = ctx.getString("entropy");
    string transform = ctx.getString("transform");
    _entropyType = EntropyEncoderFactory::getType(entropyCodec.c_str());
    _transformType = TransformFactory<kanzi::byte>::getType(transform.c_str());
    int checksum = ctx.getInt("checksum", 0);

    if (checksum == 0) {
       _hasher32 = nullptr;
       _hasher64 = nullptr;
    }
    else if (checksum == 32) {
       _hasher32 = new XXHash32(BITSTREAM_TYPE);
       _hasher64 = nullptr;
    }
    else if (checksum == 64) {
       _hasher32 = nullptr;
       _hasher64 = new XXHash64(BITSTREAM_TYPE);
    }
    else {
        throw invalid_argument("The block checksum size must be 0, 32 or 64");
    }

#ifdef CONCURRENCY_ENABLED
    _futures.resize(_jobs);
#endif

    _jobsPerTask.resize(_jobs);

    // Assign optimal number of tasks and jobs per task (if the number of blocks is available)
    if (_jobs > 1) {
        // Limit the number of tasks if there are fewer blocks that _jobs
        // It allows more jobs per task and reduces memory usage.
        int nbTasks = (_nbInputBlocks != 0) ? min(_nbInputBlocks, _jobs) : _jobs;
        Global::computeJobsPerTask(&_jobsPerTask[0], _jobs, nbTasks);
    }
    else {
        _jobsPerTask[0] = 1;
    }

    _buffers = new SliceArray<kanzi::byte>*[2 * _jobs];

    // Allocate first buffer and add padding for incompressible blocks
    const int bufSize = max(_blockSize + (_blockSize >> 3), DEFAULT_BUFFER_SIZE);
    _buffers[0] = new SliceArray<kanzi::byte>(new kanzi::byte[bufSize], bufSize, 0);

    for (int i = 1; i < 2 * _jobs; i++)
       _buffers[i] = new SliceArray<kanzi::byte>(nullptr, 0, 0);
}

CompressedOutputStream::~CompressedOutputStream()
{
    try {
        close();
    }
    catch (const exception&) {
        // Ignore and continue
    }

    for (int i = 0; i < 2 * _jobs; i++) {
        if (_buffers[i]->_array != nullptr)
           delete[] _buffers[i]->_array;

        delete _buffers[i];
    }

    delete[] _buffers;
    delete _obs;

    if (_hasher32 != nullptr) {
        delete _hasher32;
        _hasher32 = nullptr;
    }

    if (_hasher64 != nullptr) {
        delete _hasher64;
        _hasher64 = nullptr;
    }
}

void CompressedOutputStream::writeHeader()
{
    if ((_headless == true) || (EXCHANGE_ATOMIC(_initialized, 1) == 1))
        return;

    if (_obs->writeBits(BITSTREAM_TYPE, 32) != 32)
        throw IOException("Cannot write bitstream type to header", Error::ERR_WRITE_FILE);

    if (_obs->writeBits(BITSTREAM_FORMAT_VERSION, 4) != 4)
        throw IOException("Cannot write bitstream version to header", Error::ERR_WRITE_FILE);

    uint ckSize = 0;

    if (_hasher32 != nullptr)
        ckSize = 1;
    else if (_hasher64 != nullptr)
        ckSize = 2;

    if (_obs->writeBits(ckSize, 2) != 2)
        throw IOException("Cannot write block checksum size to header", Error::ERR_WRITE_FILE);

    if (_obs->writeBits(_entropyType, 5) != 5)
        throw IOException("Cannot write entropy type to header", Error::ERR_WRITE_FILE);

    if (_obs->writeBits(_transformType, 48) != 48)
        throw IOException("Cannot write transform types to header", Error::ERR_WRITE_FILE);

    if (_obs->writeBits(_blockSize >> 4, 28) != 28)
        throw IOException("Cannot write block size to header", Error::ERR_WRITE_FILE);

    // _inputSize not provided or >= 2^48 -> 0, <2^16 -> 1, <2^32 -> 2, <2^48 -> 3
    const uint szMask = ((_inputSize == 0) || (_inputSize >= (int64(1) << 48))) ? 0
        : (Global::log2(uint64(_inputSize)) >> 4) + 1;

    if (_obs->writeBits(szMask, 2) != 2)
        throw IOException("Cannot write size of input to header", Error::ERR_WRITE_FILE);

    if (szMask != 0) {
        if (_obs->writeBits(_inputSize, 16 * szMask) != 16 * szMask)
            throw IOException("Cannot write size of input to header", Error::ERR_WRITE_FILE);
    }

    const uint64 padding = 0;

    if (_obs->writeBits(padding, 15) != 15)
        throw IOException("Cannot write padding to header", Error::ERR_WRITE_FILE);

    uint32 seed = 0x01030507 * BITSTREAM_FORMAT_VERSION; // no const to avoid VS2008 warning
    const uint32 HASH = 0x1E35A7BD;
    uint32 cksum = HASH * seed;
    cksum ^= (HASH * uint32(~ckSize));
    cksum ^= (HASH * uint32(~_entropyType));
    cksum ^= (HASH * uint32((~_transformType) >> 32));
    cksum ^= (HASH * uint32(~_transformType));
    cksum ^= (HASH * uint32(~_blockSize));

    if (szMask != 0) {
        cksum ^= (HASH * uint32((~_inputSize) >> 32));
        cksum ^= (HASH * uint32(~_inputSize));
    }

    cksum = (cksum >> 23) ^ (cksum >> 3);

    if (_obs->writeBits(cksum, 24) != 24)
        throw IOException("Cannot write checksum to header", Error::ERR_WRITE_FILE);
}

bool CompressedOutputStream::addListener(Listener<Event>& bl)
{
    _listeners.push_back(&bl);
    return true;
}

bool CompressedOutputStream::removeListener(Listener<Event>& bl)
{
    std::vector<Listener<Event>*>::iterator it = find(_listeners.begin(), _listeners.end(), &bl);

    if (it == _listeners.end())
        return false;

    _listeners.erase(it);
    return true;
}

ostream& CompressedOutputStream::write(const char* data, streamsize length)
{
    if (length < 0)
       throw IOException("Invalid buffer size");

    streamsize off = 0;
    streamsize remaining = length;

    while (remaining > 0) {
        const streamsize lenChunk = min(remaining, streamsize(_bufferThreshold - _buffers[_bufferId]->_index));

        if (lenChunk > 0) {
            memcpy(&_buffers[_bufferId]->_array[_buffers[_bufferId]->_index], &data[off], size_t(lenChunk));
            _buffers[_bufferId]->_index += int(lenChunk);
            off += lenChunk;
            remaining -= lenChunk;

            if (_buffers[_bufferId]->_index >= _bufferThreshold) {
                processBuffer();
            }
        }
        else {
            // Handle full buffer / closed stream
            processBuffer();
        }
    }

    return *this;
}


void CompressedOutputStream::close()
{
    if (LOAD_ATOMIC(_closed) == 1)
        return;

    string errMsg;

    try {
        // Submit the last partial block (if any)
        submitBlock();

#ifdef CONCURRENCY_ENABLED
        // Wait for ALL pending tasks to complete
        for (int i = 0; i < _jobs; i++) {
            if (_futures[i].valid()) {
                EncodingTaskResult res = _futures[i].get();

                if (res._error != 0)
                    throw IOException(res._msg, res._error);
            }
        }
#endif

        // Write last block: length-3 (0) and 0 bits
        _obs->writeBits(uint64(0), 5);
        _obs->writeBits(uint64(0), 3);
        _obs->close();
    }
    catch (const exception& e) {
        setstate(ios::badbit);
        errMsg = e.what();
    }

    STORE_ATOMIC(_closed, 1);

    // Force subsequent writes to trigger submitBlock immediately
    _bufferThreshold = 0;

    // Release resources
    for (int i = 0; i < 2 * _jobs; i++) {
        if (_buffers[i]->_array != nullptr)
           delete[] _buffers[i]->_array;

        _buffers[i]->_array = nullptr;
        _buffers[i]->_length = 0;
        _buffers[i]->_index = 0;
    }

    if (errMsg != "")
       throw IOException(errMsg, Error::ERR_WRITE_FILE);

    setstate(ios::eofbit);
}


void CompressedOutputStream::processBuffer()
{
    submitBlock();
    _bufferId = (_bufferId + 1) % _jobs;

#ifdef CONCURRENCY_ENABLED
    if (_futures[_bufferId].valid()) {
        EncodingTaskResult res = _futures[_bufferId].get();

        if (res._error != 0)
            throw IOException(res._msg, res._error);
    }
#endif

    const int bSize = _blockSize + (_blockSize >> 6);
    const int bufSize = max(bSize, 65536);

    if (_buffers[_bufferId]->_length == 0) {
        if (_buffers[_bufferId]->_array != nullptr)
            delete[] _buffers[_bufferId]->_array;

        _buffers[_bufferId]->_array = new kanzi::byte[bufSize];
        _buffers[_bufferId]->_length = bufSize;
    }

    _buffers[_bufferId]->_index = 0;
}


void CompressedOutputStream::submitBlock()
{
    if (LOAD_ATOMIC(_closed) == 1)
        throw IOException("Stream closed", Error::ERR_WRITE_FILE);

    writeHeader(); // Ensure header is written before first block processing

    const int dataLength = _buffers[_bufferId]->_index;

    if (dataLength == 0)
        return;

    // Increment input block counter (1-based for the Task logic)
    _inputBlockId++;

    Context copyCtx(_ctx);
    copyCtx.putLong("tType", _transformType);
    copyCtx.putInt("eType", _entropyType);
    copyCtx.putInt("blockId", _inputBlockId);
    copyCtx.putInt("size", dataLength);
    copyCtx.putInt("jobs", _jobsPerTask[_bufferId]);

    // Prepare the buffer for processing
    _buffers[_bufferId]->_index = 0;

    // Create the task
    // Note: Input is _buffers[_bufferId], Output is _buffers[_jobs + _bufferId]
    EncodingTask<EncodingTaskResult>* task = new EncodingTask<EncodingTaskResult>(
        _buffers[_bufferId],
        _buffers[_jobs + _bufferId],
        _obs, _hasher32, _hasher64,
#ifdef CONCURRENCY_ENABLED
        &_blockMutex, &_blockCondition,
#endif
        &_blockId, _listeners, copyCtx);

#ifdef CONCURRENCY_ENABLED
    std::shared_ptr<EncodingTask<EncodingTaskResult>> safeTask(task);

    auto taskWrapper = [safeTask]() {
        return safeTask->run();
    };

    if (_pool == nullptr) {
        _futures[_bufferId] = std::async(std::launch::async, taskWrapper);
    }
    else {
        // REQUIRES: Pool size > Number of concurrent tasks to avoid deadlock
        _futures[_bufferId] = _pool->schedule(taskWrapper);
    }
#else
    // Synchronous fallback
    try {
        EncodingTaskResult res = task->run();

        if (res._error != 0)
           throw IOException(res._msg, res._error);
    } catch (...) {
        delete task;
        throw;
    }

    delete task;
#endif
}

ostream& CompressedOutputStream::put(char c)
{
    try {
        if (_buffers[_bufferId]->_index >= _bufferThreshold) {
            // Submit current buffer
            submitBlock();

            // Rotate to next buffer
            _bufferId = (_bufferId + 1) % _jobs;

            // If concurrent, wait if the target buffer is still busy
#ifdef CONCURRENCY_ENABLED
            if (_futures[_bufferId].valid()) {
                EncodingTaskResult res = _futures[_bufferId].get();

                if (res._error != 0)
                    throw IOException(res._msg, res._error);
            }
#endif

            // Allocation / Reset logic
            const int bufSize = max(_blockSize + (_blockSize >> 6), 65536);

            if (_buffers[_bufferId]->_length == 0) {
                 if (_buffers[_bufferId]->_array != nullptr)
                     delete[] _buffers[_bufferId]->_array;

                _buffers[_bufferId]->_array = new kanzi::byte[bufSize];
                _buffers[_bufferId]->_length = bufSize;
            }

            _buffers[_bufferId]->_index = 0;
        }

        _buffers[_bufferId]->_array[_buffers[_bufferId]->_index++] = kanzi::byte(c);
        return *this;
    }
    catch (const exception& e) {
        setstate(std::ios::badbit);
        throw std::ios_base::failure(e.what());
    }
}

void CompressedOutputStream::notifyListeners(vector<Listener<Event>*>& listeners, const Event& evt)
{
    for (vector<Listener<Event>*>::iterator it = listeners.begin(); it != listeners.end(); ++it)
        (*it)->processEvent(evt);
}

template <class T>
EncodingTask<T>::EncodingTask(SliceArray<kanzi::byte>* iBuffer, SliceArray<kanzi::byte>* oBuffer,
    DefaultOutputBitStream* obs, XXHash32* hasher32, XXHash64* hasher64,
#ifdef CONCURRENCY_ENABLED
    std::mutex* blockMutex, std::condition_variable* blockCondition,
#endif
    atomic_int_t* processedBlockId, vector<Listener<Event>*>& listeners,
    const Context& ctx)
    : _obs(obs)
    , _listeners(listeners)
    , _ctx(ctx)
{
    _data = iBuffer;
    _buffer = oBuffer;
    _hasher32 = hasher32;
    _hasher64 = hasher64;
#ifdef CONCURRENCY_ENABLED
    _blockMutex = blockMutex;
    _blockCondition = blockCondition;
#endif
    _processedBlockId = processedBlockId;
}

template <class T>
void EncodingTask<T>::storeProcessedBlockId(int value)
{
#ifdef CONCURRENCY_ENABLED
    {
        std::lock_guard<std::mutex> lock(*_blockMutex);
        STORE_ATOMIC(*_processedBlockId, value);
    }

    _blockCondition->notify_all();
#else
    STORE_ATOMIC(*_processedBlockId, value);
#endif
}

template <class T>
void EncodingTask<T>::fetchAddProcessedBlockId()
{
#ifdef CONCURRENCY_ENABLED
    {
        std::lock_guard<std::mutex> lock(*_blockMutex);
        FETCH_ADD_ATOMIC(*_processedBlockId, 1);
    }

    _blockCondition->notify_all();
#else
    FETCH_ADD_ATOMIC(*_processedBlockId, 1);
#endif
}

// Encode mode + transformed entropy coded data
// mode | 0b1yy0xxxx => copy block
//      | 0b0yy00000 => size(size(block))-1
//  case 4 transforms or less
//      | 0b0001xxxx => transform sequence skip flags (1 means skip)
//  case more than 4 transforms
//      | 0b0yy00000 0bxxxxxxxx => transform sequence skip flags in next kanzi::byte (1 means skip)
template <class T>
T EncodingTask<T>::run()
{
    const int blockId = _ctx.getInt("blockId");
    const int blockLength = _ctx.getInt("size");
    TransformSequence<kanzi::byte>* transform = nullptr;
    EntropyEncoder* ee = nullptr;

    try {
        if (blockLength == 0) {
            // Last block (only block with 0 length)
            fetchAddProcessedBlockId();
            return T(blockId, 0, "Success");
        }

        kanzi::byte mode = kanzi::byte(0);
        int postTransformLength = blockLength;
        uint64 checksum = 0;
        uint64 tType = _ctx.getLong("tType");
        short eType = short(_ctx.getInt("eType"));
        Event::HashType hashType = Event::NO_HASH;
        WallTimer timer;

        // Compute block checksum
        if (_hasher32 != nullptr) {
            checksum = _hasher32->hash(&_data->_array[_data->_index], blockLength);
            hashType = Event::SIZE_32;
        }
        else if (_hasher64 != nullptr) {
            checksum = _hasher64->hash(&_data->_array[_data->_index], blockLength);
            hashType = Event::SIZE_64;
        }

        if (_listeners.size() > 0) {
            // Notify before transform
            Event evt(Event::BEFORE_TRANSFORM, blockId,
                int64(blockLength), timer.getCurrentTime(), checksum, hashType);
            CompressedOutputStream::notifyListeners(_listeners, evt);
        }

        if (blockLength <= CompressedOutputStream::SMALL_BLOCK_SIZE) {
            tType = TransformFactory<kanzi::byte>::NONE_TYPE;
            eType = EntropyEncoderFactory::NONE_TYPE;
            mode |= CompressedOutputStream::COPY_BLOCK_MASK;
        }
        else {
            int checkSkip = _ctx.getInt("skipBlocks", 0);

            if (checkSkip != 0) {
                bool skip = Magic::isCompressed(Magic::getType(&_data->_array[_data->_index]));

                if (skip == false) {
                    uint histo[256] = { 0 };
                    Global::computeHistogram(&_data->_array[_data->_index], blockLength, histo);
                    const int entropy = Global::computeFirstOrderEntropy1024(blockLength, histo);
                    skip = entropy >= EntropyUtils::INCOMPRESSIBLE_THRESHOLD;
                    //_ctx.putString("histo0", toString(histo, 256));
                }

                if (skip == true) {
                    tType = TransformFactory<kanzi::byte>::NONE_TYPE;
                    eType = EntropyEncoderFactory::NONE_TYPE;
                    mode |= CompressedOutputStream::COPY_BLOCK_MASK;
                }
            }
        }

        _ctx.putInt("size", blockLength);
        transform = TransformFactory<kanzi::byte>::newTransform(_ctx, tType);
        const int requiredSize = transform->getMaxEncodedLength(blockLength);

        if (blockLength >= 4) {
           uint magic = Magic::getType(&_data->_array[_data->_index]);

           if (Magic::isCompressed(magic) == true)
               _ctx.putInt("dataType", Global::BIN);
           else if (Magic::isMultimedia(magic) == true)
               _ctx.putInt("dataType", Global::MULTIMEDIA);
           else if (Magic::isExecutable(magic) == true)
               _ctx.putInt("dataType", Global::EXE);
        }

        if (_buffer->_length < requiredSize) {
            if (_buffer->_array != nullptr)
               delete[] _buffer->_array;

            _buffer->_array = new kanzi::byte[requiredSize];
            _buffer->_length = requiredSize;
        }

        // Forward transform (ignore error, encode skipFlags)
        // _data->_length is at least blockLength
        _buffer->_index = 0;
        transform->forward(*_data, *_buffer, blockLength);
        const int nbTransforms = transform->getNbTransforms();
        const kanzi::byte skipFlags = transform->getSkipFlags();
        delete transform;
        transform = nullptr;
        postTransformLength = _buffer->_index;

        if (postTransformLength < 0) {
            storeProcessedBlockId(CompressedOutputStream::CANCEL_TASKS_ID);
            return T(blockId, Error::ERR_WRITE_FILE, "Invalid transform size");
        }

        _ctx.putInt("size", postTransformLength);
        const int dataSize = (postTransformLength < 256) ? 1 : (Global::_log2(uint32(postTransformLength)) >> 3) + 1;

        if (dataSize > 4) {
            storeProcessedBlockId(CompressedOutputStream::CANCEL_TASKS_ID);
            return T(blockId, Error::ERR_WRITE_FILE, "Invalid block data length");
        }

        // Record size of 'block size' - 1 in bytes
        mode |= kanzi::byte(((dataSize - 1) & 0x03) << 5);

        if (_listeners.size() > 0) {
            // Notify after transform
            Event evt(Event::AFTER_TRANSFORM, blockId,
                int64(postTransformLength), timer.getCurrentTime(), checksum, hashType);
            CompressedOutputStream::notifyListeners(_listeners, evt);
        }

        const int bufSize = max(CompressedOutputStream::DEFAULT_BUFFER_SIZE,
                                max(postTransformLength, blockLength + (blockLength >> 3)));

        if (_data->_length < bufSize) {
            // Rare case where the transform expanded the input or
            // entropy coder may expand size.
            delete[] _data->_array;
            _data->_length = bufSize;
            _data->_array = new kanzi::byte[_data->_length];
        }

        _data->_index = 0;
        ofixedbuf buf(reinterpret_cast<char*>(&_data->_array[_data->_index]), streamsize(_data->_length));
        ostream os(&buf);
        DefaultOutputBitStream obs(os);

        // Write block 'header' (mode + compressed length)
        if (((mode & CompressedOutputStream::COPY_BLOCK_MASK) != kanzi::byte(0)) || (nbTransforms <= 4)) {
            mode |= kanzi::byte(skipFlags >> 4);
            obs.writeBits(uint64(mode), 8);
        }
        else {
            mode |= CompressedOutputStream::TRANSFORMS_MASK;
            obs.writeBits(uint64(mode), 8);
            obs.writeBits(uint64(skipFlags), 8);
        }

        obs.writeBits(postTransformLength, 8 * dataSize);

        // Write checksum
        if (_hasher32 != nullptr)
            obs.writeBits(checksum, 32);
        else if (_hasher64 != nullptr)
            obs.writeBits(checksum, 64);

        if (_listeners.size() > 0) {
            // Notify before entropy
            Event evt(Event::BEFORE_ENTROPY, blockId,
                int64(postTransformLength), timer.getCurrentTime(), checksum, hashType);
            CompressedOutputStream::notifyListeners(_listeners, evt);
        }

        // Each block is encoded separately
        // Rebuild the entropy encoder to reset block statistics
        ee = EntropyEncoderFactory::newEncoder(obs, _ctx, eType);

        // Entropy encode block
        if (ee->encode(_buffer->_array, 0, postTransformLength) != postTransformLength) {
            delete ee;
            storeProcessedBlockId(CompressedOutputStream::CANCEL_TASKS_ID);
            return T(blockId, Error::ERR_PROCESS_BLOCK, "Entropy coding failed");
        }

        // Dispose before processing statistics (may write to the bitstream)
        ee->dispose();
        delete ee;
        ee = nullptr;
        obs.close();
        uint64 written = obs.written();
        const uint lw = (written < 8) ? 3 : uint(Global::log2(uint32(written >> 3)) + 4);

#ifdef CONCURRENCY_ENABLED
        {
            std::unique_lock<std::mutex> lock(*_blockMutex);
            _blockCondition->wait(lock, [this, blockId]() {
                const int taskId = LOAD_ATOMIC(*_processedBlockId);
                return (taskId == CompressedOutputStream::CANCEL_TASKS_ID) || (taskId == blockId - 1);
            });
        }

        if (LOAD_ATOMIC(*_processedBlockId) == CompressedOutputStream::CANCEL_TASKS_ID)
            return T(blockId, 0, "Canceled");
#endif

        // Emit block size in bits (max size pre-entropy is 1 GB = 1 << 30 bytes)
#if !defined(_MSC_VER) || _MSC_VER > 1500
        const int64 blockOffset = _obs->tell();
#endif
        _obs->writeBits(lw - 3, 5); // write length-3 (5 bits max)
        _obs->writeBits(written, lw);
        int64 ww = int64((written + 7) >> 3);

        // Emit data to shared bitstream
        for (uint n = 0; written > 0; ) {
            uint chkSize = uint(min(written, uint64(1) << 30));
            _obs->writeBits(&_data->_array[n], chkSize);
            n += ((chkSize + 7) >> 3);
            written -= uint64(chkSize);
        }

        // After completion of the entropy coding, increment the block id.
        // It unblocks the task processing the next block (if any).
        storeProcessedBlockId(blockId);

        if (_listeners.size() > 0) {
            // Notify after entropy
            Event evt1(Event::AFTER_ENTROPY, blockId, ww, timer.getCurrentTime(), checksum, hashType);
            CompressedOutputStream::notifyListeners(_listeners, evt1);

#if !defined(_MSC_VER) || _MSC_VER > 1500
            if (_ctx.getInt("verbosity", 0) > 4) {
                string oName = _ctx.getString("outputName");

                if (oName.length() == 4) {
                    std::transform(oName.begin(), oName.end(), oName.begin(), ::toupper);
                }

                Event evt2(Event::BLOCK_INFO, blockId,
                   int64((written + 7) >> 3), timer.getCurrentTime(), checksum, hashType, blockOffset, uint8(skipFlags));
                CompressedOutputStream::notifyListeners(_listeners, evt2);
            }
#endif
        }

        return T(blockId, 0, "Success");
    }
    catch (const exception& e) {
        // Cancel any in-flight task waiting on this block.
        storeProcessedBlockId(CompressedOutputStream::CANCEL_TASKS_ID);

        if (transform != nullptr)
            delete transform;

        if (ee != nullptr)
            delete ee;

        return T(blockId, Error::ERR_PROCESS_BLOCK, e.what());
    }
}
