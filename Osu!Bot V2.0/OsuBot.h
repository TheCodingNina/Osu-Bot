#pragma once

#define STACK_LENIENCE 3

#define MODE_STANDARD 2001
#define MODE_FLOWING 2002

#include "stdafx.h"

using namespace std;

//Global Variables:
FILE *wEventLog;

vector<TimingPoint> timingPoints;
vector<HitObject> hitObjects;

wstring songsPath;
wstring beatmapPath;
wstring displayBeatmapPath;

HWND osuWindow, hwndProgressBar;
HANDLE osuProcessHandle;
LPVOID timeAddress;

IFileDialog *pfd, *pfd2;
IShellItem *psi, *psi2;

vec2f pBack;
vec2f pCursor, pP, pB, pE, pN;
POINT cursorPoint;

INPUT input;

int osuWindowX, osuWindowY;
int	songTime, prevTime, prevInputTime;

int
modeMoveTo = MODE_FLOWING,
modeSlider = MODE_FLOWING,
modeSpinner = MODE_STANDARD;

float
multiplierX, multiplierY,
stackLeniency,
beatMapDivider,
overallDifficulty,
circleSize,
sliderMultiplier,
sliderTickRate,
stackOffset,
BPM;

bool
songStarted,
pathSet,
autoOpenSong,
altKey,
inputFlip,
inputKeyBoard = TRUE,
firstStart = TRUE,
nextObject = TRUE;

extern wstring statusText;
extern wstring songFileText;
extern HWND hWnd;
extern HWND hwndButtonOpenSongFile;
extern int nHeight = 290, nWidth = 585;
extern WORD inputMainKey = 0xBC;
extern WORD inputAltKey = 0xBE;
extern RECT rectStatus = { 15, nHeight - 65, nWidth - (2 * rectStatus.left), rectStatus.top + 20 };
extern RECT rectSongFile;
extern string userSelect = "user.\n";
extern string autoSelect = "Osu!Bot.\n";
//Global Variables END;


LPVOID GetBaseAddress(HANDLE hProcess, HANDLE hThread) {
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


	LPCWSTR moduleName = L"ntdll.dll";
	bool loadedManually = false;


	HMODULE module = GetModuleHandle(moduleName);

	if (!module) {
		module = LoadLibrary(moduleName);
		loadedManually = true;
	}

	LONG(__stdcall *NtQueryInformationThread)(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength);
	NtQueryInformationThread = reinterpret_cast<decltype(NtQueryInformationThread)>(GetProcAddress(module, "NtQueryInformationThread"));

	if (NtQueryInformationThread) {
		NT_TIB tib = { 0 };
		THREAD_BASIC_INFORMATION tbi = { 0 };

		LONG status = NtQueryInformationThread(hThread, ThreadBasicInformation, &tbi, sizeof(tbi), nullptr);
		if (status >= 0) {
			ReadProcessMemory(hProcess, tbi.TebBaseAddress, &tib, sizeof(tib), nullptr);

			if (loadedManually)
				FreeLibrary(module);

			return tib.StackBase;
		}
	}

	if (loadedManually)
		FreeLibrary(module);

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
		/* EventLog */	fprintf(wEventLog, "[ERROR]  Could not get a snapshot of the procces \"osu!\"!\n");

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

LPVOID FindPointerAddress(HANDLE pHandle, LPVOID baseAddress, int pLevel, LPINT offsets) {
	int address = reinterpret_cast<int>(baseAddress);

	for (int i = 0; i < pLevel; i++) {
		ReadProcessMemory(pHandle, (LPCVOID)address, &address, 4, NULL);
		address += offsets[i];
	}

	return reinterpret_cast<LPVOID>(address);
}

LPVOID GetAddress(HANDLE processHandle, LPVOID baseAddress, int pLevel, LPINT offsets) {
	return FindPointerAddress(processHandle, baseAddress, pLevel, offsets);
}

LPVOID GetTimeAddress() {
	int pLevel = 5;
	int threadOffset = -0x32C;
	LPINT offsets = new int[5] {
		0x50,
		0x73C,
		0x20,
		0x4F0,
		0x1EC
	};

	DWORD osuProcessID = GetProcessID("osu!.exe");
	osuProcessHandle = GetHandle(osuProcessID);

	LPVOID threadAddress = reinterpret_cast<LPVOID>((GetThreadStack(osuProcessHandle, ThreadList(osuProcessID)))[0] + threadOffset);

	return GetAddress(osuProcessHandle, threadAddress, pLevel, offsets);
}


void TimeThread() {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while (true) {
		ReadProcessMemory(osuProcessHandle, timeAddress, &songTime, 4, NULL);
		this_thread::sleep_for(chrono::microseconds(200));
	}
}


void SendKeyPress(HitObject *hitObject) {
	BPM = hitObject->getBPM() == 0.f ? BPM * 1.02f : hitObject->getBPM();

	if ((static_cast<float>(hitObject->getStartTime() - prevInputTime) > 125.f) || altKey == TRUE) {
		if (inputKeyBoard) {
			input.type = INPUT_KEYBOARD;
			input.ki.dwFlags = NULL;
			if (inputFlip)
				input.ki.wVk = inputAltKey;
			else
				input.ki.wVk = inputMainKey;
		}
		else {
			input.type = INPUT_MOUSE;
			if (inputFlip)
				input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
			else
				input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		}
		altKey = FALSE;
	}
	else {
		if (inputKeyBoard) {
			input.type = INPUT_KEYBOARD;
			input.ki.dwFlags = NULL;
			if (inputFlip)
				input.ki.wVk = inputMainKey;
			else
				input.ki.wVk = inputAltKey;
		}
		else {
			input.type = INPUT_MOUSE;
			if (inputFlip)
				input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			else
				input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		}
		altKey = TRUE;
	}

	SendInput(1, &input, sizeof(INPUT));
}

void SendKeyRelease(HitObject *hitObject) {
	if (inputKeyBoard) {
		input.type = INPUT_KEYBOARD;
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));
	}
	else {
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(1, &input, sizeof(INPUT));
		input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		SendInput(1, &input, sizeof(INPUT));
	}

	prevInputTime = hitObject->getStartTime();
	if (hitObject->getEndTime() > hitObject->getStartTime())
		prevInputTime = hitObject->getEndTime();
}


