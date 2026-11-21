#pragma once
// Pixelwolf Edition: opt-in, coarse load-time instrumentation.
// Enable from PW Edition > Capture Load Diagnostics, or set GGCPW_PROFILE_LOAD=1 before launch.
// Output: %TEMP%\\GGCPW-loadtrace-<process-id>.csv
// No file is touched and no QPC call is made unless explicitly enabled.
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>

inline bool pw_loadtrace_enabled()
{
    // Recheck on entry to each coarse phase: users can enable diagnostics
    // from the running editor without relaunching or rebuilding.
    char option[8] = { 0 };
    DWORD n = GetEnvironmentVariableA("GGCPW_PROFILE_LOAD", option, sizeof(option));
    return n == 1 && option[0] == '1';
}

class PWLoadTraceScope
{
public:
    explicit PWLoadTraceScope(const char* stage)
        : stage_(stage), enabled_(pw_loadtrace_enabled())
    {
        start_.QuadPart = 0;
        if (enabled_)
        {
            if (!QueryPerformanceCounter(&start_)) { enabled_ = false; return; }
            // A separate begin marker survives a hang or crash inside this
            // phase. Timing summaries omit begin.* but raw CSV keeps it.
            char marker[256] = { 0 };
            if (sprintf_s(marker, sizeof(marker), "begin.%s", stage_) > 0)
                WriteSample(marker, 0.0);
        }
    }

    ~PWLoadTraceScope()
    {
        if (!enabled_) return;
        LARGE_INTEGER finish;
        LARGE_INTEGER frequency;
        if (!QueryPerformanceCounter(&finish) || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart == 0)
            return;

        const double durationMs = 1000.0 * double(finish.QuadPart - start_.QuadPart) / double(frequency.QuadPart);
        WriteSample(stage_, durationMs);
        if (durationMs >= 10000.0)
        {
            char warning[256] = { 0 };
            if (sprintf_s(warning, sizeof(warning), "slow.%s", stage_) > 0)
                WriteSample(warning, durationMs);
        }
    }

    PWLoadTraceScope(const PWLoadTraceScope&) = delete;
    PWLoadTraceScope& operator=(const PWLoadTraceScope&) = delete;

private:
    static void WriteSample(const char* stage, double durationMs)
    {
        char tempDir[MAX_PATH] = { 0 };
        DWORD n = GetTempPathA(MAX_PATH, tempDir);
        if (n == 0 || n >= MAX_PATH) return;

        char fileName[MAX_PATH + 80] = { 0 };
        int written = sprintf_s(fileName, sizeof(fileName), "%sGGCPW-loadtrace-%lu.csv",
            tempDir, (unsigned long)GetCurrentProcessId());
        if (written <= 0 || written >= (int)sizeof(fileName)) return;

        HANDLE file = CreateFileA(fileName, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE) return;

        LARGE_INTEGER fileSize;
        DWORD bytes = 0;
        if (GetFileSizeEx(file, &fileSize) && fileSize.QuadPart == 0)
        {
            const char header[] = "tick_ms,pid,stage,duration_ms\r\n";
            WriteFile(file, header, (DWORD)(sizeof(header) - 1), &bytes, NULL);
        }

        char row[512] = { 0 };
        written = sprintf_s(row, sizeof(row), "%llu,%lu,%s,%.3f\r\n",
            (unsigned long long)GetTickCount64(),
            (unsigned long)GetCurrentProcessId(), stage, durationMs);
        if (written > 0 && written < (int)sizeof(row))
            WriteFile(file, row, (DWORD)written, &bytes, NULL);
        CloseHandle(file);
    }

    const char* stage_;
    bool enabled_;
    LARGE_INTEGER start_;
};
#else
class PWLoadTraceScope
{
public:
    explicit PWLoadTraceScope(const char*) {}
};
#endif
