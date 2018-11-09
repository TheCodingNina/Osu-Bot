// OsuBot.cpp : Defines the entry point for the application.
//

#define _CRT_SECURE_NO_WARNINGS

#define MAX_LOADSTRING 100
#define BTN_ButtonOpenSongFolder 3001
#define BTN_ButtonOpenSongFile 3002
#define CB_CheckBoxAutoOpenSong 3003
#define CB_CheckBoxHardrockFlip 3004
#define TB_TrackBarDanceAmplifier 3005
#define CB_ComboBoxDanceModeMoveTo 3006
#define CB_ComboBoxDanceModeSlider 3007
#define CB_ComboBoxDanceModeSpinner 3008

#include "stdafx.h"

#include "GlobalVariables.h"
#include "DrawTextToWindow.h"
#include "ConfigurationFile.h"
#include "OsuBot.h"

using namespace std;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


// User Global Variables:
bool songFileCheck;

// Standard Variables for UI:
RECT rectOsuBotWindow;
RECT rectDanceAmplifier = { 15, 150, 200, 170 };
RECT rectDanceModeMoveTo = { 210, 150, 320, 175 };
RECT rectDanceModeSlider = { 330, 150, 440, 175 };
RECT rectDanceModeSpinner = { 450, 150, 560, 175 };
// User Global Variables END;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Settings(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	ErrorBox(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place user added startup code here.
	CreateDirectory(LPCTSTR(L"Data"), NULL);
	CreateDirectory(LPCTSTR(L"Data\\Logs"), NULL);


	FILE* rEventLog = _wfopen(L"Data\\Logs\\Events.log", L"r");
	if (rEventLog != NULL) {
		TCHAR buff[MAX_LOADSTRING];
		fgetws(buff, MAX_LOADSTRING, rEventLog);

		wstring timestamp(buff);
		timestamp.assign(timestamp.begin() + 22U, timestamp.end() - 1U);

		for (unsigned int i = 0; i < timestamp.size() - 1; i++)
			if (timestamp.at(i) == L':')
				timestamp.at(i) = L'.';

		wstring backupLog = (L"Data\\Logs\\Events (" + timestamp + L").log");

		FILE* bEventLog = _wfopen(&backupLog[0], L"w");

		fputws(buff, bEventLog);
		while (!feof(rEventLog))
			if (fgetws(buff, MAX_LOADSTRING, rEventLog) != NULL)
				fputws(buff, bEventLog);
		fflush(bEventLog);
		fclose(bEventLog);
	}

	wEventLog = _wfopen(L"Data\\Logs\\Events.log", L"w");
	if (wEventLog == NULL) { DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox); }

	time_t timeStamp = time(nullptr);
	wstring timeString = _wasctime(localtime(&timeStamp));
	timeString.erase(timeString.end() - 1, timeString.end());
	wstring logString = L"Events.log created at " + timeString;

	/* EventLog */	fwprintf(wEventLog, (logString + L"\n").c_str());
	fflush(wEventLog);


	if (_wfopen(L"Data\\configFile.cfg", L"r")) {
		/* EventLog */	fwprintf(wEventLog, L"[EVENT]  ConfigFile found.\n");

		ReadAllConfigSettings();
	}
	else {
		/* EventLog */	fwprintf(wEventLog, L"[WARNING]  ConfigFile not found!\n");

		int configMB = MessageBox(hWnd,
			L"No configFile was found!\nDo you want to generate a new configFile?\n\nIf this doesn't work try manualy creating an empty file named \"configFile.cfg\" under the \"Data\" folder.",
			L"ConfigFile not found!",
			MB_ICONWARNING | MB_YESNO | MB_APPLMODAL);

		if (configMB == IDYES) {
			CreateNewConfigFile();
			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  ConfigFile generated successful.\n");

			ReadAllConfigSettings();
		}
		else
			/* EventLog */	fwprintf(wEventLog, L"[WARNING]  ConfigFile was not auto generated.\n");
	}
	fflush(wEventLog);


	thread findGameThread(FindGame);
	findGameThread.detach();
	// TODO END;

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_OSUBOTV2, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OSUBOTV2));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OSUBOTV2));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_OSUBOTV2);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX |
		CS_VREDRAW | CS_HREDRAW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		nWidth, nHeight,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 600;
		lpMMI->ptMinTrackSize.y = 350;
		break;
	}
	case WM_CTLCOLORSTATIC:
	{
		SetBkColor((HDC)wParam, RGB(255, 255, 255));
		return (LRESULT)GetStockObject(WHITE_BRUSH);
	}
	case WM_HSCROLL:
	{
		switch (LOWORD(wParam)) {
		case TB_ENDTRACK:
			ULONG dwPos = (ULONG)SendMessage(hwndTrackBarDanceAmplifier, TBM_GETPOS, NULL, NULL);

			trackBarPos = (INT)dwPos;
			Amplifier = static_cast<float>(dwPos) / 80.f;

			UpdateConfigFile(danceSettings);
		}
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId) {
		case BTN_ButtonOpenSongFolder:
		{
			SendMessage(hwndButtonOpenSongFolder, WM_SETTEXT, 0, LPARAM(L"..."));

			statusText = L"Selecting Songs Folder...";
			DrawTextToWindow(hwnd, statusText, rectStatus);

			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  User selecting/changing Songs Folder.\n");
			fflush(wEventLog);

			pathSet = OpenSongFolder();

			if (!pathSet) {
				statusText = L"No Songs Folder Selected!";
				DrawTextToWindow(hwnd, statusText, rectStatus);

				/* EventLog */	fwprintf(wEventLog, L"[EVENT]  No Songs folder Selected.\n");
			}
			else {
				DrawTextToWindow(hwnd, songsPath.c_str(), rectSongsFolder);
				statusText = L"Songs Folder Successfully Selected!";
				DrawTextToWindow(hwnd, statusText, rectStatus);

				/* EventLog */	fwprintf(wEventLog, L"[EVENT]  Songs Folder Successfully Selected.\n           or not changed.\n");

				UpdateConfigFile(songsFolderPath);
			}
			fflush(wEventLog);

			SendMessage(hwndButtonOpenSongFolder, WM_SETTEXT, 0, ((pathSet) ? (LPARAM(L"Change")) : (LPARAM(L"Select"))));
			break;
		}
		case BTN_ButtonOpenSongFile:
		{
			SendMessage(hwndButtonOpenSongFile, WM_SETTEXT, 0, LPARAM(L"..."));

			statusText = L"Selecting a Beatmap...";
			DrawTextToWindow(hwnd, statusText, rectStatus);

			songFileCheck = OpenSongManual();
			SongFileCheck(songFileCheck, userSelect);

			SendMessage(hwndButtonOpenSongFile, WM_SETTEXT, 0, ((songFileCheck) ? (LPARAM(L"Change")) : (LPARAM(L"Select"))));
			break;
		}
		case CB_CheckBoxAutoOpenSong:
		{
			if (autoOpenSong)
				autoOpenSong = FALSE;
			else autoOpenSong = TRUE;

			wstring autoState = (autoOpenSong ? L"Enabled" : L"Disabled");
			/* EventLog */	fwprintf(wEventLog, (L"[EVENT]  Auto opening of beatmap: " + autoState + L"\n").c_str());
			fflush(wEventLog);

			if (autoOpenSong && !pathSet) {
				statusText = L"Please select \"osu!\" Songs Folder for Osu!Bot to autosearch in for the beatmaps!";
				DrawTextToWindow(hwnd, statusText, rectStatus);
			}

			break;
		}
		case CB_CheckBoxHardrockFlip:
		{
			if (hardrockFlip)
				hardrockFlip = FALSE;
			else hardrockFlip = TRUE;

			wstring hardrockState = (hardrockFlip ? L"Enabled" : L"Disabled");
			/* EventLog */	fwprintf(wEventLog, (L"[EVENT]  Hardrock mod: " + hardrockState + L"\n").c_str());
			fflush(wEventLog);

			break;
		}
		case CB_ComboBoxDanceModeMoveTo:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				int index = (INT)SendMessage(hwndComboBoxDanceModeMoveTo, CB_GETCURSEL, NULL, NULL);
				
				modeMoveTo = index;
				if (index < MODE_STANDARD || index > MODE_PREDICTING) {
					modeMoveTo = MODE_NONE;
				}
			}
			UpdateConfigFile(danceSettings);
			break;
		}
		case CB_ComboBoxDanceModeSlider:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				int index = (INT)SendMessage(hwndComboBoxDanceModeSlider, CB_GETCURSEL, NULL, NULL);

				modeSlider = index;
				if (index < MODE_STANDARD || index > MODE_PREDICTING) {
					modeSlider = MODE_NONE;
				}
			}
			UpdateConfigFile(danceSettings); 
			break;
		}
		case CB_ComboBoxDanceModeSpinner:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				int index = (INT)SendMessage(hwndComboBoxDanceModeSpinner, CB_GETCURSEL, NULL, NULL);
				
				modeSpinner = index;
				if (index < MODE_STANDARD || index > MODE_PREDICTING) {
					modeSpinner = MODE_NONE;
				}
			}
			UpdateConfigFile(danceSettings);
			break;
		}
		case ID_DATAFILES_OPENDATAFOLDER:
		{
			if (PtrToLong(ShellExecute(NULL, LPCTSTR(L"explore"), LPCTSTR(L"Data"), NULL, NULL, SW_SHOW)) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hwnd, ErrorBox);

			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  User Opening \"Data\" Folder.\n");
			fflush(wEventLog);
			break;
		}
		case ID_DATAFILES_OPENSONGDATA:
		{
			if (PtrToLong(ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"SFData.txt"), NULL, LPCTSTR(L"Data"), SW_SHOW)) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hwnd, ErrorBox);

			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  User Opening \"SFData.txt\".\n");
			fflush(wEventLog);
			break;
		}
		case ID_DATAFILES_OPENBEATMAPDATA:
		{
			if (PtrToLong(ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"BMData.txt"), NULL, LPCTSTR(L"Data"), SW_SHOW)) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hwnd, ErrorBox);

			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  User Opening \"BMData.txt\".\n");
			fflush(wEventLog);
			break;
		}
		case ID_DATAFILES_OPENEVENTLOG:
		{
			if (PtrToLong(ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"Events.log"), NULL, LPCTSTR(L"Data\\Logs"), SW_SHOW)) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hwnd, ErrorBox);

			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  User Opening \"Events.log\".\n");
			fflush(wEventLog);
			break;
		}
		case IDM_CLEARDATA:
		{
			int clearDataMB = MessageBox(hwnd,
				L"Osu!Bot will clear/clean up all collected data (logs).\nNOTE: this will clear ALL logs (The config file will not be removed!)\n\nAre you sure you want to delete the data files?",
				L"Delete Osu!Bot data files?",
				MB_ICONWARNING | MB_YESNO | MB_APPLMODAL);
			if (clearDataMB == IDNO)
				break;

			DeleteFile(L"Data\\SFData.txt");
			DeleteFile(L"Data\\BMData.txt");
			DeleteFile(L"Data\\Logs");

			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  User cleared data Files.\n");
			fflush(wEventLog);

			SendMessage(hwndButtonOpenSongFolder, WM_SETTEXT, 0, LPARAM(L"Select"));
			SendMessage(hwndButtonOpenSongFile, WM_SETTEXT, 0, LPARAM(L"Select"));

			DrawTextToWindow(hwnd, songsFolderText, rectSongsFolder);
			DrawTextToWindow(hwnd, songFileText, rectSongFile);
			break;
		}
		case IDM_SETTINGS:
			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  User Opened Settings.\n");
			DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGSBOX), hwnd, Settings);
			fflush(wEventLog);
			break;

		case IDM_ABOUT:
			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  User opened about box.\n");
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
			fflush(wEventLog);
			break;

		case IDM_EXIT:
			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  User Exited the program.\n");
			DestroyWindow(hwnd);
			fflush(wEventLog);
			break;

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
		} break;
	}
	case WM_PAINT:
	{
		// Dance Settings Strings
		LPCTSTR& modeNone = (LPCTSTR&)L"None";
		LPCTSTR& modeStandard = (LPCTSTR&)L"Standard";
		LPCTSTR& modeFlowing = (LPCTSTR&)L"Flowing";
		LPCTSTR& modePredicting = (LPCTSTR&)L"Predicting";
		
		// Clean-up before re-draw hwnds
		DestroyWindow(hwndButtonOpenSongFile);
		DestroyWindow(hwndButtonOpenSongFolder);
		DestroyWindow(hwndTrackBarDanceAmplifier);
		DestroyWindow(hwndProgressBar);


		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		GetClientRect(hwnd, &rectOsuBotWindow);
		nHeight = rectOsuBotWindow.bottom > 290 ? rectOsuBotWindow.bottom : 290;
		nWidth = rectOsuBotWindow.right > 585 ? rectOsuBotWindow.right : 585;

		rectSongsFolder = { 10, 10, nWidth - 140, 50 };
		rectSongFile = { 10, 80, nWidth - 140, 120 };
		rectStatus = { 15, nHeight - 65, nWidth - 30, rectStatus.top + 18 };


		// TODO: Add any drawing code that uses hdc here...
		// Status
		DrawTextToWindow(
			hdc,
			statusText,
			rectStatus
		);

		// Text
		DrawTextToWindow(
			hdc,
			pathSet ? songsPath : songsFolderText,
			rectSongsFolder
		);

		DrawTextToWindow(
			hdc,
			songFileCheck ? displayBeatmapPath : songFileText,
			rectSongFile
		);

		DrawTextToWindow(
			hdc,
			L"Dance Amplifier",
			rectDanceAmplifier
		);

		DrawTextToWindow(
			hdc,
			L"Mode MoveTo",
			rectDanceModeMoveTo
		);

		DrawTextToWindow(
			hdc,
			L"Mode Slider",
			rectDanceModeSlider
		);

		DrawTextToWindow(
			hdc,
			L"Mode Spinner",
			rectDanceModeSpinner
		);

		// Buttons
		hwndButtonOpenSongFolder = CreateWindow(
			WC_BUTTON,
			pathSet ? L"Change" : L"Select",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			(nWidth - 110), 10,
			100, 40,
			hwnd,
			(HMENU)BTN_ButtonOpenSongFolder,
			(HINSTANCE)LongToPtr(GetWindowLong(hwnd, GWLP_HINSTANCE)),
			nullptr
		);

		hwndButtonOpenSongFile = CreateWindow(
			WC_BUTTON,
			songFileCheck ? L"Change" : L"Select",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			(nWidth - 110), 80,
			100, 40,
			hwnd,
			(HMENU)BTN_ButtonOpenSongFile,
			(HINSTANCE)LongToPtr(GetWindowLong(hwnd, GWLP_HINSTANCE)),
			nullptr
		);

		// Check Boxes
		hwndCheckBoxAutoOpenSong = CreateWindowEx(
			NULL,
			WC_BUTTON,
			L"Auto Open Song",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
			10, 60,
			140, 15,
			hwnd,
			(HMENU)CB_CheckBoxAutoOpenSong,
			(HINSTANCE)LongToPtr(GetWindowLong(hwnd, GWLP_HINSTANCE)),
			nullptr
		);

		SendMessage(hwndCheckBoxAutoOpenSong, BM_SETCHECK, WPARAM(autoOpenSong), NULL);

		hwndCheckBoxHardrockFlip = CreateWindowEx(
			NULL,
			WC_BUTTON,
			L"Hardrock (flip)",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
			10, 130,
			140, 15,
			hwnd,
			(HMENU)CB_CheckBoxHardrockFlip,
			(HINSTANCE)LongToPtr(GetWindowLong(hwnd, GWLP_HINSTANCE)),
			nullptr
		);

		SendMessage(hwndCheckBoxHardrockFlip, BM_SETCHECK, WPARAM(hardrockFlip), NULL);

		// Dance Settings
		hwndTrackBarDanceAmplifier = CreateWindowEx(
			NULL,
			TRACKBAR_CLASS,
			NULL,
			WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_BOTTOM | TBS_DOWNISLEFT | TBS_AUTOTICKS | TBS_TOOLTIPS,
			10, 170,
			180, 30,
			hwnd,
			(HMENU)TB_TrackBarDanceAmplifier,
			(HINSTANCE)LongToPtr(GetWindowLong(hwnd, GWLP_HINSTANCE)),
			nullptr
		);

		SendMessage(hwndTrackBarDanceAmplifier, TBM_SETRANGE, FALSE, MAKELPARAM(50, 200));
		SendMessage(hwndTrackBarDanceAmplifier, TBM_SETTICFREQ, (WPARAM)10, NULL);
		SendMessage(hwndTrackBarDanceAmplifier, TBM_SETPOS, TRUE, trackBarPos);

		hwndComboBoxDanceModeMoveTo = CreateWindowEx(
			NULL,
			WC_COMBOBOX,
			L"Mode MoveTo",
			WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
			210, 175,
			110, 100,
			hwnd,
			(HMENU)CB_ComboBoxDanceModeMoveTo,
			(HINSTANCE)LongToPtr(GetWindowLong(hwnd, GWLP_HINSTANCE)),
			nullptr
		);

		SendMessage(hwndComboBoxDanceModeMoveTo, CB_ADDSTRING, NULL, (LPARAM)&modeNone);
		SendMessage(hwndComboBoxDanceModeMoveTo, CB_ADDSTRING, NULL, (LPARAM)&modeStandard);
		SendMessage(hwndComboBoxDanceModeMoveTo, CB_ADDSTRING, NULL, (LPARAM)&modeFlowing);
		SendMessage(hwndComboBoxDanceModeMoveTo, CB_ADDSTRING, NULL, (LPARAM)&modePredicting);
		SendMessage(hwndComboBoxDanceModeMoveTo, CB_SETCURSEL, (WPARAM)modeMoveTo, NULL);

		hwndComboBoxDanceModeSlider = CreateWindowEx(
			NULL,
			WC_COMBOBOX,
			L"Mode Slider",
			WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
			330, 175,
			110, 100,
			hwnd,
			(HMENU)CB_ComboBoxDanceModeSlider,
			(HINSTANCE)LongToPtr(GetWindowLong(hwnd, GWLP_HINSTANCE)),
			nullptr
		);

		SendMessage(hwndComboBoxDanceModeSlider, CB_ADDSTRING, NULL, (LPARAM)&modeNone);
		SendMessage(hwndComboBoxDanceModeSlider, CB_ADDSTRING, NULL, (LPARAM)&modeStandard);
		SendMessage(hwndComboBoxDanceModeSlider, CB_ADDSTRING, NULL, (LPARAM)&modeFlowing);
		SendMessage(hwndComboBoxDanceModeSlider, CB_ADDSTRING, NULL, (LPARAM)&modePredicting);
		SendMessage(hwndComboBoxDanceModeSlider, CB_SETCURSEL, (WPARAM)modeSlider, NULL);

		hwndComboBoxDanceModeSpinner = CreateWindowEx(
			NULL,
			WC_COMBOBOX,
			L"Mode Spinner",
			WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
			450, 175,
			110, 100,
			hwnd,
			(HMENU)CB_ComboBoxDanceModeSpinner,
			(HINSTANCE)LongToPtr(GetWindowLong(hwnd, GWLP_HINSTANCE)),
			nullptr
		);

		SendMessage(hwndComboBoxDanceModeSpinner, CB_ADDSTRING, NULL, (LPARAM)&modeNone);
		SendMessage(hwndComboBoxDanceModeSpinner, CB_ADDSTRING, NULL, (LPARAM)&modeStandard);
		SendMessage(hwndComboBoxDanceModeSpinner, CB_ADDSTRING, NULL, (LPARAM)&modeFlowing);
		SendMessage(hwndComboBoxDanceModeSpinner, CB_ADDSTRING, NULL, (LPARAM)&modePredicting);
		SendMessage(hwndComboBoxDanceModeSpinner, CB_SETCURSEL, (WPARAM)modeSpinner, NULL);

		// Utilities
		hwndProgressBar = CreateWindowEx(
			NULL,
			PROGRESS_CLASS,
			NULL,
			WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
			10, (nHeight - 40),
			(nWidth - 20), 30,
			hwnd,
			nullptr,
			hInst,
			nullptr
		);

		SendMessage(hwndProgressBar, PBM_SETRANGE, NULL, MAKELPARAM(0, hitObjects.size() + 1));
		SendMessage(hwndProgressBar, PBM_SETSTEP, WPARAM(1), NULL);
		SendMessage(hwndProgressBar, PBM_SETPOS, WPARAM(objectNumber), NULL);

		// TODO END;
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	} return 0;
}

