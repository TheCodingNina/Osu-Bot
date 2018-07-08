#pragma once

#include "stdafx.h"

#include "GlobalVariables.h"
#include <stdexcept>


using namespace std;


enum configurationSettings: wchar_t {
	songsFolderPath = L's',
	inputKeys = L'k',
	inputMethod = L'b',
	timerPointer = L'h'
};


bool WriteToConfigFile(vector<wstring> configStrings) {
	try {
		FILE *wConfigFile = _wfopen(L"Data\\configFile.cfg", L"w");

		for (wstring configString : configStrings) {
			fwprintf(wConfigFile, (configString + L"\n").c_str());
		}

		fclose(wConfigFile);

		return TRUE;
	}
	catch (const exception &e) {
		wstringstream ss; ss << e.what();

		/* EventLog */	fwprintf(wEventLog, (L"[ERROR]  Couldn't write to configuration file!\n            with error: " + ss.str()).c_str());
		fflush(wEventLog);
		return FALSE;
	}
	return FALSE;
}


bool FillMap(map<wstring, LPVOID> &configValues, wstring &configString, const configurationSettings configurationSetting) {
	switch (configurationSetting) {
	case timerPointer:
		configString = L"[Timer Pointer]";
		configValues[L"ThreadOffset"] = &threadOffset;
		configValues[L"Offset0"] = &(offsets[0]);
		configValues[L"Offset1"] = &(offsets[1]);
		configValues[L"Offset2"] = &(offsets[2]);
		configValues[L"Offset3"] = &(offsets[3]);
		configValues[L"Offset4"] = &(offsets[4]);
		break;

	case songsFolderPath:
		configString = L"[Songs Folder Path]";
		configValues[L"FolderPath"] = &songsPath;
		break;

	case inputKeys:
		configString = L"[Input Keys]";
		configValues[L"MainKey"] = &inputMainKey;
		configValues[L"AltKey"] = &inputAltKey;
		break;

	case inputMethod:
		configString = L"[Input Method]";
		configValues[L"UseKeyboard"] = &inputKeyBoard;
		break;

	default:
		return FALSE;
	}

	return TRUE;
}


bool CreateNewConfigFile() {
	try {
		vector<wstring> configStrings = {
			L"[Songs Folder Path]",
			L"FolderPath : ",
			L"",
			L"[Timer Pointer]",
			L"Offset0 : dc",
			L"Offset1 : 750",
			L"Offset2 : a0",
			L"Offset3 : 684",
			L"Offset4 : c8",
			L"ThreadOffset : -32c",
			L"",
			L"[Input Methode]",
			L"UseKeyboard : true",
			L"",
			L"[Input Keys] //Currently only works with non-special keys!",
			L"MainKey : Z",
			L"AltKey : X",
			L""
		};

		WriteToConfigFile(configStrings);
		return TRUE;
	}
	catch (const exception &e) {
		wstringstream ss; ss << e.what();
		
		/* EventLog */	fwprintf(wEventLog, (L"[ERROR]  Couldn't create a configuration file!\n            with error: " + ss.str()).c_str());
		fflush(wEventLog);
		return FALSE;
	}
}


bool AddSettingString(wstring &settingString, const wstring configSetting, const LPVOID configValue, const configurationSettings configurationSetting) {
	if (configurationSetting == L'i') {
		int* settingValue = (int*)configValue;
		settingString = configSetting + L" : " + to_wstring(*settingValue);
	}
	else if (configurationSetting == L'h') {
		wstringstream ss;
		int* settingValue = (int*)configValue;
		int hexValue = *settingValue;

		if (*settingValue < 0) {
			hexValue *= -1;
			ss << hex << hexValue;
			settingString = configSetting + L" : -" + wstring(ss.str());
		}
		else {
			ss << hex << hexValue;
			settingString = configSetting + L" : " + wstring(ss.str());
		}
	}
	else if (configurationSetting == L's') {
		wstring* settingValue = (wstring*)configValue;
		wstring wValue = *settingValue;
		settingString = configSetting + L" : " + wValue;
	}
	else if (configurationSetting == L'k') {
		int* settingValue = (int*)configValue;
		TCHAR keyValue = (TCHAR)*settingValue;
		settingString = configSetting + L" : " + keyValue;
	}
	else if (configurationSetting == L'b') {
		bool* settingValue = (bool*)configValue;
		bool boolValue = *settingValue;
		wstring boolString = (boolValue ? L"true" : L"false");
		settingString = configSetting + L" : " + boolString;
	}
	else
		return FALSE;
	return TRUE;
}