vector<vec2f> FindControlPoints(vec2f vec1, vec2f vec2, vec2f vec3) {
	vec2f
		d1(vec1.x - vec2.x, vec1.y - vec2.y),
		d2(vec2.x - vec3.x, vec2.y - vec3.y);

	float
		l1 = sqrtf(d1.x * d1.x + d1.y * d1.y),
		l2 = sqrtf(d2.x * d2.x + d2.y * d2.y);

	vec2f
		m1 = (vec1 + vec2) / 2.f,
		m2 = (vec2 + vec3) / 2.f;

	vec2f
		p0 = m2 + (vec2 - (m2 + (m1 - m2) * ((l2 * 1.85f) / (l1 + l2)))),
		p1 = m1 + (vec2 - (m2 + (m1 - m2) * ((l2 * ((atan2f(l2 / 480.f, 1.85f + (l2 / 960.f)) / 240.f) + 1.f)) / (l1 + l2))));

	return { p0, p1 };
}


void MoveToStandard(HitObject *hitObject) {
	GetCursorPos(&cursorPoint);

	pB = vec2f(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y));
	pE = vec2f((hitObject->getStartPos().x - stackOffset * hitObject->getStack()) * multiplierX + osuWindowX,
		(hitObject->getStartPos().y - stackOffset * hitObject->getStack())  * multiplierY + osuWindowY);

	float dt = static_cast<float>(hitObject->getStartTime() - songTime);

	while (songTime < hitObject->getStartTime() && songStarted) {
		float t = (dt - static_cast<float>(hitObject->getStartTime() - songTime)) / dt;
		pCursor = pB + t * (pE - pB);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));
		this_thread::sleep_for(chrono::microseconds(100));
	}

	if (hitObject->getHitType() == HIT_CIRCLE) {
		SendKeyPress(hitObject);
		this_thread::sleep_for(chrono::milliseconds(12));
		SendKeyRelease(hitObject);
	}

	nextObject = TRUE;
}

void MoveToFlowing(vector<HitObject>* hitObjects, const int nObject) {
	GetCursorPos(&cursorPoint);

	vec2f p0;

	if (nObject != 0) {
		p0 = vec2f((pE - pBack) + pE);
	}

	pP = nObject == 0 ? vec2f(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y)) : vec2f((hitObjects->at(nObject - 1).getEndPos().x - stackOffset * hitObjects->at(nObject - 1).getStack()) * multiplierX + osuWindowX,
		(hitObjects->at(nObject - 1).getEndPos().y - stackOffset * hitObjects->at(nObject - 1).getStack()) * multiplierY + osuWindowY);
	pB = vec2f(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y));
	pE = vec2f((hitObjects->at(nObject).getStartPos().x - stackOffset * hitObjects->at(nObject).getStack()) * multiplierX + osuWindowX,
		(hitObjects->at(nObject).getStartPos().y - stackOffset * hitObjects->at(nObject).getStack()) * multiplierY + osuWindowY);
	pN = nObject + 1 == static_cast<signed int>(hitObjects->size()) ? pE : vec2f((hitObjects->at(nObject + 1).getStartPos().x - stackOffset * hitObjects->at(nObject + 1).getStack()) * multiplierX + osuWindowX,
		(hitObjects->at(nObject + 1).getStartPos().y - stackOffset * hitObjects->at(nObject + 1).getStack()) * multiplierY + osuWindowY);

	if (UINT(nObject + 1) < hitObjects->size() && hitObjects->at(nObject + 1).getHitType() == HIT_SLIDER) {
		float tN = 1.f / ceilf(hitObjects->at(nObject + 1).getSliderTickCount());
		pN = vec2f(((hitObjects->at(nObject + 1).getPointByT(tN).x - hitObjects->at(nObject + 1).getStack() * stackOffset) * multiplierX) + osuWindowX,
			((hitObjects->at(nObject + 1).getPointByT(tN).y - hitObjects->at(nObject + 1).getStack() * stackOffset) * multiplierY) + osuWindowY);
	}

	if (nObject == 0)
		p0 = FindControlPoints(pP, pB, pE).at(0);



	if ((pN - pE).length() < (1.f / circleSize) * 400.f && (pE - pB).length() < (1.f / circleSize) * 400.f) {
		p0 = vec2f((pE - pBack).dev((pN - pE).length() * ((1.f / circleSize) * 400.f) + 1.f) + pE);
	}

	vec2f p1 = FindControlPoints(pB, pE, pN).at(1);

	vector<vec2f> pts {
		pB, p0, p1, pE
	};

	float dt = static_cast<float>(hitObjects->at(nObject).getStartTime() - songTime);
	while (songTime < hitObjects->at(nObject).getStartTime() && songStarted) {
		float t = (dt - static_cast<float>(hitObjects->at(nObject).getStartTime() - songTime)) / dt;

		pCursor = PolyBezier(pts, pts.size() - 1, 0, t);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		this_thread::sleep_for(chrono::microseconds(100));
	}

	if (hitObjects->at(nObject).getHitType() == HIT_CIRCLE) {
		SendKeyPress(&hitObjects->at(nObject));
		this_thread::sleep_for(chrono::milliseconds(12));
		SendKeyRelease(&hitObjects->at(nObject));
	}

	pBack = pts.at(pts.size() - 2);

	nextObject = TRUE;
}

