// PW Edition: isolated cOccluderThread cooperative shutdown regression test.
// Does not compile, render, or require proprietary GameGuru media.
#include <windows.h>
#include "cOccluderThread.h"
#include <cassert>
#include <atomic>
#include <cstdio>

HANDLE g_hOccluderBegin = NULL;
HANDLE g_hOccluderEnd = NULL;
std::atomic<int> g_invocations(0);
void CPU3DDoOcclude() { ++g_invocations; }

int main()
{
    for (int cycle = 0; cycle < 12; ++cycle)
    {
        g_hOccluderBegin = CreateEventA(NULL, FALSE, FALSE, NULL);
        g_hOccluderEnd = CreateEventA(NULL, FALSE, FALSE, NULL);
        assert(g_hOccluderBegin && g_hOccluderEnd);

        cOccluderThread* worker = new cOccluderThread();
        worker->Start();
        const int before = g_invocations.load();
        assert(SetEvent(g_hOccluderBegin));
        for (int tries = 0; tries < 100 && g_invocations.load() == before; ++tries)
            Sleep(5);
        assert(g_invocations.load() > before);

        worker->Stop();
        assert(SetEvent(g_hOccluderBegin)); // wake an otherwise idle worker
        worker->Join();
        const int stoppedCount = g_invocations.load();
        Sleep(5);
        assert(g_invocations.load() == stoppedCount); // no callbacks after teardown
        if (worker->pThread) { CloseHandle((HANDLE)worker->pThread); worker->pThread = NULL; }
        delete worker;
        CloseHandle(g_hOccluderBegin);
        CloseHandle(g_hOccluderEnd);
        g_hOccluderBegin = NULL;
        g_hOccluderEnd = NULL;
    }
    puts("PW occluder thread cleanup passed 12 synthetic level cycles.");
    return 0;
}
