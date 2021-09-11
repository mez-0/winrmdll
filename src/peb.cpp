#include "peb.h"

HMODULE WINAPI zzGetModuleHandle(std::wstring sModuleName)
{
	#ifdef _M_IX86 
	PEB* peb = (PEB*)__readfsdword(0x30);
	#else
	PEB* peb = (PEB*)__readgsqword(0x60);
	#endif

	if (sModuleName.size() == 0)
	{
		(HMODULE)nullptr;
	}

	PEB_LDR_DATA* pebLdr = peb->Ldr;
	LIST_ENTRY* ModuleList = NULL;

	ModuleList = &pebLdr->InMemoryOrderModuleList;
	LIST_ENTRY* pStartListEntry = ModuleList->Flink;

	for (LIST_ENTRY* pListEntry = pStartListEntry; pListEntry != ModuleList; pListEntry = pListEntry->Flink)
	{
		LDR_DATA_TABLE_ENTRY* pEntry = (LDR_DATA_TABLE_ENTRY*)((BYTE*)pListEntry - sizeof(LIST_ENTRY));
		if (lstrcmpiW(pEntry->BaseDllName.Buffer, sModuleName.c_str()) == 0)
		{
			return (HMODULE)pEntry->DllBase;
		}
	}
	return (HMODULE)nullptr;
}

FARPROC WINAPI zzGetProcAddress(HMODULE hModule, std::string sProcName)
{
	char* pAddress = (char*)hModule;

	IMAGE_DOS_HEADER* pDOSHeaders = (IMAGE_DOS_HEADER*)pAddress;
	IMAGE_NT_HEADERS* pNTHeaders = (IMAGE_NT_HEADERS*)(pAddress + pDOSHeaders->e_lfanew);
	IMAGE_OPTIONAL_HEADER* pOptionalHeaders = &pNTHeaders->OptionalHeader;
	IMAGE_DATA_DIRECTORY* pExportDataDirectory = (IMAGE_DATA_DIRECTORY*)(&pOptionalHeaders->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
	IMAGE_EXPORT_DIRECTORY* pExportDirectory = (IMAGE_EXPORT_DIRECTORY*)(pAddress + pExportDataDirectory->VirtualAddress);

	DWORD* pExportAddressTable = (DWORD*)(pAddress + pExportDirectory->AddressOfFunctions);
	DWORD* pFunctionNameTable = (DWORD*)(pAddress + pExportDirectory->AddressOfNames);
	WORD* pHintsTable = (WORD*)(pAddress + pExportDirectory->AddressOfNameOrdinals);

	FARPROC pProcAddress = NULL;

	if (((DWORD_PTR)sProcName.c_str() >> 16) == 0)
	{
		WORD ordinal = (WORD)sProcName.c_str() & 0xFFFF;
		DWORD Base = pExportDirectory->Base;
		if (ordinal < Base || ordinal >= Base + pExportDirectory->NumberOfFunctions)
		{
			return NULL;
		}
		else
		{
			pProcAddress = (FARPROC)(pAddress + (DWORD_PTR)pExportAddressTable[ordinal - Base]);
		}
	}
	else
	{
		for (DWORD i = 0; i < pExportDirectory->NumberOfNames; i++)
		{
			char* sTmpFuncName = (char*)pAddress + (DWORD_PTR)pFunctionNameTable[i];
			if (strcmp(sProcName.c_str(), sTmpFuncName) == 0)
			{
				pProcAddress = (FARPROC)(pAddress + (DWORD_PTR)pExportAddressTable[pHintsTable[i]]);
				break;
			}
		}
	}
	return (FARPROC)pProcAddress;
}