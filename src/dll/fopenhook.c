#include "fopenhook.h"

#include "hook/hook.h"
#include "config.h"
#include "msvcr.h"

#include "log.h"

#include "helper/aes.h"
#include "helper/7z.h"

typedef struct FILE FILE;

static HOOKINFO _fopen_hook = { 0 };
static HOOKINFO _wfopen_hook = { 0 }; // for ABSpace

typedef FILE *(*fopen_pt)(const char *filename, const char *mode);
static fopen_pt fopen;

static void createRoot(const char *path) {
    char p[MAX_PATH];
    strcpy_s(p, MAX_PATH, path);
    char *i = strrchr(p, '/');
    if (i == NULL) return;
    *i = 0;
    
    if (!CreateDirectoryA(p, NULL)) {
        createRoot(p);
        CreateDirectoryA(p, NULL);
    }

}


static int tryDecrypt(const char *filename) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        logging_printf("Failed to open file (tryDecrypt)");
        return 0;
    }

    int size = GetFileSize(hFile, NULL);
    if (size == 0) {
        logging_printf("Empty file, skipping");
        return 1;
    }
    unsigned char *filedata = (unsigned char *)VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (!filedata) {
        logging_printf("Failed to allocate");
        CloseHandle(hFile);
        return 0;
    }
    int i = 0;
    while (i < size) {
        DWORD read;
        if (!ReadFile(hFile, filedata + i, size - i, &read, NULL)) {
            logging_printf("Failed to read data");
            VirtualFree(filedata, 0, MEM_RELEASE);
            CloseHandle(hFile);
            return 0;
        }
        i += read;
    }
    CloseHandle(hFile);

    unsigned char *decdata = (unsigned char *)VirtualAlloc(NULL, size + AES_MAX_BLOCK_LENGTH, MEM_COMMIT, PAGE_READWRITE);
    if (!decdata) {
        logging_printf("Failed to allocated");
        VirtualFree(filedata, 0, MEM_RELEASE);
        return 0;
    }
    int decsize = aes_decrypt(filedata, size, decdata);
    VirtualFree(filedata, 0, MEM_RELEASE);
    if (decsize == -1) {
        logging_printf("Bad Decrypt (file is not encrypted)");
        VirtualFree(decdata, 0, MEM_RELEASE);
        return 0;
    }

    int reslen = decsize;
    unsigned char *res = decdata;
    if (config.compMode == comp7z) {
        SEVENZIP_RESULT *sr = sevenzip_decompress(decdata, decsize);
        VirtualFree(decdata, 0, MEM_RELEASE);
        if (!sr) {
            logging_printf("Failed to decompress (file is not compressed or error)");
            return 0;
        }
        reslen = sevenzip_res_length(sr);
        res = (unsigned char *)VirtualAlloc(NULL, reslen, MEM_COMMIT, PAGE_READWRITE);
        if (!res) {
            logging_printf("Failed to allocate");
            sevenzip_res_free(sr);
            return 0;
        }
        if (!sevenzip_res_read(sr, res)) {
            logging_printf("Failed to read data");
            VirtualFree(res, 0, MEM_RELEASE);
            sevenzip_res_free(sr);
            return 0;
        }
        sevenzip_res_free(sr);
    }

    char path[521];
    wsprintfA(path, "%s/%s", config.decOut, filename);

    logging_printf("Decryption success. writing decrypted data to `%s`", path);

    createRoot(path);

    hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        VirtualFree(res, 0, MEM_RELEASE);
        return 1;
    }

    i = 0;
    while (i < reslen) {
        DWORD read;
        if (!WriteFile(hFile, res + i, reslen - i, &read, NULL)) {
            CloseHandle(hFile);
            VirtualFree(res, 0, MEM_RELEASE);
            return 1;
        }
        i += read;
    }

    CloseHandle(hFile);
    VirtualFree(res, 0, MEM_RELEASE);
    return 1;
}

