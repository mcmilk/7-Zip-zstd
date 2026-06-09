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

#pragma once
#ifndef knz_fixedbuf
#define knz_fixedbuf

#include <cstddef>
#include <streambuf>


// Ahem ... Visual Studio
// This code is required because Microsoft cannot bother to implement streambuf::pubsetbuf().
// Also On libstdc++, pubsetbuf() silently ignores the supplied buffer and leaves internal pointers null.

class ifixedbuf : public std::streambuf {
public:
    ifixedbuf(char* data, std::size_t size) {
        // Always manually set the read pointers.
        // pubsetbuf() is unreliable on libstdc++, and MSVC doesn't implement it.
        this->setg(data, data, data + size);
    }
};

class ofixedbuf : public std::streambuf {
public:
    ofixedbuf(char* data, std::size_t size) {
        // Always set buffer manually - pubsetbuf is useless on libstdc++
        this->setp(data, data + size);
    }

    std::size_t written() const {
        return this->pptr() - this->pbase();
    }
};

class iofixedbuf : public std::streambuf {
public:
    iofixedbuf(char* data, std::size_t size) {
        this->setg(data, data, data + size);
        this->setp(data, data + size);
    }
};

#endif
