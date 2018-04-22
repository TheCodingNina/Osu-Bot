#pragma once

#include "stdafx.h"

#include "GlobalVariables.h"
#include <stdexcept>


using namespace std;


enum configurationSettings: char {
	songsFolderPath = 's',
	timerPointer = 'i'
};


bool WriteToConfigFile(vector<string> configStrings);


bool CreateNewConfigFile() {
	try {
		vector<string> configStrings = {
			"[Songs Folder Path]",
			"FolderPath : ",
			"",
			"[Timer Pointer]",
			"ThreadOffset : -812",
			"Offset0 : 220",
			"Offset1 : 1960",
			"Offset2 : 1612",
			"Offset3 : 1988",
			"Offset4 : 1632",
			""
		};

		WriteToConfigFile(configStrings);
		return TRUE;
	}
	catch (const exception &e) {
		/* EventLog */	fprintf(wEventLog, ("[ERROR]  Couldn't create a configuration file!\n            with error: " + (string)e.what()).c_str());
		fflush(wEventLog);
		return FALSE;
	}
	return FALSE;
}


bool UpdateConfigFile() {
	try {
		vector<string> configStrings = {
			"[Songs Folder Path]",
			("FolderPath : " + string(songsPath.begin(), songsPath.end())),
			"",
			"[Timer Pointer]",
			("ThreadOffset : " + to_string(threadOffset)),
			("Offset0 : " + to_string(offsets[0])),
			("Offset1 : " + to_string(offsets[1])),
			("Offset2 : " + to_string(offsets[2])),
			("Offset3 : " + to_string(offsets[3])),
			("Offset4 : " + to_string(offsets[4])),
			""
		};

		WriteToConfigFile(configStrings);
		return TRUE;
	}
	catch (const exception &e) {
		/* EventLog */	fprintf(wEventLog, ("[ERROR]  Couldn't update a configuration file!\n            with error: " + (string)e.what()).c_str());
		fflush(wEventLog);
		return FALSE;
	}
	return FALSE;
}

bool ReadFromConfigFile(const vector<configurationSettings> &settingsList) {
	try {
		string readLine;

		for (configurationSettings configurationSetting : settingsList) {
			string configString;
			unordered_map<string, LPVOID> configValues;

			switch (configurationSetting) {
			case timerPointer:
				configString = "[Timer Pointer]";
				configValues.emplace("ThreadOffset", &threadOffset);
				configValues.emplace("Offset0", &offsets[0]);
				configValues.emplace("Offset1", &offsets[1]);
				configValues.emplace("Offset2", &offsets[2]);
				configValues.emplace("Offset3", &offsets[3]);
				configValues.emplace("Offset4", &offsets[4]);
				break;

			case songsFolderPath:
				configString = "[Songs Folder Path]";
				configValues.emplace("FolderPath", &songsPath);
				break;

			default:
				return FALSE;
			}


			fstream configFile;
			configFile.open("Data\\configFile.cfg", fstream::in || fstream::out);

			while (!configFile.eof() && readLine.find(configString) == string::npos) {
				getline(configFile, readLine);
			}

			if (readLine.empty() && configFile.eof())
				return FALSE;


			for (unsigned int i = 0; i < configValues.size(); i++) {
				getline(configFile, readLine);
				string configSetting = readLine.substr(0,
					MIN(readLine.find_first_of(" "), readLine.find_first_of(":")) - 1);

				UINT pos = readLine.find_first_of(":") + 1U;
				UINT valuePos = readLine.at(pos) == ' ' ? pos + 1U : pos;

				if (configurationSetting == 'i') {
					int intValue;
					if (readLine.at(valuePos) == '-') {
						intValue = stoi(readLine.substr(valuePos + 1));
						intValue *= -1;
					}
					else
						intValue = stoi(readLine.substr(valuePos));

					int *configValue = (int*)configValues.at(configSetting);
					*configValue = intValue;
				}
				else if (configurationSetting == 's') {
					string stringValue = readLine.substr(valuePos);

					wstring *configValue = (wstring*)configValues.at(configSetting);
					*configValue = wstring(stringValue.begin(), stringValue.end());
				}
				else
					return FALSE;
			}

			configFile.close();
		}

		return TRUE;
	}
	catch (const exception &e) {
		/* EventLog */	fprintf(wEventLog, ("[ERROR]  Couldn't read from configuration file!\n            with error: " + (string)e.what()).c_str());
		fflush(wEventLog);
		return FALSE;
	}
	return FALSE;
}

bool WriteToConfigFile(vector<string> configStrings) {
	try {
		FILE *wConfigFile = fopen("Data\\configFile.cfg", "w");

		for (string configString : configStrings) {
			fprintf(wConfigFile, (configString + "\n").c_str());
		}

		return TRUE;
	}
	catch (const exception &e) {
		/* EventLog */	fprintf(wEventLog, ("[ERROR]  Couldn't write to configuration file!\n            with error: " + (string)e.what()).c_str());
		fflush(wEventLog);
		return FALSE;
	}
	return FALSE;
}