#include "log.h"

#include <stdio.h>
#include <windows.h>

static HANDLE _logfile;

void logging_enable() {
    _logfile = CreateFileA("log.txt", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

void logging_printf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    
    char output[128];
    int len = vsprintf_s(output, sizeof(output), fmt, args);

    if (_logfile) {
        WriteFile(_logfile, output, len, NULL, NULL);
        WriteFile(_logfile, "\r\n", 2, NULL, NULL);
        FlushFileBuffers(_logfile);
    }

    va_end(args);
}