void SliderStandard(HitObject *hitObject) {
	SendKeyPress(hitObject);

	while (songTime <= hitObject->getEndTime() && songStarted) {
		auto t = static_cast<float>(songTime - hitObject->getStartTime()) / hitObject->getSliderTime();

		vec2f vec = hitObject->getPointByT(t);
		pCursor = vec2f((((vec.x - hitObject->getStack() * stackOffset) * multiplierX) + osuWindowX),
			((vec.y - hitObject->getStack() * stackOffset) * multiplierY) + osuWindowY);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		this_thread::sleep_for(chrono::microseconds(100));
	}

	SendKeyRelease(hitObject);

	nextObject = TRUE;
}

void SliderFlowing(vector<HitObject> *hitObjects, const int nObject) {
	GetCursorPos(&cursorPoint);

	int cpCount = 3;

	int nPolyCount = 0;
	int nPolyCountReverse;
	float nPoly = 0.f;
	int nP = 0;
	bool T1 = false;
	vector<vec2f> pts;
	pts.resize(static_cast<unsigned int>(ceilf(hitObjects->at(nObject).getSliderTickCount()) * hitObjects->at(nObject).getSliderRepeatCount() * static_cast<float>(cpCount)) + (UINT)1);

	vec2f pPBack = pP;
	vec2f pNBack = pN;

	if (hitObjects->at(nObject).getSliderRepeatCount() == 1.f) {
		for (float i = 0.f; i < hitObjects->at(nObject).getSliderTickCount(); i++) {
			float tB = i / hitObjects->at(nObject).getSliderTickCount();
			float tE = (i + 1.f) > hitObjects->at(nObject).getSliderTickCount() ? 1.f : (i + 1.f) / hitObjects->at(nObject).getSliderTickCount();
			float tN = (i + 2.f) / hitObjects->at(nObject).getSliderTickCount();

			pN = pNBack;
			if (tN < 1.f) {
				pN = vec2f(((hitObjects->at(nObject).getPointByT(tN).x - hitObjects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
					((hitObjects->at(nObject).getPointByT(tN).y - hitObjects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);
			}

			if (nPolyCount != 0) {
				pts.at(nPolyCount + 1) = vec2f((pE - pts.at(nPolyCount - cpCount + 2)) + pE);
			}

			pE = vec2f(((hitObjects->at(nObject).getPointByT(tE).x - hitObjects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
				((hitObjects->at(nObject).getPointByT(tE).y - hitObjects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);

			if (nPolyCount == 0) {
				pB = vec2f(((hitObjects->at(nObject).getPointByT(tB).x - hitObjects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
					((hitObjects->at(nObject).getPointByT(tB).y - hitObjects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);
				pts.at(nPolyCount) = pB;

				//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
				pts.at(nPolyCount + 1) = vec2f((pts.at(nPolyCount) - pBack) + pts.at(nPolyCount));
			}

			//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
			pts.at(nPolyCount + 2) = FindControlPoints(pts.at(nPolyCount), pE, pN).at(1);
			pts.at(nPolyCount + cpCount) = pE;

			pP = pts.at(nPolyCount);
			nPolyCount += cpCount;
		}
	}
	else {
		for (int repeated = 0; static_cast<float>(repeated) < hitObjects->at(nObject).getSliderRepeatCount(); repeated++) {
			if (repeated % 2 == 0) {
				for (float i = 0.f; i < hitObjects->at(nObject).getSliderTickCount(); i++) {
					float tB = i / hitObjects->at(nObject).getSliderTickCount();
					float tE = (i + 1.f) > hitObjects->at(nObject).getSliderTickCount() ? 1.f : (i + 1.f) / hitObjects->at(nObject).getSliderTickCount();
					float tN = (i + 2.f) / hitObjects->at(nObject).getSliderTickCount();

					pN = pNBack;
					if (tN < hitObjects->at(nObject).getSliderRepeatCount()) {
						pN = vec2f(((hitObjects->at(nObject).getPointByT(tN).x - hitObjects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
							((hitObjects->at(nObject).getPointByT(tN).y - hitObjects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);
					}

					if (nPolyCount != 0) {
						pts.at(nPolyCount + 1) = vec2f((pE - pts.at(nPolyCount - cpCount + 2)) + pE);
					}

					pE = vec2f(((hitObjects->at(nObject).getPointByT(tE).x - hitObjects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
						((hitObjects->at(nObject).getPointByT(tE).y - hitObjects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);

					if (nPolyCount == 0) {
						pB = vec2f(((hitObjects->at(nObject).getPointByT(tB).x - hitObjects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
							((hitObjects->at(nObject).getPointByT(tB).y - hitObjects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);
						pts.at(nPolyCount) = pB;

						//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
						pts.at(nPolyCount + 1) = vec2f((pts.at(nPolyCount) - pBack) + pts.at(nPolyCount));
					}

					//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
					pts.at(nPolyCount + 2) = FindControlPoints(pts.at(nPolyCount), pE, pN).at(1);
					pts.at(nPolyCount + cpCount) = pE;

					pP = pts.at(nPolyCount);
					nPolyCount += cpCount;
				}
			}
			if (repeated % 2 != 0) {
				nPolyCountReverse = nPolyCount;
				for (float i = 0.f; i < hitObjects->at(nObject).getSliderTickCount(); i++) {
					nPolyCountReverse -= cpCount;

					pts.at(nPolyCount + 1) = vec2f((pE - pts.at(nPolyCount - cpCount + 2)) + pE);

					pP = pts.at(nPolyCount);
					pE = pts.at(nPolyCountReverse);

					if ((i + 1.f) / hitObjects->at(nObject).getSliderTickCount() >= hitObjects->at(nObject).getSliderRepeatCount()) {
						pN = pNBack;
					}
					else if (nPolyCountReverse - cpCount > 0) {
						pN = pts.at(nPolyCountReverse - cpCount);
					}
					else {
						pN = pPBack;
					}

					//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
					pts.at(nPolyCount + 2) = FindControlPoints(pts.at(nPolyCount), pE, pN).at(1);
					pts.at(nPolyCount + cpCount) = pE;

					pP = pts.at(nPolyCount);
					nPolyCount += cpCount;
				}
			}
		}
	}

	SendKeyPress(&hitObjects->at(nObject));

	while (songTime < hitObjects->at(nObject).getEndTime() && songStarted) {
		float t = static_cast<float>(songTime - hitObjects->at(nObject).getStartTime()) / static_cast<float>(hitObjects->at(nObject).getSliderTime());

		float floor = floorf(t);
		t = static_cast<int>(floor) % 2 == 0 ? t - floor : floor + 1.f - t;

		/*if (t >= 1 && !T1) {
			nPoly++;
			nP++;
			T1 = true;
		}
		if (t * hitObjects->at(nObject).getSliderTickCount() - nPoly > 1.f) {
			nPoly++;
			nP++;
		}
		if (nPoly > hitObjects->at(nObject).getSliderTickCount()) {
			nPoly = floorf(nP / hitObjects->at(nObject).getSliderTickCount()) * hitObjects->at(nObject).getSliderTickCount();
		}*/

		float T = t * hitObjects->at(nObject).getSliderTickCount() - nPoly;
		if (T > 1.f) {
			T = 1.f;
		}

		/* EventLog */	fprintf(wEventLog, ("[DEBUGGING]  t: " + to_string(t) + "\n             nPoly: " + to_string(nPoly) + "\n             nP: " + to_string(nP) + "\n             T: " + to_string(T) + "\n").c_str());

		pCursor = PolyBezier(pts, cpCount, nP, T);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		this_thread::sleep_for(chrono::microseconds(100));
	}

	SendKeyRelease(&hitObjects->at(nObject));

	pBack = pts.at(pts.size() - 2);
	pP = pts.at(pts.size() - 1 - cpCount);

	nextObject = TRUE;
}

void SpinnerStandard(HitObject *hitObject) {
	vec2f center(hitObject->getStartPos().x * multiplierX + osuWindowX,
		(hitObject->getStartPos().y + 6.f) * multiplierY + osuWindowY);
	float angle = TWO_PI;
	float radius = (1.f / circleSize) * 450.f;

	SendKeyPress(hitObject);

	while (songTime < hitObject->getEndTime() && songStarted) {
		float t = static_cast<float>(songTime - hitObject->getStartTime()) / static_cast<float>(hitObject->getEndTime() - hitObject->getStartTime());
		t *= 3.f; if (t >= 1.f) t = 1.f;
		float radiusT = radius * t;

		pCursor = CirclePoint(center, radiusT, angle);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		angle -= static_cast<float>(M_PI / 30.f);

		this_thread::sleep_for(chrono::microseconds(100));
	}

	SendKeyRelease(hitObject);

	nextObject = TRUE;
}


void AutoPlay(wstring nowPlaying) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	wstring beatmap(nowPlaying.begin() + 8, nowPlaying.end());

	if (statusText != L"No Beatmap Selected!")
		statusText = L"Now Playing: " + beatmap;
	DrawTextToWindow(hWnd, statusText, rectStatus);

	prevInputTime = songTime;
	SendMessage(hwndProgressBar, PBM_SETPOS, WPARAM(0), NULL);

	for (unsigned int nObject = 0; nObject < hitObjects.size(); nObject++) {
		HitObject hit = hitObjects.at(nObject);
		if (nextObject && songStarted) {
			nextObject = FALSE;

			// Movement calls:
			switch (modeMoveTo) {
			case MODE_STANDARD:
				MoveToStandard(&hit);
				break;

			case MODE_FLOWING:
				MoveToFlowing(&hitObjects, nObject);
				break;
			}

			if (hit.getHitType() == HIT_SLIDER) {
				switch (modeSlider) {
				case MODE_STANDARD:
					SliderStandard(&hit);
					break;

				case MODE_FLOWING:
					SliderFlowing(&hitObjects, nObject);
					break;
				}
			}
			else if (hit.getHitType() == HIT_SPINNER) {
				switch (modeSpinner) {
				case MODE_STANDARD:
					SpinnerStandard(&hit);
					break;

				case MODE_FLOWING:

					break;
				}
			}
			// Movement calls END;

			SendMessage(hwndProgressBar, PBM_SETPOS, WPARAM(nObject), NULL);
		}
		else {
			while (!nextObject | (!songStarted && !firstStart)) {
				this_thread::sleep_for(chrono::microseconds(50));
			}
			nObject -= (UINT)2;
		}
		this_thread::sleep_for(chrono::microseconds(100));
	}

	this_thread::sleep_for(chrono::milliseconds(1000));

	statusText = L"Waiting for user...";
	DrawTextToWindow(hWnd, statusText, rectStatus);

	/* EventLog */	fprintf(wEventLog, "[EVENT]  AutoPlay thread finished.\n");

	songStarted = FALSE;
}


float MapDifficultyRange(float difficulty, float min, float mid, float max) {
	if (difficulty > 5.0f) return mid + (max - mid)*(difficulty - 5.0f) / 5.0f;
	if (difficulty < 5.0f) return mid - (mid - min)*(5.0f - difficulty) / 5.0f;
	return mid;
}

void ParseSong(LPCTSTR songPath) {
	fstream path; path.open(songPath, fstream::in);
	bool general = false;
	bool difficulty = false;
	bool timing = false;
	bool hits = false;
	bool beatDivisor = false;
	while (path) {
		string str;
		getline(path, str);
		if (str.find("[General]") != string::npos) {
			general = true;
		}
		else if (general) {
			if (str.find("StackLeniency") != string::npos) {
				stackLeniency = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find(':') == string::npos) {
				general = false;
			}
		}
		else if (str.find("[Editor]") != string::npos) {
			beatDivisor = true;
		}
		else if (beatDivisor) {
			if (str.find("BeatDivisor") != string::npos) {
				beatMapDivider = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find(':') == string::npos) {
				beatDivisor = false;
			}
		}
		else if (str.find("[Difficulty]") != string::npos) {
			difficulty = true;
		}
		else if (difficulty) {
			if (str.find("OverallDifficulty:") != string::npos) {
				overallDifficulty = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find("CircleSize") != string::npos) {
				circleSize = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find("SliderMultiplier") != string::npos) {
				sliderMultiplier = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find("SliderTickRate") != string::npos) {
				sliderTickRate = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find(':') == string::npos) {
				difficulty = false;
			}
		}
		else if (str.find("[TimingPoints]") != string::npos) {
			timing = true;
		}
		else if (timing) {
			if (str.find(',') == string::npos) {
				timing = false;
			}
			else {
				TimingPoint TP = TimingPoint(str);
				timingPoints.push_back(TP);
			}
		}
		else if (str.find("[HitObjects]") != string::npos) {
			hits = true;
		}
		else if (hits) {
			if (str.find(',') == string::npos) {
				hits = false;
			}
			else {
				HitObject HO = HitObject(str, &timingPoints, sliderMultiplier, sliderTickRate);
				hitObjects.push_back(HO);
			}
		}
	}
	path.close();

	float preEmpt = MapDifficultyRange(overallDifficulty, 1800.0f, 1200.0f, 450.0f);

	stackOffset = ((512.0f / 16.0f) * (1.0f - 0.7f * (circleSize - 5.0f) / 5.0f) / 10.0f) / circleSize;

	for (int i = hitObjects.size() - 1; i > 0; i--) {
		HitObject* hitObjectI = &hitObjects[i];
		if (hitObjectI->getStack() != 0 || hitObjectI->getHitType() == HIT_SPINNER) {
			continue;
		}

		for (int n = i - 1; n >= 0; n--) {
			HitObject* hitObjectN = &hitObjects[n];
			if (hitObjectN->getHitType() == HIT_SPINNER) {
				continue;
			}

			// check if in range stack calculation
			float timeI = hitObjectI->startTime - preEmpt * stackLeniency;
			float timeN = float(hitObjectN->getHitType() == HIT_SLIDER ? hitObjects[n].endTime : hitObjectN->startTime);
			if (timeI > timeN)
				break;

			if (hitObjectN->getHitType() == HIT_SLIDER) {
				vec2f p1 = hitObjects[i].getStartPos();
				vec2f p2 = hitObjects[n].getEndPos();
				float distance = (p2 - p1).length();

				// check if hit object part of this stack
				if (stackLeniency > 0.0f) {
					float circleRadius = pow(circleSize, -1) * 200.0f;
					if (distance < circleRadius) {
						int offset = hitObjectI->getStack() - hitObjectN->getStack() + 1;
						for (int j = n + 1; j <= i; j++) {
							HitObject* hitObjectJ = &hitObjects[j];
							p1 = hitObjectJ->getStartPos();
							distance = (p2 - p1).length();
							//cout << offset;
							// hit object below slider end
							if (distance < circleRadius)
								hitObjectJ->setStack(hitObjectJ->getStack() - offset);
						}
						break;  // slider end always start of the stack: reset calculation
					}
				}
				else {
					break;
				}
			}
			auto distance = (
				hitObjectN->getStartPos() -
				hitObjectI->getStartPos()
				).length();
			if (distance < stackLeniency) {
				hitObjectN->setStack(hitObjectI->getStack() + 1);
				hitObjectI = hitObjectN;
			}
		}
	}
	for (int i = hitObjects.size() - 1; i > 0; i--) {
		int n = i;
		/* We should check every note which has not yet got a stack.
		* Consider the case we have two interwound stacks and this will make sense.
		*
		* o <-1      o <-2
		*  o <-3      o <-4
		*
		* We first process starting from 4 and handle 2,
		* then we come backwards on the i loop iteration until we reach 3 and handle 1.
		* 2 and 1 will be ignored in the i loop because they already have a stack value.
		*/

		HitObject *objectI = &hitObjects[i];

		if (objectI->stackId != 0 || objectI->getHitType() == HIT_SPINNER) continue;

		/* If this object is a hitcircle, then we enter this "special" case.
		* It either ends with a stack of hitcircles only, or a stack of hitcircles that are underneath a slider.
		* Any other case is handled by the "is Slider" code below this.
		*/
		if (objectI->endTime == 0) {
			while (--n >= 0) {
				HitObject* objectN = &hitObjects[n];

				if (objectN->getHitType() == HIT_SPINNER) continue;

				//HitObjectSpannable spanN = objectN as HitObjectSpannable;
				float timeI = objectI->startTime - preEmpt * stackLeniency;
				float timeN = static_cast<float>(objectN->getHitType() == HIT_SLIDER ? objectN->endTime : objectN->startTime);
				if (timeI > timeN)
					break;

				/* This is a special case where hticircles are moved DOWN and RIGHT (negative stacking) if they are under the *last* slider in a stacked pattern.
				*    o==o <- slider is at original location
				*        o <- hitCircle has stack of -1
				*         o <- hitCircle has stack of -2
				*/
				if (objectN->endTime != 0 && (objectI->getEndPos() - objectN->startPosition).length() < STACK_LENIENCE) {
					int offset = objectI->stackId - objectN->stackId + 1;
					for (int j = n + 1; j <= i; j++) {
						//For each object which was declared under this slider, we will offset it to appear *below* the slider end (rather than above).
						if ((hitObjects[j].startPosition - objectN->getEndPos()).length() < STACK_LENIENCE)
							hitObjects[j].stackId -= offset;
					}

					//We have hit a slider.  We should restart calculation using this as the new base.
					//Breaking here will mean that the slider still has StackCount of 0, so will be handled in the i-outer-loop.
					break;
				}

				if ((objectI->startPosition - objectN->startPosition).length() < STACK_LENIENCE) {
					//Keep processing as if there are no sliders.  If we come across a slider, this gets cancelled out.
					//NOTE: Sliders with start positions stacking are a special case that is also handled here.

					objectN->stackId = objectI->stackId + 1;
					objectI = objectN;
				}
			}
		}
		else if (objectI->getHitType() == HIT_SLIDER) {
			/* We have hit the first slider in a possible stack.
			* From this point on, we ALWAYS stack positive regardless.
			*/
			while (--n >= 0) {
				HitObject* objectN = &hitObjects[n];

				if (objectN->getHitType() == HIT_SPINNER) continue;

				//HitObjectSpannable spanN = objectN as HitObjectSpannable;

				if (objectI->startTime - (preEmpt * stackLeniency) > objectN->startTime)
					break;

				if ((objectI->startPosition - (objectN->endTime != 0 ? objectN->getEndPos() : objectN->startPosition)).length() < STACK_LENIENCE) {
					objectN->stackId = objectI->stackId + 1;
					objectI = objectN;
				}
			}
		}
	}

	if (path)
		/* EventLog */	fprintf(wEventLog, "[EVENT]  Beatmap parsed without major errors.\n");
}

bool OpenSongAuto(wstring title) {
	char charsToRemove[] = { "?.\"" };
	char charsToRemoveDiff[] = { "?<>" };
	string displayTitle;
	string beatmapName;
	string difficultyName;
	string beatmap;
	vector<string> beatmapSets;
	int beatmapSetCount = 0;


	displayTitle.assign(title.begin(), title.end());

	if (displayTitle.find("[") != string::npos) {
		difficultyName = displayTitle.substr(displayTitle.find_last_of("["));
	}

	beatmapName = displayTitle.substr(displayTitle.find(displayTitle.at(8)), displayTitle.find_last_of("[") - 9);

	for (unsigned int i = 0; i < beatmapName.size() - 1; i++) {
		switch (beatmapName.at(i)) {
		case '<':
			beatmapName.at(i) = '-';
		case '>':
			beatmapName.at(i) = '-';
		case '*':
			beatmapName.at(i) = '_';
		case ':':
			beatmapName.at(i) = '_';
		}
	}

	for (unsigned int i = 0; i < strlen(charsToRemove); i++) {
		beatmapName.erase(remove(beatmapName.begin(), beatmapName.end(), charsToRemove[i]), beatmapName.end());
	}

	for (unsigned int i = 0; i < difficultyName.size() - 1; i++) {
		switch (difficultyName.at(i)) {
		case '*':
			difficultyName.at(i) = '_';
		case ':':
			difficultyName.at(i) = '_';
		}
	}

	for (unsigned int i = 0; i < strlen(charsToRemoveDiff); i++) {
		difficultyName.erase(remove(difficultyName.begin(), difficultyName.end(), charsToRemoveDiff[i]), difficultyName.end());
	}

	difficultyName += ".osu";


	DeleteFile(L"Data\\BMData.txt");
	/* EventLog */	fprintf(wEventLog, "[EVENT]  Cleared BMData from previous writing.\n");

	/* EventLog */	fprintf(wEventLog, "[EVENT]  Starting SFData reading...\n");

	fstream rSFData; rSFData.open("Data\\SFData.txt", fstream::in);

	string readLine;
	while (readLine != "[Songs Folders]") {
		getline(rSFData, readLine);
	}

	while (rSFData) {
		getline(rSFData, readLine);
		if (readLine.find(beatmapName) != string::npos) {
			/* EventLog */	fprintf(wEventLog, "[EVENT]  Starting BMData writing...\n");

			fstream wBMData; wBMData.open("Data\\BMData.txt", fstream::out | fstream::app);
			wBMData << "[BeatmapDifficulties]\n";

			string directory = string(songsPath.begin(), songsPath.end()) + "\\" + readLine;

			for (auto beatmapFile : experimental::filesystem::directory_iterator(directory)) {
				stringstream sstream; sstream << beatmapFile;

				string beatmapString = sstream.str();
				wBMData << string((beatmapString.begin() + directory.size() + 1), beatmapString.end()) << endl;
			}

			beatmapSets.push_back(directory);

			wBMData << "\n";

			wBMData.close();

			/* EventLog */	fprintf(wEventLog, "[EVENT]    Finished BMData writing.\n");
		}
	}
	rSFData.close();
	readLine.clear();

	/* EventLog */	fprintf(wEventLog, "[EVENT]    Finished SFData reading.\n");
	/* EventLog */	fprintf(wEventLog, "[EVENT]  Starting BMData reading...\n");

	fstream rBMData; rBMData.open("Data\\BMData.txt", fstream::in);

	getline(rBMData, readLine);

	while (rBMData) {
		getline(rBMData, readLine);
		if (readLine == "[BeatmapDifficulties]") {
			getline(rBMData, readLine);
			beatmapSetCount++;
		}

		if (readLine.find(difficultyName) != string::npos) {
			string beatmapConvert = beatmapSets.at(beatmapSetCount) + "\\" + readLine;
			beatmapPath = wstring(beatmapConvert.begin(), beatmapConvert.end());
			displayBeatmapPath = wstring(readLine.begin(), readLine.end());

			rBMData.close();

			return TRUE;
		}
	}
	rBMData.close();

	/* EventLog */	fprintf(wEventLog, "[EVENT]    Finished BMData reading.\n");

	return FALSE;
}

bool OpenSongManual() {
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd2)))) {
		LPWSTR pathName = NULL; if (CoTaskMemAlloc(NULL) == NULL) return FALSE;
		DWORD dwOptions2;
		COMDLG_FILTERSPEC rgFilterSpec[] = { { L"osu! Beatmap", L"*.osu" } };
		pfd2->GetOptions(&dwOptions2);
		pfd2->SetOptions(dwOptions2 | FOS_STRICTFILETYPES);
		pfd2->SetFileTypes(UINT(1), rgFilterSpec);
		pfd2->Show(NULL);
		try {
			if (SUCCEEDED(pfd2->GetResult(&psi2))) {
				if (psi2->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pathName) == S_OK) { beatmapPath = pathName; }
				else beatmapPath.clear();
				psi2->Release();
			}
			else beatmapPath.clear();
		}
		catch (...) {}
		pfd2->Release();
		CoTaskMemFree(pathName);
		CoUninitialize();
	}

	if (!beatmapPath.empty()) {
		displayBeatmapPath = beatmapPath.substr(beatmapPath.find_last_of('\\') + 1);
		return TRUE;
	}
	else return FALSE;
}

bool OpenSongFolder() {
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
		LPWSTR pathName = NULL; if (CoTaskMemAlloc(NULL) == NULL) return FALSE;
		DWORD dwOptions;
		pfd->GetOptions(&dwOptions);
		pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
		pfd->Show(NULL);
		if (SUCCEEDED(pfd->GetResult(&psi))) {
			if (psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pathName) == S_OK) {
				songsPath = pathName;

				fstream wSFData; wSFData.open("Data\\SFData.txt", fstream::out);
				wSFData << "[osu! Songs directory]\n" << string(songsPath.begin(), songsPath.end()) << "\n\n[Songs Folders]\n";

				for (auto beatmapFolder : experimental::filesystem::directory_iterator(pathName)) {
					stringstream sstream; sstream << beatmapFolder;

					string beatmapString = sstream.str();
					wSFData << string((beatmapString.begin() + songsPath.size() + 1), beatmapString.end()) << endl;
				}
				wSFData.close();
			}
			else songsPath.clear();
			psi->Release();
		}
		else songsPath.clear();
		pfd->Release();
		CoTaskMemFree(pathName);
		CoUninitialize();
	} fstream rSFData; rSFData.open("Data\\SFData.txt", fstream::in);
	if (!songsPath.empty()) { rSFData.close(); return TRUE; }
	else if (rSFData && songsPath.empty()) {
		string getLine;
		getline(rSFData, getLine); getline(rSFData, getLine);
		songsPath.assign(getLine.begin(), getLine.end());
		rSFData.close(); return TRUE;
	}
	else return FALSE;
}

void SongFileCheck(bool songFileCheck, string selectedBy) {
	if (songFileCheck) {
		DrawTextToWindow(hWnd, displayBeatmapPath.c_str(), rectSongFile);

		statusText = L"Beatmap Successfully Selected!";
		DrawTextToWindow(hWnd, statusText, rectStatus);

		/* EventLog */	fprintf(wEventLog, ("[EVENT]  Beatmap successfully selected by " + selectedBy).c_str());

		hitObjects.clear();
		timingPoints.clear();

		ParseSong((beatmapPath.c_str()));

		SendMessage(hwndProgressBar, PBM_SETRANGE, NULL, MAKELPARAM(0, hitObjects.size() + 1));
		SendMessage(hwndProgressBar, PBM_SETSTEP, WPARAM(1), NULL);
		SendMessage(hwndProgressBar, PBM_SETPOS, WPARAM(0), NULL);
	}
	else {
		statusText = L"No Beatmap Selected!";
		DrawTextToWindow(hWnd, statusText, rectStatus);

		/* EventLog */	fprintf(wEventLog, ("[EVENT]  No Beatmap was selected by " + selectedBy).c_str());
	}
}

void GameActiveChecker() {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	while (true) {
		RECT rect;
		POINT w = { 0, 8 };
		GetClientRect(osuWindow, &rect);
		ClientToScreen(osuWindow, &w);

		int x = min(rect.right, GetSystemMetrics(SM_CXSCREEN)), swidth = x,
			y = min(rect.bottom, GetSystemMetrics(SM_CXSCREEN)), sheight = y;

		if (swidth * 3 > sheight * 4) swidth = sheight * 4 / 3;
		else sheight = swidth * 3 / 4;
		multiplierX = swidth / 640.f;
		multiplierY = sheight / 480.f;

		int xOffset = static_cast<int>(x - 512.f * multiplierX) / 2,
			yOffset = static_cast<int>(y - 384.f * multiplierY) / 2;

		osuWindowX = w.x + xOffset;
		osuWindowY = w.y + yOffset;

		TCHAR titleC[MAXCHAR];
		GetWindowTextW(osuWindow, (LPWSTR)titleC, MAXCHAR);
		wstring title(titleC);

		if (title != L"osu!" && title != L"" && pathSet) {
			if (firstStart) {
				songStarted = TRUE;

				if (autoOpenSong) {
					SongFileCheck(OpenSongAuto(title), autoSelect);
				}

				/* EventLog */	fprintf(wEventLog, "[EVENT]  Starting AutoPlay thread!\n");

				thread AutoThread(AutoPlay, title);
				AutoThread.detach();

				firstStart = FALSE;
			}
			else if (songTime > 0) {
				if (songTime == prevTime) {
					songStarted = FALSE;
				}
				else {
					songStarted = TRUE;
				}
			} prevTime = songTime;
		}
		else if (title == L"osu!" && songStarted) {
			firstStart = TRUE;
			songStarted = FALSE;
		}
		else if (title == L"osu!" && !songStarted) {
			firstStart = TRUE;
		}
		else if (title == L"") {
			if (!songStarted) {
				if (true) /*if Option to close Osu!Bot on osu! exit.*/
				{
					TerminateProcess(GetCurrentProcess(), 0);
				}
			}
		}
		this_thread::sleep_for(chrono::milliseconds(5));
	}
}

void FindGame() {
	osuWindow = FindWindowA(NULL, "osu!");
	if (osuWindow == NULL) {
		/* EventLog */	fprintf(wEventLog, "[WARNING]  The procces \"osu!\" was not found!\n");

		statusText = L"\"osu!\" NOT found!   Please start \"osu!\"...";
		DrawTextToWindow(hWnd, statusText, rectStatus);

		while (osuWindow == NULL) {
			osuWindow = FindWindowA(NULL, "osu!");
			Sleep(500);
		}
	}

	/* EventLog */	fprintf(wEventLog, "[EVENT]  osu!.exe FOUND!\n");

	timeAddress = GetTimeAddress();

	if (timeAddress == reinterpret_cast<LPVOID>(0xCCCCCCCC)
		|| timeAddress == reinterpret_cast<LPVOID>(0x0)) {
		CloseHandle(osuProcessHandle);
		/* EventLog */	fprintf(wEventLog, "[WARNING]  timeAddres NOT FOUND!\n");

		statusText = L"timeAddress NOT found!   Please start \"osu!\" BEFORE starting \"Osu!Bot\"!";
		DrawTextToWindow(hWnd, statusText, rectStatus);

		FindGame();
	}

	stringstream timeAddressString;
	timeAddressString << "0x" << hex << (UINT)timeAddress;

	/* EventLog */	fprintf(wEventLog, ("[EVENT]  \"timeAddres\" FOUND!  Starting Checker and Time threads!\n           timeAddres: " + timeAddressString.str() + "\n").c_str());

	statusText = L"Waiting for user...";
	DrawTextToWindow(hWnd, statusText, rectStatus);

	thread gameActiveThread(GameActiveChecker);
	gameActiveThread.detach();
	thread timeThread(TimeThread);
	timeThread.detach();
}
