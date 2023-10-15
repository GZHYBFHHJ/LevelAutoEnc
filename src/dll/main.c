#include <windows.h>
#include <winnt.h>

#include "proxy/loader.h"
#include "config.h"
#include "msvcr.h"
#include "fopenhook.h"

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            if (!proxy_findDll()) {
                MessageBox(NULL, TEXT("Failed to load original dll"), TEXT("Proxy Failed"), 0);
                return FALSE;
            }
            
            int result = config_load();
            if (result != CONFIG_SUCCESS) {
                if (result == CONFIG_INVALIDKEY)
                    MessageBox(NULL, TEXT("Encryption key size must be 32 characters"), TEXT("Invalid Config"), 0);
                
                return TRUE;
            }

            if (!msvcr_find()) {
                MessageBox(NULL, TEXT("Could not found 'msvcrXXX.dll'"), TEXT("Failed"), 0);
                return TRUE;
            }

            fopenhook_init();
            
            break;
        case DLL_PROCESS_DETACH:
            fopenhook_uninit();
            proxy_unloadDll();
            break;
    }
    return TRUE;
}

