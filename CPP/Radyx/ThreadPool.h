///////////////////////////////////////////////////////////////////////////////
//
// Class: ThreadPool
//        Simple thread pool to eliminate extra thread creation overhead
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

#ifndef RADYX_THREAD_POOL_H
#define RADYX_THREAD_POOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace Radyx {

class ThreadPool
{
public:
	class Thread
	{
	public:
		Thread();
		~Thread();
		void SetWork(std::function<void(void*, int)> fn, void *argp, int argi);
		void Join();

	private:
		void ThreadFn();

		std::thread thread;
		std::mutex mutex;
		std::condition_variable cv;
		volatile bool work_available;
		volatile bool exit;
		std::function<void(void*, int)> work_fn;
		void* argp;
		int argi;

		Thread(const Thread&) = delete;
		Thread& operator=(const Thread&) = delete;
		Thread(Thread&&) = delete;
		Thread& operator=(Thread&&) = delete;
	};

	ThreadPool(unsigned thread_count_)
		: threads(new Thread[thread_count_]), thread_count(thread_count_) {}
	Thread& operator[](size_t index) {
		return threads.get()[index];
	}
	unsigned GetCount() const noexcept {
		return thread_count;
	}

private:
	std::unique_ptr<Thread[]> threads;
	unsigned thread_count;

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;
};

}

#endif // RADYX_THREAD_POOL_H