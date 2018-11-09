#pragma once

#include "stdafx.h"

#include "OsuBot.h"

#ifdef _WIN64 // Offsets for _WIN64 compatibility
ULONG xBaseOffset = 8192UL;
ULONG xStackOffset = 90112UL;
#elif _WIN32 // Offsets are not needed (set to 0UL) on _WIN32
ULONG xBaseOffset = 0UL;
ULONG xStackOffset = 0UL;
#endif // Offsets for _WIN64 compatibility

typedef struct _CLIENT_ID {
	LPVOID UniqueProcess;
	LPVOID UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _THREAD_BASIC_INFORMATION {
	LONG ExitStatus;
	LPVOID TebBaseAddress;
	CLIENT_ID ClientId;
	KAFFINITY AffinityMask;
	ULONG Priority;
	ULONG BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

enum THREADINFOCLASS {
	ThreadBasicInformation
};


LPVOID GetBaseAddress(HANDLE hProcess, HANDLE hThread) {
	LPCTSTR moduleName = L"ntdll.dll";
	bool loadedManually = FALSE;

	HMODULE hModule = GetModuleHandle(moduleName);

	if (!hModule) {
		hModule = LoadLibrary(moduleName);
		loadedManually = TRUE;
	}

	LONG(__stdcall *NtQueryInformationThread)(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, LPVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength);
	NtQueryInformationThread = reinterpret_cast<decltype(NtQueryInformationThread)>(GetProcAddress(hModule, "NtQueryInformationThread"));

	if (NtQueryInformationThread) {
		NT_TIB tib = { NULL };
		THREAD_BASIC_INFORMATION tbi = { NULL };

		LONG ntStatus = NtQueryInformationThread(hThread, ThreadBasicInformation, &tbi, sizeof(tbi), nullptr);
		if (ntStatus >= NULL) {
			tbi.TebBaseAddress = UlongToPtr(PtrToUlong(tbi.TebBaseAddress) + xBaseOffset);

			ReadProcessMemory(hProcess, tbi.TebBaseAddress, &tib, sizeof(tib), nullptr);

			if (loadedManually) {
				FreeLibrary(hModule);
				loadedManually = FALSE;
			}
			/* EventLog */	fwprintf(wEventLog, (L"[DEBUG]  GetBaseAddress / TebBaseAddress = " + to_wstring(PtrToUlong(tbi.TebBaseAddress)) + L"\n").c_str());
			/* EventLog */	fwprintf(wEventLog, (L"[DEBUG]  GetBaseAddress / StackBase = " + to_wstring(PtrToUlong(tib.StackBase) + xStackOffset) + L"\n").c_str()); fflush(wEventLog);

			return UlongToPtr(PtrToUlong(tib.StackBase) + xStackOffset);
		}
	}

	if (loadedManually) {
		FreeLibrary(hModule);
		loadedManually = FALSE;
	}

	return NULL;
}

ULONG GetThreadStartAddress(HANDLE hProcess, HANDLE hThread) {
	ULONG stacktop = NULL;
	ULONG result = NULL;

	MODULEINFO mi;

	HMODULE hModule = LoadLibraryEx(L"kernel32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

	GetModuleInformation(hProcess, hModule, &mi, sizeof(mi));
	stacktop = PtrToUlong(GetBaseAddress(hProcess, hThread));

	CloseHandle(hThread);

	if (stacktop) {
		const LONG buff32size = 4096L;
		ULONG* buff32 = new ULONG[buff32size];

		if (ReadProcessMemory(hProcess, UlongToPtr(stacktop - (ULONG)buff32size), buff32, buff32size, nullptr)) {
			for (LONG i = buff32size / 4L - 1L; i >= NULL; --i) {
				if (buff32[i] >= PtrToUlong(mi.lpBaseOfDll) && buff32[i] <= PtrToUlong(mi.lpBaseOfDll) + mi.SizeOfImage) {
					result = (stacktop - (ULONG)buff32size + i * 4UL);
					break;
				}
			}
		}
		delete buff32;
	}

	FreeModule(hModule);
	
	/* EventLog */	fwprintf(wEventLog, (L"[DEBUG]  GetThreadStartAddress / result = " + to_wstring(result) + L"\n").c_str()); fflush(wEventLog);

	return result;
}

ULONG GetThreadStack(HANDLE hProcess, LPVOID thread) {
	HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, PtrToUlong(thread));
	return GetThreadStartAddress(hProcess, hThread);
}

vector<LPVOID> GetThreadList(LPVOID pid) {
	vector<LPVOID> threadList;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, NULL);

	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return threadList;
	}

	THREADENTRY32 te;
	te.dwSize = sizeof(te);

	if (Thread32First(hSnapshot, &te)) {
		do {
			if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID)) {
				if ((ULONG)te.th32OwnerProcessID == PtrToUlong(pid)) {
					threadList.push_back(ULongToPtr((ULONG)te.th32ThreadID));
				}
			}
			te.dwSize = sizeof(te);
		} while (Thread32Next(hSnapshot, &te));
	}

	return threadList;
}

HANDLE GetHandle(LPVOID processId) {
	return OpenProcess(PROCESS_ALL_ACCESS | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PtrToUlong(processId));
}

LPVOID GetProcessID(wstring exe) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (hSnapshot == INVALID_HANDLE_VALUE) {
		/* EventLog */	fwprintf(wEventLog, L"[ERROR]  Could not get a snapshot of the process \"osu!\"!\n");
		fflush(wEventLog);

		statusText = L"Unable to get a Snapshot of the process!";
		DrawTextToWindow(hWnd, statusText, rectStatus);

		return NULL;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	if (Process32First(hSnapshot, &pe)) {
		do {
			if (pe.szExeFile == exe) {
				CloseHandle(hSnapshot);
				return ULongToPtr((ULONG)pe.th32ProcessID);
			}
		} while (Process32Next(hSnapshot, &pe));
	}

	CloseHandle(hSnapshot);
	return NULL;
}

LPVOID GetAddress(HANDLE hProcess, LPVOID baseAddress, UINT pLevel) {
	ULONG address;
	ULONG result;

	ReadProcessMemory(hProcess, baseAddress, &address, sizeof(address), nullptr);

	for (UINT i = 1U; i < pLevel; i++) {
		ReadProcessMemory(hProcess, ULongToPtr(address), &result, sizeof(result), nullptr);
		address = result + offsets[i];
	}

	return ULongToPtr(address);
}
