#pragma once

#include "stdafx.h"

#include "GlobalVariables.h"
#include <stdexcept>


using namespace std;


enum configurationSettings: char {
	songsFolderPath = 's',
	inputKeys = 'k',
	inputMethod = 'b',
	timerPointer = 'h'
};


bool WriteToConfigFile(vector<string> configStrings) {
	try {
		FILE *wConfigFile = fopen("Data\\configFile.cfg", "w");

		for (string configString : configStrings) {
			fprintf(wConfigFile, (configString + "\n").c_str());
		}

		fclose(wConfigFile);

		return TRUE;
	}
	catch (const exception &e) {
		/* EventLog */	fprintf(wEventLog, ("[ERROR]  Couldn't write to configuration file!\n            with error: " + (string)e.what() + "\n").c_str());
		fflush(wEventLog);
		return FALSE;
	}
	return FALSE;
}


bool FillMap(map<string, LPVOID> &configValues, string &configString, const configurationSettings configurationSetting) {
	switch (configurationSetting) {
	case timerPointer:
		configString = "[Timer Pointer]";
		configValues["ThreadOffset"] = &threadOffset;
		configValues["Offset0"] = &(offsets[0]);
		configValues["Offset1"] = &(offsets[1]);
		configValues["Offset2"] = &(offsets[2]);
		configValues["Offset3"] = &(offsets[3]);
		configValues["Offset4"] = &(offsets[4]);
		break;

	case songsFolderPath:
		configString = "[Songs Folder Path]";
		configValues["FolderPath"] = &songsPath;
		break;

	case inputKeys:
		configString = "[Input Keys]";
		configValues["MainKey"] = &inputMainKey;
		configValues["AltKey"] = &inputAltKey;
		break;

	case inputMethod:
		configString = "[Input Method]";
		configValues["UseKeyboard"] = &inputKeyBoard;
		break;

	default:
		return FALSE;
	}

	return TRUE;
}


bool CreateNewConfigFile() {
	try {
		vector<string> configStrings = {
			"[Songs Folder Path]",
			"FolderPath : ",
			"",
			"[Timer Pointer]",
			"Offset0 : dc",
			"Offset1 : 750",
			"Offset2 : a0",
			"Offset3 : 684",
			"Offset4 : c8",
			"ThreadOffset : -32c",
			"",
			"[Input Methode]",
			"UseKeyboard : true",
			"",
			"[Input Keys] //Currently only works with non-special keys!",
			"MainKey : Z",
			"AltKey : X",
			""
		};

		WriteToConfigFile(configStrings);
		return TRUE;
	}
	catch (const exception &e) {
		/* EventLog */	fprintf(wEventLog, ("[ERROR]  Couldn't create a configuration file!\n            with error: " + (string)e.what() + "\n").c_str());
		fflush(wEventLog);
		return FALSE;
	}
}


bool AddSettingString(string &settingString, const string configSetting, const LPVOID configValue, const configurationSettings configurationSetting) {
	if (configurationSetting == 'i') {
		int* settingValue = (int*)configValue;
		settingString = configSetting + " : " + to_string(*settingValue);
	}
	else if (configurationSetting == 'h') {
		stringstream ss;
		int* settingValue = (int*)configValue;
		int hexValue = *settingValue;

		if (*settingValue < 0) {
			hexValue *= -1;
			ss << hex << hexValue;
			settingString = configSetting + " : -" + string(ss.str());
		}
		else {
			ss << hex << hexValue;
			settingString = configSetting + " : " + string(ss.str());
		}
	}
	else if (configurationSetting == 's') {
		wstring* settingValue = (wstring*)configValue;
		wstring wValue = *settingValue;
		settingString = configSetting + " : " + string(wValue.begin(), wValue.end());
	}
	else if (configurationSetting == 'k') {
		int* settingValue = (int*)configValue;
		char keyValue = (char)*settingValue;
		settingString = configSetting + " : " + keyValue;
	}
	else if (configurationSetting == 'b') {
		bool* settingValue = (bool*)configValue;
		bool boolValue = *settingValue;
		string boolString = (boolValue ? "true" : "false");
		settingString = configSetting + " : " + boolString;
	}
	else
		return FALSE;
	return TRUE;
}

