// OsuBot.cpp : Defines the entry point for the application.
//

#define _CRT_SECURE_NO_WARNINGS

#define MAX_LOADSTRING 100
#define BTN_ButtonOpenSongFolder 3001
#define BTN_ButtonOpenSongFile 3002
#define CB_CheckBoxAutoOpenSong 3003
#define CB_CheckBoxHardrockFlip 3004

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
HWND hWnd = NULL;
HWND hwndButtonOpenSongFolder = NULL;
HWND hwndButtonOpenSongFile = NULL;
HWND hwndCheckBoxAutoOpenSong = NULL;
HWND hwndCheckBoxHardrockFlip = NULL;
wstring statusText = L"Start up...";
wstring songsPath;
bool songFileCheck;

// Standard Variables for UI:
wstring songsFolderText = L"Select \"osu!\" Songs Folder.";
wstring songFileText = L"Select an \"osu! beatmap\"";

int nHeight = 290;
int nWidth = 585;
RECT rectOsuBotWindow;
RECT rectSongsFolder = { 10, 10, nWidth - 140, 50 };
RECT rectSongFile = { 10, 80, nWidth - 140, 120 };
RECT rectStatus = { 15, nHeight - 65, nWidth - 30, rectStatus.top + 18 };
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
	CreateDirectory(LPCWSTR(L"Data"), NULL);


	if (fopen("Data\\Events.log", "r")) {
		DeleteFile(L"Data\\Events_OLD.log");
		experimental::filesystem::copy("Data\\Events.log", "Data\\Events_OLD.log");
		_fcloseall();
	}

	wEventLog = fopen("Data\\Events.log", "w");
	if (wEventLog == NULL) { DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox); }

	time_t timeStamp = time(nullptr);
	string timeString = asctime(localtime(&timeStamp));
	timeString.erase(timeString.end() - 1, timeString.end());
	string logString = "Events.log created at " + timeString;

	/* EventLog */	fprintf(wEventLog, (logString + "\n").c_str());
	fflush(wEventLog);


	if (fopen("Data\\configFile.cfg", "r")) {
		/* EventLog */	fprintf(wEventLog, "[EVENT]  ConfigFile found.\n");

		if (ReadFromConfigFile({ timerPointer }))
			/* EventLog */	fprintf(wEventLog, "[EVENT]  TimerPointer get succesfull.\n");
		else
			/* EventLog */	fprintf(wEventLog, "[WARNING]  TimerPointer not specified!\n");

		if (ReadFromConfigFile({ songsFolderPath })) {
			/* EventLog */	fprintf(wEventLog, "[EVENT]  SongsFolderPath get succesfull.\n");
			if (songsPath.compare(L"") != 0)
				pathSet = TRUE;
			else
				pathSet = FALSE;
		}
		else {
			/* EventLog */	fprintf(wEventLog, "[WARNING]  SongsFolderPath not specified!\n");
			pathSet = FALSE;
		}

		if (ReadFromConfigFile({ inputKeys }))
			/* EventLog */	fprintf(wEventLog, "[EVENT]  InputKeys get succesfull.\n");
		else
			/* EventLog */	fprintf(wEventLog, "[WARNING]  InputKeys not specified!\n");
	}
	else {
		/* EventLog */	fprintf(wEventLog, "[WARNING]  ConfigFile not found!\n");

		int configMB = MessageBoxW(hWnd,
			L"No config file was found!\nDo you want to generate a new config file?\n\nIf this doesn't work try manualy creating an empty file named \"configFile.cfg\" under the \"Data\" folder.",
			L"Config file not found!",
			MB_ICONWARNING | MB_YESNO | MB_APPLMODAL);

		if (configMB == IDYES) {
			CreateNewConfigFile();

			/* EventLog */	fprintf(wEventLog, "[EVENT]  Config file generated.\n");

			if (ReadFromConfigFile({ timerPointer }))
				/* EventLog */	fprintf(wEventLog, "[EVENT]  TimerPointer get succesfull.\n");
			else
				/* EventLog */	fprintf(wEventLog, "[WARNING]  TimerPointer not specified!\n");

			if (ReadFromConfigFile({ songsFolderPath })) {
				/* EventLog */	fprintf(wEventLog, "[EVENT]  SongsFolderPath get succesfull.\n");
				if (songsPath.compare(L"") != 0)
					pathSet = TRUE;
				else
					pathSet = FALSE;
			}
			else {
				/* EventLog */	fprintf(wEventLog, "[WARNING]  SongsFolderPath not specified!\n");
				pathSet = FALSE;
			}

			if (ReadFromConfigFile({ inputKeys }))
				/* EventLog */	fprintf(wEventLog, "[EVENT]  InputKeys get succesfull.\n");
			else
				/* EventLog */	fprintf(wEventLog, "[WARNING]  InputKeys not specified!\n");
		}
		else
			/* EventLog */	fprintf(wEventLog, "[WARNING]  Config file was not auto generated.\n");
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
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_OSUBOTV2);
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
			fflush(wEventLog);

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

				UpdateConfigFile({ songsFolderPath });
			}
			fflush(wEventLog);

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
			if (autoOpenSong)
				autoOpenSong = FALSE;
			else autoOpenSong = TRUE;

			string autoState = (autoOpenSong ? "Enabled" : "Disabled");
			/* EventLog */	fprintf(wEventLog, ("[EVENT]  Auto opening of beatmap: " + autoState + "\n").c_str());
			fflush(wEventLog);

			if (autoOpenSong && !pathSet) {
				statusText = L"Please select \"osu!\" Songs Folder for Osu!Bot to autosearch in for the beatmaps!";
				DrawTextToWindow(hWnd, statusText, rectStatus);
			}

			break;
		}
		case CB_CheckBoxHardrockFlip:
		{
			if (hardrockFlip)
				hardrockFlip = FALSE;
			else hardrockFlip = TRUE;

			string hardrockState = (hardrockFlip ? "Enabled" : "Disabled");
			/* EventLog */	fprintf(wEventLog, ("[EVENT]  Hardrock mod: " + hardrockState + "\n").c_str());
			fflush(wEventLog);

			break;
		}
		case ID_DATAFILES_OPENDATAFOLDER:
		{
			if ((LONG)ShellExecute(NULL, LPCTSTR(L"explore"), LPCTSTR(L"Data"), NULL, NULL, SW_SHOW) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox);

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Opening \"Data\" Folder.\n");
			fflush(wEventLog);
			break;
		}
		case ID_DATAFILES_OPENSONGDATA:
		{
			if ((LONG)ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"SFData.txt"), NULL, LPCTSTR(L"Data"), SW_SHOW) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox);

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Opening \"SFData.txt\".\n");
			fflush(wEventLog);
			break;
		}
		case ID_DATAFILES_OPENBEATMAPDATA:
		{
			if ((LONG)ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"BMData.txt"), NULL, LPCTSTR(L"Data"), SW_SHOW) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox);

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Opening \"BMData.txt\".\n");
			fflush(wEventLog);
			break;
		}
		case ID_DATAFILES_OPENEVENTLOG:
		{
			if ((LONG)ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"Events.log"), NULL, LPCTSTR(L"Data"), SW_SHOW) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox);

			if ((LONG)ShellExecute(NULL, LPCTSTR(L"open"), LPCTSTR(L"Events_OLD.log"), NULL, LPCTSTR(L"Data"), SW_SHOW) == ERROR_FILE_NOT_FOUND)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ERRORBOX), hWnd, ErrorBox);

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Opening \"Events.log\".\n");
			fflush(wEventLog);
			break;
		}
		case IDM_CLEARDATA:
		{
			int clearDataMB = MessageBoxW(hWnd,
				L"Osu!Bot will clear/clean up all collected data.\nNOTE: this will not clear the current log file and config file!\n\nAre you sure you want to delete the data files?",
				L"Delete Osu!Bot data files?",
				MB_ICONWARNING | MB_YESNO | MB_APPLMODAL);
			if (clearDataMB == IDNO)
				break;

			DeleteFile(L"Data\\SFData.txt");
			DeleteFile(L"Data\\BMData.txt");
			DeleteFile(L"Data\\Events_OLD.log");

			/* EventLog */	fprintf(wEventLog, "[EVENT]  User cleared data Files.\n");
			fflush(wEventLog);

			SendMessage(hwndButtonOpenSongFolder, WM_SETTEXT, 0, LPARAM(L"Select"));
			SendMessage(hwndButtonOpenSongFile, WM_SETTEXT, 0, LPARAM(L"Select"));

			DrawTextToWindow(hWnd, songsFolderText, rectSongsFolder);
			DrawTextToWindow(hWnd, songFileText, rectSongFile);
			break;
		}
		case IDM_SETTINGS:
			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Opened Settings.\n");
			DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGSBOX), hWnd, Settings);
			fflush(wEventLog);
			break;

		case IDM_ABOUT:
			/* EventLog */	fprintf(wEventLog, "[EVENT]  User opened about box.\n");
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			fflush(wEventLog);
			break;

		case IDM_EXIT:
			/* EventLog */	fprintf(wEventLog, "[EVENT]  User Exited the program.\n");
			DestroyWindow(hWnd);
			fflush(wEventLog);
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

		hwndCheckBoxHardrockFlip = CreateWindowEx(
			NULL,
			WC_BUTTON,
			L"Hardrock (flip)",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
			10, 130,
			140, 15,
			hWnd,
			(HMENU)CB_CheckBoxHardrockFlip,
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

// Message handler for settings box.
INT_PTR CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
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

			SetWindowText(GetDlgItem(hDlg, IDC_THREADOFFSET + i), LPCTSTR(str.c_str()));
		}

		
		
		LPCTSTR& inputKeyboard = (LPCTSTR&)L"Keyboard";
		SendDlgItemMessage(hDlg, IDC_INPUTMETHODE, CB_ADDSTRING, NULL, (LPARAM)&inputKeyboard);
		LPCTSTR& inputMouse = (LPCTSTR&)L"Mouse";
		SendDlgItemMessage(hDlg, IDC_INPUTMETHODE, CB_ADDSTRING, NULL, (LPARAM)&inputMouse);
		SendDlgItemMessage(hDlg, IDC_INPUTMETHODE, CB_SETCURSEL, NULL, (LPARAM)!inputKeyBoard);


		LPCTSTR str;

		str = (LPCTSTR)(TCHAR)inputMainKey;
		SetDlgItemText(hDlg, IDC_INPUTKEYMAIN, str);
		str = (LPCTSTR)(TCHAR)inputAltKey;
		SetDlgItemText(hDlg, IDC_INPUTKEYALT, str);


		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			TCHAR key[MAXCHAR];

			GetWindowText(GetDlgItem(hDlg, IDC_INPUTKEYMAIN), (LPTSTR)key, MAXCHAR);
			inputMainKey = wstring(key)[0];

			GetWindowText(GetDlgItem(hDlg, IDC_INPUTKEYALT), (LPTSTR)key, MAXCHAR);
			inputAltKey = wstring(key)[0];

			UpdateConfigFile({ inputKeys });


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


			UpdateConfigFile({ timerPointer });
			ReadFromConfigFile({ timerPointer });


			timeAddress = GetTimeAddress();

			if (timeAddress == reinterpret_cast<LPVOID>(0xCCCCCCCC)
				|| timeAddress == reinterpret_cast<LPVOID>(0x0)) {
				CloseHandle(osuProcessHandle);

				/* EventLog */	fprintf(wEventLog, "[WARNING]  timeAddres NOT FOUND!\n");
				fflush(wEventLog);

				statusText = L"timeAddress NOT found!   Please use a correct timeAddress pointer.";
				DrawTextToWindow(hWnd, statusText, rectStatus);
			}

			stringstream timeAddressString;
			timeAddressString << "0x" << hex << (UINT)timeAddress;

			/* EventLog */	fprintf(wEventLog, ("[EVENT]  \"timeAddres\" UPDATED!\n           timeAddres: " + timeAddressString.str() + "\n").c_str());
			fflush(wEventLog);


			/* EventLog */	fprintf(wEventLog, "            Settings are changed!\n");
			EndDialog(hDlg, LOWORD(wParam));
			fflush(wEventLog);

			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			/* EventLog */	fprintf(wEventLog, "            Nothing was changed.\n");
			fflush(wEventLog);

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	} return (INT_PTR)FALSE;
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
