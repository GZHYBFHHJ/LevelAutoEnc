#include "msvcr.h"

#include <tlhelp32.h>

HMODULE msvcr_module = NULL;

int msvcr_find() {

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32 me32 = { 0 };
    me32.dwSize = sizeof(MODULEENTRY32);

    int found = 0;
    while (Module32Next(snapshot, &me32)) {
        if (RtlCompareMemory(me32.szModule, "MSVCRT", 6) == 5 || RtlCompareMemory(me32.szModule, "msvcrt", 6) == 5) {
            msvcr_module = me32.hModule;
            found = 1;
            break;
        }
    }
    CloseHandle(snapshot);
    return found;
}