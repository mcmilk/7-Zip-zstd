///////////////////////////////////////////////////////////////////////////////
//
// Class:   AsyncWriter
//          Write output data in a worker thread
//
// Copyright 2015 Conor McCarthy
//
// This file is part of Radyx.
//
// Radyx is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Radyx is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Radyx. If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "AsyncWriter.h"


namespace Radyx {

void AsyncWriter::ThreadFn(void* pwork, int /*unused*/)
{
	AsyncWriter* writer = reinterpret_cast<AsyncWriter*>(pwork);
	if (!writer->out_stream.Fail()) {
		writer->out_stream.Write(reinterpret_cast<char*>(writer->out_buffer),
			writer->buffer_size);
		if (writer->out_stream.Fail()) {
			writer->error.LoadOsErrorCode();
			writer->error.type = ErrorCode::kWrite;
		}
	}
}

void AsyncWriter::Write(uint8_t* out_buffer_, size_t buffer_size_)
{
	thread.Join();
	out_buffer = out_buffer_;
	buffer_size = buffer_size_;
	thread.SetWork(ThreadFn, this, 0);
}

}