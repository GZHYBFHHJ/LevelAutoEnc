#include "7z.h"

#include <windows.h>

static int _exec_cmd(const char *cmd, char *err, int errsize) {
    SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE hRead, hErr;
	if (!CreatePipe(&hRead, &hErr, &sa, 0)) return -1;
    
    STARTUPINFOA si;
    si.cb = sizeof(STARTUPINFOA);
    GetStartupInfoA(&si);
    si.wShowWindow = SW_HIDE;
    si.hStdError = hErr;
    si.hStdInput = NULL;
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    if (!CreateProcessA(NULL, (LPSTR)cmd/*why is it not LPCSTR*/, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hErr);
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(hErr);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    int i = 0;
    if (err) {
        errsize -= 1;
        while (i < errsize) {
            DWORD read;
            if (!ReadFile(hRead, err + i, errsize - i, &read, NULL) || read == 0) {
                break;
            }
            i += read;
        }
        err[i] = 0;
    }
    CloseHandle(hRead);
    return i;
}


int sevenzip_available() {
    return _exec_cmd("7z", NULL, 0) != -1;
}


struct SEVENZIP_RESULT {
    HANDLE file;
    int type;
    char path[MAX_PATH];
};

static inline int _getTempFile(char *filename) {
    char path[MAX_PATH - 2];
    if (GetTempPathA(sizeof(path), path) == 0) return 0;
    return GetTempFileNameA(path, "CMP", 0, filename);
}

static void remDirectory(const char *path) {
    char findpath[MAX_PATH];
    strcpy_s(findpath, sizeof(findpath), path);
    strcat_s(findpath, sizeof(findpath), "/*");

    WIN32_FIND_DATAA finddata;
    HANDLE hFind = FindFirstFileA(findpath, &finddata);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (strcmp(finddata.cFileName, ".") == 0 || strcmp(finddata.cFileName, "..") == 0) continue;

        char p[521];
        wsprintfA(p, "%s/%s", path, finddata.cFileName);
        if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            remDirectory(p);
        } else {
            DeleteFileA(p);
        }
    } while (FindNextFileA(hFind, &finddata));
    FindClose(hFind);

    RemoveDirectoryA(path);

}

static inline SEVENZIP_RESULT *sevenzip_res_new(HANDLE file, int type, const char *path) {
    if (file == INVALID_HANDLE_VALUE) {
        if (type == 1) {
            remDirectory(file);
        }
        return NULL;
    }

    SEVENZIP_RESULT *res = (SEVENZIP_RESULT *)HeapAlloc(GetProcessHeap(), 0, sizeof(SEVENZIP_RESULT));
    if (!res) {
        CloseHandle(file);
        if (type == 1) {
            remDirectory(path);
        }
        return NULL;
    }

    res->file = file;
    res->type = type;
    CopyMemory(res->path, path, sizeof(res->path));
    return res;
}

void sevenzip_res_free(SEVENZIP_RESULT *res) {
    if (!res) return;

    CloseHandle(res->file);
    if (res->type == 1) {
        remDirectory(res->path);
    } else {
        DeleteFileA(res->path);
    }
    HeapFree(GetProcessHeap(), 0, res);
}

int sevenzip_res_length(SEVENZIP_RESULT *res) {
    return GetFileSize(res->file, NULL);
}

int sevenzip_res_read(SEVENZIP_RESULT *res, unsigned char *output) {
    HANDLE hFile = res->file;

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    int len = GetFileSize(hFile, NULL);
    if (len == INVALID_FILE_SIZE)
        return 0;
    return ReadFile(hFile, output, len - 1, NULL, NULL);
}

SEVENZIP_RESULT *sevenzip_compress(const unsigned char *input, int inputlen) {
    char in[MAX_PATH];
    char out[MAX_PATH];
    if (_getTempFile(in) == 0) {
        return NULL;
    }
    if (_getTempFile(out) == 0) {
        DeleteFileA(in);
        return NULL;
    }
    DeleteFileA(out);

    HANDLE hIn = CreateFileA(in, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIn == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    if (!WriteFile(hIn, input, inputlen, NULL, NULL)) {
        CloseHandle(hIn);
        DeleteFileA(in);
        return NULL;
    }
    CloseHandle(hIn);
    
    char cmd[550];
    wsprintfA(cmd, "7z a -y %s %s", out, in);

    char err[8];
    int res = _exec_cmd(cmd, err, sizeof(err));
    DeleteFileA(in);
    if (res == -1 || res >= 7) {
        DeleteFileA(out);
        return NULL;
    }

    return sevenzip_res_new(CreateFileA(out, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL), 0, out);
}

SEVENZIP_RESULT *sevenzip_decompress(const unsigned char *input, int inputlen) {
    char in[MAX_PATH];
    char out[MAX_PATH];
    if (_getTempFile(in) == 0) {
        return NULL;
    }
    if (_getTempFile(out) == 0) {
        DeleteFileA(in);
        return NULL;
    }
    DeleteFileA(out);

    HANDLE hIn = CreateFileA(in, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIn == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    if (!WriteFile(hIn, input, inputlen, NULL, NULL)) {
        CloseHandle(hIn);
        DeleteFileA(in);
        return NULL;
    }
    CloseHandle(hIn);
    
    char cmd[550];
    wsprintfA(cmd, "7z e -y %s -o\"%s/\"", in, out);

    char err[8];
    int res = _exec_cmd(cmd, err, sizeof(err));
    DeleteFileA(in);
    if (res == -1 || res >= 7) {
        return NULL;
    }

    HANDLE hFile = INVALID_HANDLE_VALUE;

    char findpath[MAX_PATH];
    strcpy_s(findpath, sizeof(findpath), out);
    strcat_s(findpath, sizeof(findpath), "/*");

    WIN32_FIND_DATAA finddata;
    HANDLE hFind = FindFirstFileA(findpath, &finddata);
    if (hFind == INVALID_HANDLE_VALUE) return NULL;
    do {
        if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        char filepath[521];
        wsprintfA(filepath, "%s/%s", out, finddata.cFileName);
        hFile = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        break;
    } while (FindNextFileA(hFind, &finddata));
    FindClose(hFind);

    return sevenzip_res_new(hFile, 1, out);
}

