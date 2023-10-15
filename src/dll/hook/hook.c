#include "hook.h"


void procHook(FARPROC origProc, void *jmpProc, HOOKINFO *hook) {
    hook->orig = origProc;
    hook->jmp = jmpProc;

    DWORD old;
    DWORD tmp;
    VirtualProtect(origProc, 5, PAGE_READWRITE, &old);

    CopyMemory(hook->origBytes, origProc, 5);

    hook->newBytes[0] = 0xE9;                                              // jmp
    *(DWORD *)&hook->newBytes[1] = (BYTE *)jmpProc - (BYTE *)origProc - 5;
    CopyMemory(origProc, hook->newBytes, 5);

    VirtualProtect(origProc, 5, old, &tmp);

}

void procUnhook(const HOOKINFO *hook) {
    FARPROC orig = hook->orig;

    DWORD old;
    DWORD tmp;
    VirtualProtect(orig, 5, PAGE_READWRITE, &old);
    CopyMemory(orig, hook->origBytes, 5);
    VirtualProtect(orig, 5, old, &tmp);

}

void procRehook(const HOOKINFO *hook) {
    FARPROC orig = hook->orig;

    DWORD old;
    DWORD tmp;
    VirtualProtect(orig, 5, PAGE_READWRITE, &old);
    CopyMemory(orig, hook->newBytes, 5);
    VirtualProtect(orig, 5, old, &tmp);

}

