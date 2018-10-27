#pragma once


#define MIN(X, Y) X < Y ? X : Y
#define CLAMP(minValue, value, maxValue) value > maxValue ? maxValue : value < minValue ? minValue : value
#define TWO_PI static_cast<float>(M_PI * 2.f)
#define HALF_PI static_cast<float>(M_PI / 2.f)

#define HIT_SLIDER 2
#define HIT_SPINNER 8
#define HIT_CIRCLE 1

#define STACK_LENIENCE 3

#define MODE_NONE 0
#define MODE_STANDARD 1
#define MODE_FLOWING 2
#define MODE_PREDICTING 3


#include "stdafx.h"

#include "vec2f.h"
#include "HitObject.h"

using namespace std;


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

WORD inputMainKey = 45;
WORD inputAltKey = 44;

vector<int> offsets(5);

int
nHeight = 290,
nWidth = 585;

int
objectNumber = 0,
trackBarPos = 100;

int threadOffset = 0x0;

int osuWindowX, osuWindowY;
int	songTime, prevTime, prevInputTime;

int
modeMoveTo = MODE_NONE,
modeSlider = MODE_NONE,
modeSpinner = MODE_NONE;

float
multiplierX, multiplierY,
stackLeniency,
beatMapDivider,
overallDifficulty,
circleSize,
sliderMultiplier,
sliderTickRate,
stackOffset,
Amplifier = 1.f;

bool
songStarted,
pathSet,
autoOpenSong = true,
hardrockFlip,
altKey,
inputFlip = true,
inputKeyBoard,
firstStart = true;


wstring userSelect = L"user.\n";
wstring autoSelect = L"Osu!Bot.\n";

extern wstring songsPath;
extern wstring statusText;
extern wstring songFileText;
extern HWND hWnd;
extern HWND hwndButtonOpenSongFile;
extern WORD inputMainKey;
extern WORD inputAltKey;
extern RECT rectStatus;
extern RECT rectSongFile;
#pragma endregion
