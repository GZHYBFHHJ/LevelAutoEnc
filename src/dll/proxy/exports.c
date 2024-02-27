#include "loader.h"

#include <windows.h>


#define DLL_NAME "\\dsound.dll"

static HMODULE dll_module;

int proxy_findDll() {
    char path[MAX_PATH];
    int size = GetSystemDirectoryA(path, MAX_PATH);
    CopyMemory(path + size, DLL_NAME, sizeof(DLL_NAME));

    dll_module = LoadLibraryA(path);
    if (dll_module) return 1;
    return 0;
}

void proxy_unloadDll() {
    if (dll_module) {
        FreeLibrary(dll_module);
        dll_module = NULL;
    }
}


#define PROXY_DEFINES(ret, name, ...) typedef ret (WINAPI* name##_TYPE)(__VA_ARGS__); \
                                            static void *name##_PTR;

#define PROXY_GETFUNC(name) if (!name##_PTR) name##_PTR = GetProcAddress(dll_module, #name) 

#define PROXY_FUNC_0(ret, name) PROXY_DEFINES(ret, name) \
                                ret WINAPI name() { \
                                    PROXY_GETFUNC(name); \
                                    return ((name##_TYPE)name##_PTR)(); \
                                }

#define PROXY_FUNC_2(ret, name, p0, p1) PROXY_DEFINES(ret, name, p0, p1) \
                                        ret WINAPI name(p0 _0, p1 _1) { \
                                            PROXY_GETFUNC(name); \
                                            return ((name##_TYPE)name##_PTR)(_0, _1); \
                                        }

#define PROXY_FUNC_3(ret, name, p0, p1, p2) PROXY_DEFINES(ret, name, p0, p1, p2) \
                                            ret WINAPI name(p0 _0, p1 _1, p2 _2) { \
                                                PROXY_GETFUNC(name); \
                                                return ((name##_TYPE)name##_PTR)(_0, _1, _2); \
                                            }

#define PROXY_FUNC_10(ret, name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9) PROXY_DEFINES(ret, name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9) \
                                                                         ret WINAPI name(p0 _0, p1 _1, p2 _2, p3 _3, p4 _4, p5 _5, p6 _6, p7 _7, p8 _8, p9 _9) { \
                                                                             PROXY_GETFUNC(name); \
                                                                             return ((name##_TYPE)name##_PTR)(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9); \
                                                                         }



PROXY_FUNC_3(HRESULT, DirectSoundCreate, LPCGUID, void * /* LPDIRECTSOUND* */, LPUNKNOWN)
PROXY_FUNC_2(HRESULT, DirectSoundEnumerateA, void * /* LPDSENUMCALLBACK */, LPVOID)
PROXY_FUNC_2(HRESULT, DirectSoundEnumerateW, void * /* LPDSENUMCALLBACK */, LPVOID)
PROXY_FUNC_0(HRESULT, DllCanUnloadNow)
PROXY_FUNC_3(HRESULT, DllGetClassObject, REFCLSID, REFIID, LPVOID *)
PROXY_FUNC_3(HRESULT, DirectSoundCaptureCreate, LPCGUID, void * /* LPDIRECTSOUNDCAPTURE* */, LPUNKNOWN)
PROXY_FUNC_2(HRESULT, DirectSoundCaptureEnumerateA, void * /* LPDSENUMCALLBACK */, LPVOID)
PROXY_FUNC_2(HRESULT, DirectSoundCaptureEnumerateW, void * /* LPDSENUMCALLBACK */, LPVOID)
PROXY_FUNC_2(HRESULT, GetDeviceID, LPCGUID, LPGUID)
PROXY_FUNC_10(HRESULT,
              DirectSoundFullDuplexCreate, 
              LPCGUID, 
              LPCGUID, 
              void * /* LPCDSCBUFFERDESC */, 
              void * /* LPCDSBUFFERDESC */, 
              HWND, 
              DWORD, 
              void * /* PDIRECTSOUNDFULLDUPLEX* */,
              void * /* LPDIRECTSOUNDCAPTUREBUFFER8* */,
              void * /* LPDIRECTSOUNDBUFFER8* */,
              LPUNKNOWN
              )
PROXY_FUNC_3(HRESULT, DirectSoundCreate8, LPGUID, void * /* LPDIRECTSOUND8* */, LPUNKNOWN)
PROXY_FUNC_3(HRESULT, DirectSoundCaptureCreate8, LPCGUID, void * /* LPDIRECTSOUNDCAPTURE8* */, LPUNKNOWN)
