#include "EditorLogBuffer.h"

FEditorLogBuffer::~FEditorLogBuffer() { LogEntries.clear(); }

void FEditorLogBuffer::Log(ELogLevel Level, const char* Message)
{
    FEditorLogEntry LogEntry;
    LogEntry.Level = Level;
    LogEntry.Message = (Message != nullptr) ? Message : "";
    LogEntries.push_back(LogEntry);
}

void FEditorLogBuffer::Clear() { LogEntries.clear(); }
