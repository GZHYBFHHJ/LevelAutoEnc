#ifndef _ANTOENC_HOOK_H_
#define _ANTOENC_HOOK_H_

#include <windows.h>

typedef struct {
    FARPROC orig;
    void *jmp;

    BYTE origBytes[5];
    BYTE newBytes[5];
} HOOKINFO;


void procHook(FARPROC origProc, void *jmpProc, HOOKINFO *hook);

void procUnhook(const HOOKINFO *hook);

void procRehook(const HOOKINFO *hook);

#endif