// Message handler for settings box.
INT_PTR CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
	{
		for (int i = 0; i <= 5; i++) {
			int intValue;
			wstring str = L"";
			wstringstream stream;
			stream << hex;

			if (i == 0)
				intValue = threadOffset;
			else
				intValue = offsets[i - 1];

			if (intValue < 0) {
				intValue *= -1;
				stream << intValue;
				str = L"-";
			}
			else {
				stream << intValue;
			}

			str += stream.str();

			SetDlgItemText(hDlg, IDC_THREADOFFSET + i, LPCTSTR(str.c_str()));
		}


		LPCTSTR& inputKeyboard = (LPCTSTR&)L"Keyboard";
		SendDlgItemMessage(hDlg, IDC_INPUTMETHODE, CB_ADDSTRING, NULL, (LPARAM)&inputKeyboard);
		LPCTSTR& inputMouse = (LPCTSTR&)L"Mouse";
		SendDlgItemMessage(hDlg, IDC_INPUTMETHODE, CB_ADDSTRING, NULL, (LPARAM)&inputMouse);
		SendDlgItemMessage(hDlg, IDC_INPUTMETHODE, CB_SETCURSEL, NULL, (LPARAM)!inputKeyBoard);


		TCHAR* keyText = new TCHAR[MAXCHAR];
		if (GetKeyNameText(inputMainKey << 16, keyText, MAXCHAR)) {
			SetDlgItemText(hDlg, IDC_INPUTKEYMAIN, keyText);
		}

		if (GetKeyNameText(inputAltKey << 16, keyText, MAXCHAR)) {
			SetDlgItemText(hDlg, IDC_INPUTKEYALT, keyText);
		}

		delete keyText;

		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		{
			TCHAR method[MAXCHAR];

			GetDlgItemText(hDlg, IDC_INPUTMETHODE, (LPTSTR)method, MAXCHAR);
			inputKeyBoard = _tcscmp(method, L"Keyboard") == 0 ? TRUE : FALSE;


			TCHAR key[MAXCHAR];

			GetDlgItemText(hDlg, IDC_INPUTKEYMAIN, (LPTSTR)key, MAXCHAR);
			inputMainKey = LOWORD(OemKeyScan(key[0]));
			
			GetDlgItemText(hDlg, IDC_INPUTKEYALT, (LPTSTR)key, MAXCHAR);
			inputAltKey = LOWORD(OemKeyScan(key[0]));


			TCHAR offset[MAXCHAR];
			bool subtrating = FALSE;

			for (int i = 0; i <= 5; i++) {
				GetWindowText(GetDlgItem(hDlg, IDC_THREADOFFSET + i), (LPTSTR)offset, MAXCHAR);
				wstring wOffset = wstring(offset);

				if (wOffset[0] == '-') {
					subtrating = TRUE;
				}

				wstring tOffset = wstring(wOffset.begin() + (int)subtrating, wOffset.end());
				wstringstream stream;
				stream << hex << tOffset;

				if (i == 0) {
					stream >> threadOffset;
					if (subtrating) {
						threadOffset = -threadOffset;
						subtrating = FALSE;
					}
				}
				else {
					stream >> offsets[i - 1];
					if (subtrating) {
						offsets[i - 1] = -offsets[i - 1];
						subtrating = FALSE;
					}
				}
			}


			UpdateConfigFile({ inputMethod, inputKeys, timerPointer });


			timeAddress = GetTimeAddress();

			if (timeAddress == nullptr) {
				CloseHandle(osuProcessHandle);

				/* EventLog */	fwprintf(wEventLog, L"[WARNING]  timeAddress NOT FOUND!\n");
				fflush(wEventLog);

				statusText = L"timeAddress NOT found!";
				DrawTextToWindow(hWnd, statusText, rectStatus);
			}

			wstringstream timeAddressString;
			timeAddressString << "0x" << hex << PtrToUlong(timeAddress);

			/* EventLog */	fwprintf(wEventLog, (L"[EVENT]  \"timeAddress\" UPDATED!\n            timeAddress: " + timeAddressString.str() + L"\n").c_str());
			fflush(wEventLog);


			/* EventLog */	fwprintf(wEventLog, L"            Settings are changed!\n");
			EndDialog(hDlg, LOWORD(wParam));
			fflush(wEventLog);

			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			/* EventLog */	fwprintf(wEventLog, L"            Nothing was changed.\n");
			fflush(wEventLog);

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		}
	} return (INT_PTR)FALSE;
}

// Message handler for error box.
INT_PTR CALLBACK ErrorBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		SetDlgItemTextW(hDlg, IDT_ERRORTEXT, LPCWSTR(L"GENERIC ERROR TEXT"));
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		} break;
	} return (INT_PTR)FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		} break;
	} return (INT_PTR)FALSE;
}