bool UpdateConfigFile(const vector<configurationSettings> &settingsList) {
	try {
		wstring readLine;
		vector<wstring> allConfigStrings;

		for (configurationSettings configurationSetting : settingsList) {
			wstring configString;
			vector<wstring> configStrings;
			map<wstring, LPVOID> configValues;

			FillMap(configValues, configString, configurationSetting);

			wfstream configFile;
			configFile.open(L"Data\\configFile.cfg", wfstream::in | wfstream::out);

			while (!configFile.eof()) {
				getline(configFile, readLine, configFile.widen('\n'));

				if (readLine.find(configString) != wstring::npos) {
					configStrings.push_back(configString);
					while (configValues.size() > 0 && (readLine.compare(L"") != 0 || readLine[0] == L'[')) {
						getline(configFile, readLine);

						wstring configSetting = readLine.substr(0,
							MIN(readLine.find_first_of(L":"), readLine.find_first_of(L" ")));

						wstring settingString;
						auto settingFinder = configValues.find(configSetting);
						if (settingFinder != configValues.end()) {
							AddSettingString(settingString, configSetting, settingFinder->second, configurationSetting);

							configValues.erase(settingFinder);
						}
						else {
							settingString = readLine;
						}
						configStrings.push_back(settingString);
					}
					for (auto& configValue : configValues) {
						wstring settingString;
						AddSettingString(settingString, configValue.first, configValue.second, configurationSetting);

						configStrings.push_back(settingString);
					}

					configValues.clear();
				}
				else {
					configStrings.push_back(readLine);
				}
			}

			if (configValues.size() > 0) {
				configStrings.push_back(configString);
				for (auto& configValue : configValues) {
					wstring settingString;

					AddSettingString(settingString, configValue.first, configValue.second, configurationSetting);

					configStrings.push_back(settingString);
				}
			}

			configFile.close();

			while (configStrings.at(0) == L"")
				configStrings.erase(configStrings.begin());
			while (configStrings.at(configStrings.size() - 1) == L"")
				configStrings.pop_back();

			configStrings.push_back(L"");

			allConfigStrings.reserve(allConfigStrings.size() + configStrings.size());
			allConfigStrings.insert(allConfigStrings.end(), configStrings.begin(), configStrings.end());

			allConfigStrings.pop_back();

			WriteToConfigFile(allConfigStrings);

			allConfigStrings.clear();
		}

		return TRUE;
	}
	catch (const exception &e) {
		wstringstream ss; ss << e.what();

		/* EventLog */	fwprintf(wEventLog, (L"[ERROR]  Couldn't update a configuration file!\n            with error: " + ss.str()).c_str());
		fflush(wEventLog);
		return FALSE;
	}
}

bool UpdateConfigFile(const configurationSettings &setting) {
	return UpdateConfigFile(vector<configurationSettings>(setting));
}

