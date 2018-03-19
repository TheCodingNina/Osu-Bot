#pragma once


#define STACK_LENIENCE 3

#define MODE_STANDARD 2001
#define MODE_FLOWING 2002


#include "stdafx.h"


#pragma region GlobalVariables
FILE *wEventLog;

vector<TimingPoint> timingPoints;
vector<HitObject> hitObjects;

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


string userSelect = "user.\n";
string autoSelect = "Osu!Bot.\n";

extern wstring songsPath;
extern wstring statusText;
extern wstring songFileText;
extern HWND hWnd;
extern HWND hwndButtonOpenSongFile;
extern WORD inputMainKey;
extern WORD inputAltKey;
extern RECT rectStatus;
extern RECT rectSongFile;
extern int nHeight;
extern int nWidth;
#pragma endregion
