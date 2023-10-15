#include "config.h"

#include <windows.h>


#define CONFIG_FILE "./autoenc.ini"
#define SECTION_NAME "ABAutoEnc"

static inline int config_getint(const char *key, int def) {
    return GetPrivateProfileIntA(SECTION_NAME, key, def, CONFIG_FILE);
}

static inline int config_getbool(const char *key, int def) {
    return config_getint(key, def) != 0;
}

static inline int config_getstring(const char *key, const char *def, char *out, int outsize) {
    return GetPrivateProfileStringA(SECTION_NAME, key, def, out, outsize, CONFIG_FILE);
}


struct _config config;

int config_load() {
    if (!config_getbool("Enabled", 0)) {
        return CONFIG_DISABLED;
    }
    
    int len = config_getstring("Key", "", config.key, 32 + 1);
    if (len != 32) return CONFIG_INVALIDKEY;

    config_getstring("DecryptOutDir", "dec", config.decOut, sizeof(config.decOut));
    config_getstring("EncryptOutDir", "enc", config.encOut, sizeof(config.encOut));

    config.compMode = config_getint("CompressionMode", compNone);
    config.appendFn = config_getbool("AppendFilename", 1);

    return CONFIG_SUCCESS;
}
