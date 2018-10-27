#pragma once

#include "stdafx.h"

#include "OsuBot.h"


typedef struct _CLIENT_ID {
	PVOID UniqueProcess;
	PVOID UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _THREAD_BASIC_INFORMATION {
	LONG ExitStatus;
	PVOID TebBaseAddress;
	CLIENT_ID ClientId;
	KAFFINITY AffinityMask;
	DWORD Priority;
	DWORD BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

enum THREADINFOCLASS {
	ThreadBasicInformation,
};


LPVOID GetBaseAddress(HANDLE hProcess, HANDLE hThread) {
	LPCTSTR moduleName = L"ntdll.dll";
	bool loadedManually = false;


	HMODULE hModule = GetModuleHandle(moduleName);

	if (!hModule) {
		hModule = LoadLibrary(moduleName);
		loadedManually = true;
	}

	LONG(__stdcall *NtQueryInformationThread)(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength);
	NtQueryInformationThread = reinterpret_cast<decltype(NtQueryInformationThread)>(GetProcAddress(hModule, "NtQueryInformationThread"));

	if (NtQueryInformationThread) {
		NT_TIB tib = { 0 };
		THREAD_BASIC_INFORMATION tbi = { 0 };

		LONG status = NtQueryInformationThread(hThread, ThreadBasicInformation, &tbi, sizeof(tbi), nullptr);
		if (status >= 0) {
			ReadProcessMemory(hProcess, tbi.TebBaseAddress, &tib, sizeof(tib), nullptr);

			if (loadedManually)
				FreeLibrary(hModule);

			return tib.StackBase;
		}
	}

	if (loadedManually)
		FreeLibrary(hModule);

	return NULL;
}

DWORD GetThreadStartAddress(HANDLE processHandle, HANDLE hThread) {
	DWORD stacktop = 0, result = 0;

	MODULEINFO mi;

	GetModuleInformation(processHandle, GetModuleHandle(L"kernel32.dll"), &mi, sizeof(mi));
	stacktop = (DWORD)GetBaseAddress(processHandle, hThread);

	CloseHandle(hThread);

	if (stacktop) {
		const int buf32Size = 4096;
		DWORD* buf32 = new DWORD[buf32Size];

		if (ReadProcessMemory(processHandle, (LPCVOID)(stacktop - buf32Size), buf32, buf32Size, NULL)) {
			for (int i = buf32Size / 4 - 1; i >= 0; --i) {
				if (buf32[i] >= (DWORD)mi.lpBaseOfDll && buf32[i] <= (DWORD)mi.lpBaseOfDll + mi.SizeOfImage) {
					result = stacktop - buf32Size + i * 4;
					break;
				}
			}
		}
		delete buf32;
	}
	return result;
}

vector<DWORD> GetThreadStack(HANDLE processHandle, vector<DWORD> threadList) {
	vector<DWORD> threadStack;

	for (auto it = threadList.begin(); it != threadList.end(); ++it) {
		HANDLE threadHandle = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, *it);
		DWORD threadStartAddress = GetThreadStartAddress(processHandle, threadHandle);
		threadStack.push_back(threadStartAddress);
	}

	return threadStack;
}

vector<DWORD> ThreadList(DWORD pid) {
	vector<DWORD> vect;
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (h == INVALID_HANDLE_VALUE)
		return vect;

	THREADENTRY32 te;
	te.dwSize = sizeof(te);
	if (Thread32First(h, &te)) {
		do {
			if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID)) {
				if (te.th32OwnerProcessID == pid) {
					vect.push_back(te.th32ThreadID);
				}
			}
			te.dwSize = sizeof(te);
		} while (Thread32Next(h, &te));
	}

	return vect;
}

HANDLE GetHandle(DWORD processId) {
	return OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
}

DWORD GetProcessID(string exe) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		/* EventLog */	fwprintf(wEventLog, L"[ERROR]  Could not get a snapshot of the process \"osu!\"!\n");
		fflush(wEventLog);

		statusText = L"Unable to get a Snapshot of the process!";
		DrawTextToWindow(hWnd, statusText, rectStatus);

		return NULL;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof pe;

	if (Process32First(hSnapshot, &pe)) {
		do {
			wstring str = pe.szExeFile;
			if (string(str.begin(), str.end()) == exe) {
				CloseHandle(hSnapshot);
				return pe.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &pe));
	}
	CloseHandle(hSnapshot);
	return NULL;
}

LPVOID FindPointerAddress(HANDLE pHandle, LPVOID baseAddress, int pLevel) {
	int address = reinterpret_cast<int>(baseAddress);

	for (int i = 0; i < pLevel; i++) {
		ReadProcessMemory(pHandle, (LPCVOID)address, &address, 4, NULL);
		address += offsets[i];
	}

	return reinterpret_cast<LPVOID>(address);
}

LPVOID GetAddress(HANDLE processHandle, LPVOID baseAddress, int pLevel) {
	return FindPointerAddress(processHandle, baseAddress, pLevel);
}
