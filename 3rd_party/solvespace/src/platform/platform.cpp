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
#include <mimalloc.h>
#include "config.h"
#if defined(WIN32)
// Conversely, include Microsoft headers after solvespace.h to avoid clashes.
#   include <windows.h>
#   include <shellapi.h>
#else
#   include <unistd.h>
#   include <sys/stat.h>
#endif

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

struct MimallocHeap {
    mi_heap_t *heap = NULL;

    ~MimallocHeap() {
        if(heap != NULL)
            mi_heap_destroy(heap);
    }
};

static thread_local MimallocHeap TempArena;

void *AllocTemporary(size_t size) {
    if(TempArena.heap == NULL) {
        TempArena.heap = mi_heap_new();
        ssassert(TempArena.heap != NULL, "out of memory");
    }
    void *ptr = mi_heap_zalloc(TempArena.heap, size);
    ssassert(ptr != NULL, "out of memory");
    return ptr;
}

void FreeAllTemporary() {
    MimallocHeap temp;
    std::swap(TempArena.heap, temp.heap);
}

}
}
