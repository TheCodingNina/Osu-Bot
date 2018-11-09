#pragma once


#define MIN(X, Y) X < Y ? X : Y
#define MAX(X, Y) X > Y ? X : Y
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
vector<LONG> offsets(5);

wstring
beatmapPath,
displayBeatmapPath,
songsPath,
statusText = L"Start up...",
songsFolderText = L"Select \"osu!\" Songs Folder.",
songFileText = L"Select an \"osu! beatmap\"",
userSelect = L"user.\n",
autoSelect = L"Osu!Bot.\n";

HWND
hWnd,
osuWindow,
hwndButtonOpenSongFolder,
hwndButtonOpenSongFile,
hwndCheckBoxAutoOpenSong,
hwndCheckBoxHardrockFlip,
hwndTrackBarDanceAmplifier,
hwndComboBoxDanceModeMoveTo,
hwndComboBoxDanceModeSlider,
hwndComboBoxDanceModeSpinner,
hwndProgressBar;

HANDLE osuProcessHandle;
LPVOID timeAddress;

IFileDialog *pfd, *pfd2;
IShellItem *psi, *psi2;

vec2f pBack;
vec2f pCursor, pP, pB, pE, pN;
POINT cursorPoint;

INPUT input;

WCHAR inputMainKey = 45;
WCHAR inputAltKey = 44;

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
inputKeyBoard = true,
firstStart = true;

RECT rectSongsFolder = { 10, 10, nWidth - 140, 50 };
RECT rectSongFile = { 10, 80, nWidth - 140, 120 };
RECT rectStatus = { 15, nHeight - 65, nWidth - 30, rectStatus.top + 18 };

#pragma endregion
