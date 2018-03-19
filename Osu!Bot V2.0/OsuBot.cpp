// OsuBot.cpp : Defines the entry point for the application.
//

#define _CRT_SECURE_NO_WARNINGS

#define MAX_LOADSTRING 100
#define BTN_ButtonOpenSongFolder 3001
#define BTN_ButtonOpenSongFile 3002
#define CB_CheckBoxAutoOpenSong 3003

#include "stdafx.h"
#include "OsuBot.h"

using namespace std;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


// User Global Variables:
HWND hWnd = NULL,
hwndButtonOpenSongFolder = NULL,
hwndButtonOpenSongFile = NULL,
hwndCheckBoxAutoOpenSong = NULL;
wstring statusText = L"Start up...";
bool songFileCheck;

// Standard Varables for UI:
wstring songsFolderText = L"Select \"osu!\" Songs Folder.";
wstring songFileText = L"Select an \"osu! beatmap\"";

RECT rectOsuBotWindow;
RECT rectSongsFolder = { 10, 10, nWidth - (rectSongsFolder.left + 130), rectSongsFolder.top + 40 };
RECT rectSongFile = { 10, 80, nWidth - (rectSongFile.left + 130), rectSongFile.top + 40 };
// User Global Variables END;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	ErrorBox(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place user added startup code here.
	CreateDirectory(LPCWSTR(L"Data"), NULL);
	wEventLog = fopen("Data\\Event.log", "w");
	if (wEventLog == NULL) { DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox); }

	time_t timeStamp = time(nullptr);
	string timeString = asctime(localtime(&timeStamp));
	timeString.erase(timeString.end() - 1, timeString.end());
	string logString = "Event.log created at " + timeString;

	/* EventLog */	fprintf(wEventLog, (logString + "\n").c_str());

	fstream rSFData; rSFData.open("Data\\SFData.txt", fstream::in);
	if (rSFData) {
		string readLine;
		getline(rSFData, readLine); getline(rSFData, readLine);
		songsPath.assign(readLine.begin(), readLine.end());
		if (!songsPath.empty()) { pathSet = TRUE; }
		rSFData.close();
	}
	else { 
		rSFData.close(); 
		pathSet = FALSE;
		/* EventLog */	fprintf(wEventLog, "[WARNING]  NO songsfolder path was found!\n");
	}

	thread findGameThread(FindGame);
	findGameThread.detach();
	// TODO END;

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_OSUBOTV20, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OSUBOTV20));

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
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OSUBOTV20));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_OSUBOTV20);
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

	hWnd = CreateWindowW(
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
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	
	switch (message) {
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 600;
		lpMMI->ptMinTrackSize.y = 350;
		break;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId) {
		case BTN_ButtonOpenSongFolder:
		{
			SendMessage(hwndButtonOpenSongFolder, WM_SETTEXT, 0, LPARAM(L"..."));

			statusText = L"Selecting Songs Folder...";
			DrawTextToWindow(hWnd, statusText, rectStatus);

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User selecting/changing Songs Folder.\n");

			pathSet = OpenSongFolder();

			if (!pathSet) {
				statusText = L"No Songs Folder Selected!";
				DrawTextToWindow(hWnd, statusText, rectStatus);

				/* EventLog */	fprintf(wEventLog, "[EVENT]  No Songs folder Selected.\n");
			}
			else {
				DrawTextToWindow(hWnd, songsPath.c_str(), rectSongsFolder);
				statusText = L"Songs Folder Successfully Selected!";
				DrawTextToWindow(hWnd, statusText, rectStatus);

				/* EventLog */	fprintf(wEventLog, "[EVENT]  Songs Folder Successfully Selected.\n           or not changed.\n");
			}

			SendMessage(hwndButtonOpenSongFolder, WM_SETTEXT, 0, ((pathSet) ? (LPARAM(L"Change")) : (LPARAM(L"Select"))));
			break;
		}
		case BTN_ButtonOpenSongFile:
		{
			SendMessage(hwndButtonOpenSongFile, WM_SETTEXT, 0, LPARAM(L"..."));

			statusText = L"Selecting a Beatmap...";
			DrawTextToWindow(hWnd, statusText, rectStatus);

			songFileCheck = OpenSongManual();
			SongFileCheck(songFileCheck, userSelect);

			SendMessage(hwndButtonOpenSongFile, WM_SETTEXT, 0, ((songFileCheck) ? (LPARAM(L"Change")) : (LPARAM(L"Select"))));
			break;
		}
		case CB_CheckBoxAutoOpenSong:
		{
			if (autoOpenSong) {
				autoOpenSong = FALSE;
			}
			else {
				autoOpenSong = TRUE;
			}

			string autoState = (autoOpenSong ? "Enabled" : "Disabled");
			/* EventLog */	fprintf(wEventLog, ("[EVENT]  Auto opening of beatmap: " + autoState + "\n").c_str());

			if (autoOpenSong && !pathSet) {
				statusText = L"Please select \"osu!\" Songs Folder for Osu!Bot to autosearch in for the beatmaps!";
				DrawTextToWindow(hWnd, statusText, rectStatus);
			}

			break;
		}
		case ID_DATAFILES_OPENDATAFOLDER:
		{
			if ((LONG)ShellExecute(NULL, LPCTSTR(L"explore"), LPCTSTR(L"Data"), NULL, NULL, SW_SHOW) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox);

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Opening \"Data\" Folder.\n");
			break;
		}
		case ID_DATAFILES_OPENSONGDATA:
		{
			if ((LONG)ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"SFData.txt"), NULL, LPCTSTR(L"Data"), SW_SHOW) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox);

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Opening \"SFData.txt\".\n");
			break;
		}
		case ID_DATAFILES_OPENBEATMAPDATA:
		{
			if ((LONG)ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"BMData.txt"), NULL, LPCTSTR(L"Data"), SW_SHOW) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox);

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Opening \"BMData.txt\".\n");
			break;
		}
		case ID_DATAFILES_OPENEVENTLOG:
		{
			if ((LONG)ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"Event.log"), NULL, LPCTSTR(L"Data"), SW_SHOW) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox);

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Opening \"Event.log\".\n");
			break;
		}
		case IDM_CLEARDATA:
		{
			DeleteFile(L"Data\\SFData.txt");
			DeleteFile(L"Data\\BMData.txt");

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User cleared Data Files.\n");

			SendMessage(hwndButtonOpenSongFolder, WM_SETTEXT, 0, LPARAM(L"Select"));
			SendMessage(hwndButtonOpenSongFile, WM_SETTEXT, 0, LPARAM(L"Select"));

			DrawTextToWindow(hWnd, songsFolderText, rectSongsFolder);
			DrawTextToWindow(hWnd, songFileText, rectSongFile);
			break;
		}
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		} break;
	}
	case WM_PAINT:
	{
		// Clean-up before re-draw hwnds
		DestroyWindow(hwndButtonOpenSongFile);
		DestroyWindow(hwndButtonOpenSongFolder);
		DestroyWindow(hwndProgressBar);


		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		GetClientRect(hWnd, &rectOsuBotWindow);
		nHeight = rectOsuBotWindow.bottom > 290 ? rectOsuBotWindow.bottom : 290;
		nWidth = rectOsuBotWindow.right > 585 ? rectOsuBotWindow.right : 585;

		rectSongsFolder = { 10, 10, nWidth - (rectSongsFolder.left + 130), rectSongsFolder.top + 40 };
		rectSongFile = { 10, 80, nWidth - (rectSongFile.left + 130), rectSongFile.top + 40 };
		rectStatus = { 15, nHeight - 65, nWidth - (2 * rectStatus.left), rectStatus.top + 20 };


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

		// Buttons
		hwndButtonOpenSongFolder = CreateWindowW(
			WC_BUTTON,
			pathSet ? L"Change" : L"Select",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT,
			(nWidth - 110), 10,
			100, 40,
			hWnd,
			(HMENU)BTN_ButtonOpenSongFolder,
			(HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE),
			nullptr
		);

		hwndButtonOpenSongFile = CreateWindowW(
			WC_BUTTON,
			songFileCheck ? L"Change" : L"Select",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT,
			(nWidth - 110), 80,
			100, 40,
			hWnd,
			(HMENU)BTN_ButtonOpenSongFile,
			(HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE),
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
			hWnd,
			(HMENU)CB_CheckBoxAutoOpenSong,
			(HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE),
			nullptr
		);

		SendMessage(hwndCheckBoxAutoOpenSong, BM_SETCHECK, WPARAM(autoOpenSong), NULL);

		// Utilities
		hwndProgressBar = CreateWindowEx(
			NULL,
			PROGRESS_CLASS,
			NULL,
			WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
			10, (nHeight - 40),
			(nWidth - 20), 30,
			hWnd,
			nullptr,
			hInst,
			nullptr
		);

		SendMessage(hwndProgressBar, PBM_SETRANGE, NULL, MAKELPARAM(0, hitObjects.size() + 1));
		SendMessage(hwndProgressBar, PBM_SETSTEP, WPARAM(1), NULL);

		// TODO END;
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	} return 0;
}

// Message handler for error box.
INT_PTR CALLBACK ErrorBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		SetDlgItemTextW(hDlg, IDT_ERRORTEXT, LPCWSTR(L"File Not Found."));
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