bool ReadFromConfigFile(const configurationSettings &setting) {
	try {
		wstring readLine;

		wstring configString;
		map<wstring, LPVOID> configValues;

		FillMap(configValues, configString, setting);

		wfstream configFile;
		configFile.open("Data\\configFile.cfg", wfstream::in);

		while (!configFile.eof() && readLine.find(configString) == wstring::npos) {
			getline(configFile, readLine, configFile.widen(L'\n'));
		}

		if (readLine.empty() && configFile.eof())
			return FALSE;


		for (unsigned int i = 0; i < configValues.size(); i++) {
			getline(configFile, readLine);
			wstring configSetting = readLine.substr(0,
				MIN(readLine.find_first_of(L" "), readLine.find_first_of(L":")) - 1);

			UINT pos = readLine.find_first_of(L":") + 1U;
			UINT valuePos = readLine.at(pos) == L' ' ? pos + 1U : pos;

			if (setting == L'i') {
				int intValue;
				if (readLine.at(valuePos) == L'-') {
					intValue = stoi(readLine.substr(valuePos + 1));
					intValue *= -1;
				}
				else
					intValue = stoi(readLine.substr(valuePos));

				int* configValue = (int*)(configValues.at(configSetting));
				*configValue = intValue;
			}
			else if (setting == L'h') {
				wstringstream ss;
				int hexValue;
				if (readLine.at(valuePos) == L'-') {
					ss << hex << readLine.substr(valuePos + 1);
					ss >> hexValue;
					hexValue *= -1;
				}
				else {
					ss << hex << readLine.substr(valuePos);
					ss >> hexValue;
				}

				int* configValue = (int*)(configValues.at(configSetting));
				*configValue = hexValue;
			}
			else if (setting == L's') {
				wstring stringValue = readLine.substr(valuePos);

				wstring* configValue = (wstring*)(configValues.at(configSetting));
				*configValue = stringValue;
			}
			else if (setting == L'k') {
				wchar_t charValue = readLine.substr(valuePos)[0];
				int KeyValue = (int)charValue;

				int* configValue = (int*)(configValues.at(configSetting));
				*configValue = KeyValue;
			}
			else if (setting == L'b') {
				wstring stringValue = readLine.substr(valuePos);

				bool boolValue;
				if (stringValue[0] == L't' || stringValue[0] == L'T')
					boolValue = TRUE;
				else if (stringValue[0] == L'f' || stringValue[0] == L'F')
					boolValue = FALSE;
				else
					boolValue = FALSE;

				bool* configValue = (bool*)(configValues.at(configSetting));
				*configValue = boolValue;
			}
			else
				return FALSE;
		}

		configFile.close();

		return TRUE;
	}
	catch (const exception &e) {
		wstringstream ss; ss << e.what();

		/* EventLog */	fwprintf(wEventLog, (L"[ERROR]  Couldn't read from configuration file!\n            with error: " + ss.str()).c_str());
		fflush(wEventLog);
		return FALSE;
	}
}

bool ReadAllConfigSettings() {
	try {
		if (ReadFromConfigFile(timerPointer))
			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  TimerPointer get succesfull.\n");
		else
			/* EventLog */	fwprintf(wEventLog, L"[WARNING]  TimerPointer not specified!\n");

		if (ReadFromConfigFile(songsFolderPath)) {
			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  SongsFolderPath get succesfull.\n");
			if (songsPath.compare(L"") != 0)
				pathSet = TRUE;
			else
				pathSet = FALSE;
		}
		else {
			/* EventLog */	fwprintf(wEventLog, L"[WARNING]  SongsFolderPath not specified!\n");
			pathSet = FALSE;
		}

		if (ReadFromConfigFile(inputKeys))
			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  InputKeys get succesfull.\n");
		else
			/* EventLog */	fwprintf(wEventLog, L"[WARNING]  InputKeys not specified!\n");

		if (ReadFromConfigFile(inputMethod))
			/* EventLog */	fwprintf(wEventLog, L"[EVENT]  InputMethod get succesfull.\n");
		else
			/* EventLog */	fwprintf(wEventLog, L"[WARNING]  InputMethod not specified!\n");

		return TRUE;
	}
	catch (const exception &e) {
		wstringstream ss; ss << e.what();

		/* EventLog */	fwprintf(wEventLog, (L"[ERROR]  Couldn't read from configuration file!\n            with error: " + ss.str()).c_str());
		fflush(wEventLog);
		return FALSE;
	}
}