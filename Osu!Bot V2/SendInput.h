#pragma once

#include "stdafx.h"

#include "OsuBot.h"


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
