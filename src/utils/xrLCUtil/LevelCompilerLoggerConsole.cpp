#include "pch.hpp"
#include "LevelCompilerLoggerConsole.hpp"

LevelCompilerLoggerConsole::LevelCompilerLoggerConsole() {}
void LevelCompilerLoggerConsole::Initialize(const char* name) {}
void LevelCompilerLoggerConsole::Destroy() {}
void LevelCompilerLoggerConsole::clMsg(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    clMsgV(format, args);
    va_end(args);
}

void LevelCompilerLoggerConsole::clMsgV(const char* format, va_list args)
{
    char buf[1024];
    vsprintf(buf, format, args);
    Msg("clMsg: %s", buf);
}

void LevelCompilerLoggerConsole::clLog(const char* format, ...)
{
    va_list mark;
    char buf[1024];
    va_start(mark, format);
    vsprintf(buf, format, mark);
    Msg("clLog: %s", buf);
}

void LevelCompilerLoggerConsole::Status(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    StatusV(format, args);
    va_end(args);
}

void LevelCompilerLoggerConsole::StatusV(const char* format, va_list args)
{
    char buf[1024] = "";
    vsprintf(buf, format, args);
    Msg("Status: %s", buf);
}

void LevelCompilerLoggerConsole::Progress(float progress) { Msg("Progress: %f", progress); }
void LevelCompilerLoggerConsole::Phase(const char* phaseName) { Msg("Phase: %s", phaseName); }
void LevelCompilerLoggerConsole::Success(const char* msg) {}
void LevelCompilerLoggerConsole::Failure(const char* msg) {}
