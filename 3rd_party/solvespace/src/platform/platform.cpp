//-----------------------------------------------------------------------------
// Platform-dependent functionality.
//
// Copyright 2017 whitequark
//-----------------------------------------------------------------------------
#if defined(__APPLE__)
// Include Apple headers before solvespace.h to avoid identifier clashes.
#   include <CoreFoundation/CFString.h>
#   include <CoreFoundation/CFURL.h>
#   include <CoreFoundation/CFBundle.h>
#endif
#include "solvespace.h"
#include "config.h"
#if defined(WIN32)
// Conversely, include Microsoft headers after solvespace.h to avoid clashes.
#   include <windows.h>
#   include <shellapi.h>
#else
#   include <unistd.h>
#   include <sys/stat.h>
#endif
#include <list>
#include <iostream>

namespace SolveSpace {
namespace Platform {

//-----------------------------------------------------------------------------
// Debug output, on Windows.
//-----------------------------------------------------------------------------

#if defined(WIN32)

void DebugPrint(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int len = _vscprintf(fmt, va) + 1;
    va_end(va);

    va_start(va, fmt);
    char *buf = (char *)_alloca(len);
    _vsnprintf(buf, len, fmt, va);
    va_end(va);

    // The native version of OutputDebugString, unlike most others,
    // is OutputDebugStringA.
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");

#ifndef NDEBUG
    // Duplicate to stderr in debug builds, but not in release; this is slow.
    fputs(buf, stderr);
    fputc('\n', stderr);
#endif
}

#endif

//-----------------------------------------------------------------------------
// Debug output, on *nix.
//-----------------------------------------------------------------------------

#if !defined(WIN32)

void DebugPrint(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);
    va_end(va);
}

#endif

//-----------------------------------------------------------------------------
// Temporary arena.
//-----------------------------------------------------------------------------

struct Chunk {
    Chunk()
    {
        data.resize(4096, 0);
        ptr = data.data();
        endptr = ptr + data.size();
    }
    std::vector<uint8_t> data;
    uint8_t *ptr = nullptr;
    uint8_t *endptr = nullptr;
};

static std::list<Chunk> chunks;
static Chunk *last_chunk = nullptr;

void *AllocTemporary(size_t size)
{
    if(!last_chunk) {
        last_chunk = &chunks.emplace_back();
    }
    else {
        if(last_chunk->ptr + size > last_chunk->endptr) {
            last_chunk = &chunks.emplace_back();
        }
    }
    auto p = (void*)last_chunk->ptr;
    size += 0x10-(size%0x10); // 16 byte alignment
    last_chunk->ptr += size;
    return p;
}

void FreeAllTemporary()
{
    size_t total_size = 0;
    for(auto &chunk:chunks) {
        total_size += chunk.data.size();
    }
    while(chunks.size() > 1) {
        chunks.pop_back();
    }
    if(chunks.size() == 0)
        return;
    auto &chunk = chunks.front();
    chunk.data.resize(total_size);
    memset(chunk.data.data(), 0, total_size);
    chunk.ptr = chunk.data.data();
    chunk.endptr = chunk.ptr + chunk.data.size();
    last_chunk = &chunk;
}

}
}