static int tryEncrypt(const char *filename, char *outpath) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        logging_printf("Failed to open file (tryEncrypt read)");
        return 0;
    }

    int size = GetFileSize(hFile, NULL);
    if (size == 0) {
        logging_printf("Empty file. Skipping");
        return 0;
    }
    unsigned char *filedata = (unsigned char *)VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (!filedata) {
        logging_printf("Failed to allocate");
        CloseHandle(hFile);
        return 0;
    }
    int i = 0;
    while (i < size) {
        DWORD read;
        if (!ReadFile(hFile, filedata + i, size - i, &read, NULL)) {
            logging_printf("Failed to read data");
            VirtualFree(filedata, 0, MEM_RELEASE);
            CloseHandle(hFile);
            return 0;
        }
        i += read;
    }
    CloseHandle(hFile);

    int reslen = size;
    unsigned char *res = filedata;
    if (config.compMode == comp7z) {
        SEVENZIP_RESULT *sr = sevenzip_compress(filedata, size);
        VirtualFree(filedata, 0, MEM_RELEASE);
        if (!sr) {
            logging_printf("Failed to compress");
            return 0;
        }
        reslen = sevenzip_res_length(sr);
        res = (unsigned char *)VirtualAlloc(NULL, reslen, MEM_COMMIT, PAGE_READWRITE);
        if (!res) {
            logging_printf("Failed to allocate");
            sevenzip_res_free(sr);
            return 0;
        }
        if (!sevenzip_res_read(sr, res)) {
            logging_printf("Failed to read compressed data");
            VirtualFree(res, 0, MEM_RELEASE);
            sevenzip_res_free(sr);
            return 0;
        }
        sevenzip_res_free(sr);
    }

    int enclen;
    unsigned char *encdata = (unsigned char *)VirtualAlloc(NULL, reslen + AES_MAX_BLOCK_LENGTH, MEM_COMMIT, PAGE_READWRITE);
    if (!encdata) {
        logging_printf("Failed to allocate");
        VirtualFree(res, 0, MEM_RELEASE);
        return 0;
    }
    enclen = aes_encrypt(res, reslen, encdata);
    VirtualFree(res, 0, MEM_RELEASE);
    if (enclen == -1) {
        logging_printf("Failed to encrypt");
        return 0;
    }
    
    wsprintfA(outpath, "%s/%s", config.encOut, filename);

    logging_printf("Encryption success. Writing encrypted data to %s", outpath);

    createRoot(outpath);

    hFile = CreateFileA(outpath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        logging_printf("Failed to create file (tryEncrypt write)");
        VirtualFree(res, 0, MEM_RELEASE);
        return 0;
    }

    i = 0;
    while (i < enclen) {
        DWORD read;
        if (!WriteFile(hFile, encdata + i, enclen - i, &read, NULL)) {
            logging_printf("Failed to write result data");
            CloseHandle(hFile);
            VirtualFree(encdata, 0, MEM_RELEASE);
            return 0;
        }
        i += read;
    }

    CloseHandle(hFile);
    VirtualFree(encdata, 0, MEM_RELEASE);
    return 1;
}


static int runPathLen;
static char runPath[MAX_PATH] = { 0 };

