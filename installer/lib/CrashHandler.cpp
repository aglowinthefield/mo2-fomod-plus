#include "CrashHandler.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <sstream>
#include <fstream>

LPTOP_LEVEL_EXCEPTION_FILTER CrashHandler::previousFilter = nullptr;
PVOID CrashHandler::vectoredHandler = nullptr;
#endif

void CrashHandler::initialize() {
#ifdef _WIN32
    std::cout << "CrashHandler initializing..." << std::endl;
    vectoredHandler = AddVectoredExceptionHandler(1, vectoredExceptionHandler);
    previousFilter = SetUnhandledExceptionFilter(unhandledExceptionFilter);
    std::cout << "CrashHandler initialized successfully" << std::endl;
#else
    std::cout << "CrashHandler: Not supported on this platform" << std::endl;
#endif
}

void CrashHandler::cleanup() {
#ifdef _WIN32
    if (vectoredHandler) {
        RemoveVectoredExceptionHandler(vectoredHandler);
        vectoredHandler = nullptr;
    }
    if (previousFilter) {
        SetUnhandledExceptionFilter(previousFilter);
        previousFilter = nullptr;
    }
#endif
}

#ifdef _WIN32
LONG WINAPI CrashHandler::vectoredExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    // Log all exceptions but only create dumps for serious ones
    std::cout << "Exception caught: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
    
    // Only create dumps for serious crashes, not benign exceptions
    if (exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
        exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW ||
        exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION) {
        
        logCrashInfo(exceptionInfo);
        
        // Generate minidump
        HANDLE file = CreateFileA("fomod_plus_crash.dmp", GENERIC_WRITE, 0, nullptr,
                                 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        
        if (file != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {0};
            dumpInfo.ThreadId = GetCurrentThreadId();
            dumpInfo.ExceptionPointers = exceptionInfo;
            dumpInfo.ClientPointers = FALSE;
            
            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                             MiniDumpNormal, &dumpInfo, nullptr, nullptr);
            CloseHandle(file);
            
            std::cout << "Crash dump written to fomod_plus_crash.dmp" << std::endl;
        }
    }
    
    return EXCEPTION_CONTINUE_SEARCH;  // Let other handlers process it
}

LONG WINAPI CrashHandler::unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    logCrashInfo(exceptionInfo);
    return EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandler::logCrashInfo(const EXCEPTION_POINTERS* exceptionInfo) {
    std::ostringstream oss;
    oss << "FOMOD PLUS CRASH DETECTED!" << std::endl;
    oss << "Exception Code: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
    oss << "Exception Address: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionAddress << std::endl;
    
    // Write to stdout immediately
    std::cout << oss.str() << std::endl;
    
    // Also write to crash log file
    std::ofstream crashLog("fomod_plus_crash.log", std::ios::app);
    if (crashLog.is_open()) {
        crashLog << oss.str() << std::endl;
    }
}
#endif
