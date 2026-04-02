#include <windows.h>
#include <dbghelp.h>
#include <iostream>

#pragma comment(lib, "dbghelp.lib")

void print_stacktrace() {
    void* stack[100];
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    WORD frames = CaptureStackBackTrace(0, 100, stack, NULL);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (int i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)stack[i], 0, symbol);
        std::cout << symbol->Name << std::endl;
    }
    free(symbol);
}

void foo() { print_stacktrace(); }
int main() { 
    foo(); 
    return 0;
}