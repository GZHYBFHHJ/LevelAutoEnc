
#include <stdio.h>
#include <windows.h>


#define EXE_NAME "AngryBirds.exe"
#define DLL_NAME "libABAutoEnc.dll"


int main(int argc, char* argv[]) {
    char *exeName = EXE_NAME;
    if (argc > 1) {
        exeName = argv[1];
    }

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessA(NULL, exeName, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "Failed to create process %s: %d\n", exeName, GetLastError());
        getchar();
        return 1;
    }

    const char *dllName = DLL_NAME;

    void *p = VirtualAllocEx(pi.hProcess, NULL, strlen(dllName) + 1, MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(pi.hProcess, p, dllName, strlen(dllName) + 1, NULL);
    
    void *loadLibraryAddr = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(pi.hProcess, NULL, 0,
            (LPTHREAD_START_ROUTINE)loadLibraryAddr,
            p,
            0,
            NULL);

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(pi.hProcess, p, strlen(dllName) + 1, MEM_DECOMMIT);
    CloseHandle(hThread);

    ResumeThread(pi.hThread);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (GetLastError() != 0) {
        fprintf(stderr, "Failed to inject to game: %d\n", GetLastError());
        getchar();
        return 1;
    }

    printf("ok");
    return 0;
}