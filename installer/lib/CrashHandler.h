#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

class CrashHandler {
public:
    static void initialize();
    static void cleanup();

private:
#ifdef _WIN32
    static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
    static LONG WINAPI vectoredExceptionHandler(EXCEPTION_POINTERS* exceptionInfo);
    static void logCrashInfo(const EXCEPTION_POINTERS* exceptionInfo);
    static LPTOP_LEVEL_EXCEPTION_FILTER previousFilter;
    static PVOID vectoredHandler;
#endif
};