bool UpdateConfigFile(const vector<configurationSettings> &settingsList) {
	try {
		string readLine;
		vector<string> allConfigStrings;

		for (configurationSettings configurationSetting : settingsList) {
			string configString;
			vector<string> configStrings;
			map<string, LPVOID> configValues;

			FillMap(configValues, configString, configurationSetting);

			fstream configFile;
			configFile.open("Data\\configFile.cfg", fstream::in | fstream::out);

			while (!configFile.eof()) {
				getline(configFile, readLine);

				if (readLine.find(configString) != string::npos) {
					configStrings.push_back(configString);
					while (configValues.size() > 0 && (readLine.compare("") != 0 || readLine[0] == '[')) {
						getline(configFile, readLine);

						string configSetting = readLine.substr(0,
							MIN(readLine.find_first_of(":"), readLine.find_first_of(" ")));

						string settingString;
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
						string settingString;
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
					string settingString;

					AddSettingString(settingString, configValue.first, configValue.second, configurationSetting);

					configStrings.push_back(settingString);
				}
			}

			configFile.close();

			while (configStrings.at(0) == "")
				configStrings.erase(configStrings.begin());
			while (configStrings.at(configStrings.size() - 1) == "")
				configStrings.pop_back();

			configStrings.push_back("");

			allConfigStrings.reserve(allConfigStrings.size() + configStrings.size());
			allConfigStrings.insert(allConfigStrings.end(), configStrings.begin(), configStrings.end());

			allConfigStrings.pop_back();

			WriteToConfigFile(allConfigStrings);

			allConfigStrings.clear();
		}

		return TRUE;
	}
	catch (const exception &e) {
		/* EventLog */	fprintf(wEventLog, ("[ERROR]  Couldn't update a configuration file!\n            with error: " + (string)e.what() + "\n").c_str());
		fflush(wEventLog);
		return FALSE;
	}
}

bool UpdateConfigFile(const configurationSettings &setting) {
	return UpdateConfigFile(vector<configurationSettings>(setting));
}

bool ReadFromConfigFile(const configurationSettings &setting) {
	try {
		string readLine;

		string configString;
		map<string, LPVOID> configValues;

		FillMap(configValues, configString, setting);

		fstream configFile;
		configFile.open("Data\\configFile.cfg", fstream::in);

		while (!configFile.eof() && readLine.find(configString) == string::npos) {
			getline(configFile, readLine);
		}

		if (readLine.empty() && configFile.eof())
			return FALSE;


		for (unsigned int i = 0; i < configValues.size(); i++) {
			getline(configFile, readLine);
			string configSetting = readLine.substr(0,
				MIN(readLine.find_first_of(" "), readLine.find_first_of(":")) - 1);

			UINT valuePos = readLine.find_first_of(":") + 1U;
			while (readLine.at(valuePos) == ' ') {
				valuePos++;
			}
			
			if (setting == 'i') {
				int intValue;
				if (readLine.at(valuePos) == '-') {
					intValue = stoi(readLine.substr(valuePos + 1));
					intValue *= -1;
				}
				else
					intValue = stoi(readLine.substr(valuePos));

				int* configValue = (int*)(configValues.at(configSetting));
				*configValue = intValue;
			}
			else if (setting == 'h') {
				stringstream ss;
				int hexValue;
				if (readLine.at(valuePos) == '-') {
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
			else if (setting == 's') {
				string stringValue = readLine.substr(valuePos);

				wstring* configValue = (wstring*)(configValues.at(configSetting));
				*configValue = wstring(stringValue.begin(), stringValue.end());
			}
			else if (setting == 'k') {
				char charValue = readLine.substr(valuePos)[0];
				int KeyValue = (int)charValue;

				int* configValue = (int*)(configValues.at(configSetting));
				*configValue = KeyValue;
			}
			else if (setting == 'b') {
				string stringValue = readLine.substr(valuePos);

				bool boolValue;
				if (stringValue[0] == 't' || stringValue[0] == 'T')
					boolValue = TRUE;
				else if (stringValue[0] == 'f' || stringValue[0] == 'F')
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
		/* EventLog */	fprintf(wEventLog, ("[ERROR]  Couldn't read from configuration file!\n            with error: " + (string)e.what() + "\n").c_str());
		fflush(wEventLog);
		return FALSE;
	}
}

bool ReadAllConfigSettings() {
	try {
		if (ReadFromConfigFile(timerPointer))
			/* EventLog */	fprintf(wEventLog, "[EVENT]  TimerPointer get succesfull.\n");
		else
			/* EventLog */	fprintf(wEventLog, "[WARNING]  TimerPointer not specified!\n");

		if (ReadFromConfigFile(songsFolderPath)) {
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

		if (ReadFromConfigFile(inputKeys))
			/* EventLog */	fprintf(wEventLog, "[EVENT]  InputKeys get succesfull.\n");
		else
			/* EventLog */	fprintf(wEventLog, "[WARNING]  InputKeys not specified!\n");

		if (ReadFromConfigFile(inputMethod))
			/* EventLog */	fprintf(wEventLog, "[EVENT]  InputMethod get succesfull.\n");
		else
			/* EventLog */	fprintf(wEventLog, "[WARNING]  InputMethod not specified!\n");

		return TRUE;
	}
	catch (const exception &e) {
		/* EventLog */	fprintf(wEventLog, ("[ERROR]  Couldn't read from configuration file!\n            with error: " + (string)e.what() + "\n").c_str());
		fflush(wEventLog);
		return FALSE;
	}
}