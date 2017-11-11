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

#include "winlean.h"
#include "common.h"
#include "ThreadPool.h"

namespace Radyx {

ThreadPool::Thread::Thread()
	: work_available(false),
	exit(false),
	argp(nullptr),
	argi(0)
{
	thread = std::thread(&Thread::ThreadFn, this);
#ifdef _WIN32
	SetThreadPriority(thread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
}

ThreadPool::Thread::~Thread()
{
	exit = true;
	cv.notify_all();
	thread.join();
}

void ThreadPool::Thread::ThreadFn()
{
	std::unique_lock<std::mutex> lock(mutex);
	for (;;)
	{
		work_available = false;
		while (!work_available && !exit) {
			cv.wait(lock);
		}
		if (exit) {
			break;
		}
		work_fn(argp, argi);
	}
}

void ThreadPool::Thread::SetWork(std::function<void(void*, int)> fn, void *argp_, int argi_)
{
	std::unique_lock<std::mutex> lock(mutex);
	work_fn = fn;
	argp = argp_;
	argi = argi_;
	work_available = true;
	cv.notify_all();
}

void ThreadPool::Thread::Join()
{
	do {
		std::unique_lock<std::mutex> lock(mutex);
		if (work_available) {
			std::this_thread::yield();
		}
	}
	while (work_available);
}

}