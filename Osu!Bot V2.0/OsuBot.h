#pragma once

#include "stdafx.h"

#include "GlobalVariables.h"
#include "timeAddress.h"
#include "SendInput.h"
#include "CursorMovement.h"
#include "SongsReading.h"


using namespace std;


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