static FILE *_fopen_hookfunc(const char *filename, const char *mode) {
    FILE *res = NULL;
    procUnhook(&_fopen_hook);

    logging_printf("fopen: %s (mode: %s)", filename, mode);

    if (strlen(filename) > 4 && RtlEqualMemory(filename + strlen(filename) - 4, ".lua", 4)) { // lua file
        if (strlen(filename) > runPathLen && RtlEqualMemory(filename, runPath, runPathLen)) { // Classic 4.0.0
            filename += runPathLen;
            logging_printf("Relative path: %s", filename);
        }

        if (strstr(filename, ":/") || !strstr(filename, "/")) { // config.lua or save file
            logging_printf("config.lua or save file, skipping");
            res = fopen(filename, mode);
            goto ret;
        }
        if (RtlEqualMemory(mode, "wb", 2)) { // save level
            if (!config.appendFn) {
                res = fopen(filename, mode);
                goto ret;
            }

            logging_printf("Patching filename for saved level file");
            
            HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (!hFile) {
                logging_printf("Failed to open file. fallback");
                res = fopen(filename, mode);
                goto ret;
            }

            WriteFile(hFile, "filename = \"", 12, NULL, NULL);
            char *fn = strrchr(filename, '/') + 1;
            WriteFile(hFile, fn, strlen(fn), NULL, NULL);
            WriteFile(hFile, "\"\r\n", 3, NULL, NULL);

            CloseHandle(hFile);

            res = fopen(filename, "ab");
        } else {
            char outPath[MAX_PATH];
            if (tryDecrypt(filename)) {
                res = fopen(filename, mode);
            } else if (!tryEncrypt(filename, outPath)) {
                res = NULL;
            } else {
                res = fopen(outPath, mode);
            }
        }
    } else {
        res = fopen(filename, mode);
    }

    ret:
    procRehook(&_fopen_hook);
    return res;
}

static FILE *_wfopen_hookfunc(const wchar_t *filename, const wchar_t *mode) {
    // Unicode To ANSI

    char *mFilename, *mMode;
    int len;

    len = WideCharToMultiByte(CP_ACP, 0, filename, wcslen(filename), NULL, 0, NULL, NULL);
    mFilename = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + 1);
    if (mFilename == NULL) return NULL;
    WideCharToMultiByte(CP_ACP, 0, filename, wcslen(filename), mFilename, len, NULL, NULL);

    len = WideCharToMultiByte(CP_ACP, 0, mode, wcslen(mode), NULL, 0, NULL, NULL);
    mMode = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + 1);
    if (mMode == NULL) {
        HeapFree(GetProcessHeap(), 0, mFilename);
        return NULL;
    }
    WideCharToMultiByte(CP_ACP, 0, mode, wcslen(mode), mMode, len, NULL, NULL);

    FILE *res = _fopen_hookfunc(mFilename, mMode);

    HeapFree(GetProcessHeap(), 0, mFilename);
    HeapFree(GetProcessHeap(), 0, mMode);
    return res;
}


void fopenhook_init() {
    aes_initkey256cbc(config.key);

    if (config.compMode == comp7z && !sevenzip_available()) {
        MessageBoxA(0, "7z.exe not found!\r\nHave you copied '7z.exe' and '7z.dll' to the game directory?", "Error", 0);
        return;
    }

    FARPROC proc_fopen = GetProcAddress(msvcr_module, "fopen");
    FARPROC proc_wfopen = GetProcAddress(msvcr_module, "_wfopen");

    if (proc_fopen == NULL && proc_wfopen == NULL) {
        MessageBox(NULL, TEXT("API 'fopen' and '_wfopen' not found"), TEXT("Error"), 0);
        return;
    }

    logging_printf("Patching `fopen`");
    procHook(proc_fopen, &_fopen_hookfunc, &_fopen_hook);
    logging_printf("Patching `_wfopen`");
    procHook(proc_wfopen, &_wfopen_hookfunc, &_wfopen_hook);

    fopen = (fopen_pt)proc_fopen;

    runPathLen = GetCurrentDirectoryA(MAX_PATH - 2, runPath);
    int i = 0;
    for (; i < runPathLen; ++i) {
        if (runPath[i] == '\\') {
            runPath[i] = '/';
        }
    }
    runPath[i] = '/';
    ++runPathLen;

    logging_printf("Current Directory: %s", runPath);

}

void fopenhook_uninit() {
    if (_fopen_hook.orig == NULL) return;

    procUnhook(&_fopen_hook);
    procUnhook(&_wfopen_hook);
    _fopen_hook.orig = NULL;

}
