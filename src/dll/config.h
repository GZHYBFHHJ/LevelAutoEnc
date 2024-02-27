#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <windows.h>

extern struct _config {
    char decOut[MAX_PATH];
    char encOut[MAX_PATH];

    char key[32 + 1];

    enum {
        compNone = 0,
        comp7z   = 1
    } compMode;

    int appendFn;

} config;

#define CONFIG_SUCCESS    0
#define CONFIG_DISABLED   1
#define CONFIG_INVALIDKEY 2

int config_load();

#endif