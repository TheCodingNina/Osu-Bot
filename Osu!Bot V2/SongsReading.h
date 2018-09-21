#pragma once

#include "stdafx.h"

#include "OsuBot.h"


float MapDifficultyRange(float difficulty, float min, float mid, float max) {
	if (difficulty > 5.0f) return mid + (max - mid)*(difficulty - 5.0f) / 5.0f;
	if (difficulty < 5.0f) return mid - (mid - min)*(5.0f - difficulty) / 5.0f;
	return mid;
}

void ParseSong(LPCTSTR songPath) {
	wfstream path; path.open(songPath, wfstream::in);
	bool general = false;
	bool difficulty = false;
	bool timing = false;
	bool hits = false;
	bool beatDivisor = false;
	while (!path.eof()) {
		wstring str;
		getline(path, str, path.widen(L'\n'));
		if (str.find(L"[General]") != wstring::npos) {
			general = true;
		}
		else if (general) {
			if (str.find(L"StackLeniency") != wstring::npos) {
				stackLeniency = stof(str.substr(str.find(L':') + 1U));
			}
			else if (str.find(L':') == wstring::npos) {
				general = false;
			}
		}
		else if (str.find(L"[Editor]") != wstring::npos) {
			beatDivisor = true;
		}
		else if (beatDivisor) {
			if (str.find(L"BeatDivisor") != wstring::npos) {
				beatMapDivider = stof(str.substr(str.find(L':') + 1U));
			}
			else if (str.find(L':') == wstring::npos) {
				beatDivisor = false;
			}
		}
		else if (str.find(L"[Difficulty]") != wstring::npos) {
			difficulty = true;
		}
		else if (difficulty) {
			if (str.find(L"OverallDifficulty") != wstring::npos) {
				overallDifficulty = stof(str.substr(str.find(L':') + 1U));
			}
			else if (str.find(L"CircleSize") != wstring::npos) {
				circleSize = stof(str.substr(str.find(L':') + 1U));
			}
			else if (str.find(L"SliderMultiplier") != wstring::npos) {
				sliderMultiplier = stof(str.substr(str.find(L':') + 1U));
			}
			else if (str.find(L"SliderTickRate") != wstring::npos) {
				sliderTickRate = stof(str.substr(str.find(L':') + 1U));
			}
			else if (str.find(L':') == wstring::npos) {
				difficulty = false;
			}
		}
		else if (str.find(L"[TimingPoints]") != wstring::npos) {
			timing = true;
		}
		else if (timing) {
			if (str.find(L',') == wstring::npos) {
				timing = false;
			}
			else {
				TimingPoint TP = TimingPoint(str);
				timingPoints.push_back(TP);
			}
		}
		else if (str.find(L"[HitObjects]") != wstring::npos) {
			hits = true;
		}
		else if (hits) {
			if (str.find(L',') == wstring::npos) {
				hits = false;
			}
			else {
				HitObject HO = HitObject(str, &timingPoints, sliderMultiplier, sliderTickRate);
				hitObjects.push_back(HO);
			}
		}
	}
	path.close();

	float preEmpt = MapDifficultyRange(overallDifficulty, 1800.0f, 1200.0f, 450.0f);

	stackOffset = ((512.0f / 16.0f) * (1.0f - 0.7f * (circleSize - 5.0f) / 5.0f) / 10.0f) / circleSize;

	for (int i = hitObjects.size() - 1; i > 0; i--) {
		HitObject* hitObjectI = &hitObjects[i];
		if (hitObjectI->getStack() != 0 || hitObjectI->getHitType() == HIT_SPINNER) {
			continue;
		}

		for (int n = i - 1; n >= 0; n--) {
			HitObject* hitObjectN = &hitObjects[n];
			if (hitObjectN->getHitType() == HIT_SPINNER) {
				continue;
			}

			// check if in range stack calculation
			float timeI = hitObjectI->startTime - preEmpt * stackLeniency;
			float timeN = float(hitObjectN->getHitType() == HIT_SLIDER ? hitObjects[n].endTime : hitObjectN->startTime);
			if (timeI > timeN)
				break;

			if (hitObjectN->getHitType() == HIT_SLIDER) {
				vec2f p1 = hitObjects[i].getStartPos();
				vec2f p2 = hitObjects[n].getEndPos();
				float distance = (p2 - p1).length();

				// check if hit object part of this stack
				if (stackLeniency > 0.0f) {
					float circleRadius = pow(circleSize, -1) * 200.0f;
					if (distance < circleRadius) {
						int offset = hitObjectI->getStack() - hitObjectN->getStack() + 1;
						for (int j = n + 1; j <= i; j++) {
							HitObject* hitObjectJ = &hitObjects[j];
							p1 = hitObjectJ->getStartPos();
							distance = (p2 - p1).length();
							//cout << offset;
							// hit object below slider end
							if (distance < circleRadius)
								hitObjectJ->setStack(hitObjectJ->getStack() - offset);
						}
						break;  // slider end always start of the stack: reset calculation
					}
				}
				else {
					break;
				}
			}
			auto distance = (
				hitObjectN->getStartPos() -
				hitObjectI->getStartPos()
				).length();
			if (distance < stackLeniency) {
				hitObjectN->setStack(hitObjectI->getStack() + 1);
				hitObjectI = hitObjectN;
			}
		}
	}
	for (int i = hitObjects.size() - 1; i > 0; i--) {
		int n = i;
		/* We should check every note which has not yet got a stack.
		* Consider the case we have two interwound stacks and this will make sense.
		*
		* o <-1      o <-2
		*  o <-3      o <-4
		*
		* We first process starting from 4 and handle 2,
		* then we come backwards on the i loop iteration until we reach 3 and handle 1.
		* 2 and 1 will be ignored in the i loop because they already have a stack value.
		*/

		HitObject *objectI = &hitObjects[i];

		if (objectI->stackId != 0 || objectI->getHitType() == HIT_SPINNER) continue;

		/* If this object is a hitcircle, then we enter this "special" case.
		* It either ends with a stack of hitcircles only, or a stack of hitcircles that are underneath a slider.
		* Any other case is handled by the "is Slider" code below this.
		*/
		if (objectI->endTime == 0) {
			while (--n >= 0) {
				HitObject* objectN = &hitObjects[n];

				if (objectN->getHitType() == HIT_SPINNER) continue;

				//HitObjectSpannable spanN = objectN as HitObjectSpannable;
				float timeI = objectI->startTime - preEmpt * stackLeniency;
				float timeN = static_cast<float>(objectN->getHitType() == HIT_SLIDER ? objectN->endTime : objectN->startTime);
				if (timeI > timeN)
					break;

				/* This is a special case where hticircles are moved DOWN and RIGHT (negative stacking) if they are under the *last* slider in a stacked pattern.
				*    o==o <- slider is at original location
				*        o <- hitCircle has stack of -1
				*         o <- hitCircle has stack of -2
				*/
				if (objectN->endTime != 0 && (objectI->getEndPos() - objectN->startPosition).length() < STACK_LENIENCE) {
					int offset = objectI->stackId - objectN->stackId + 1;
					for (int j = n + 1; j <= i; j++) {
						//For each object which was declared under this slider, we will offset it to appear *below* the slider end (rather than above).
						if ((hitObjects[j].startPosition - objectN->getEndPos()).length() < STACK_LENIENCE)
							hitObjects[j].stackId -= offset;
					}

					//We have hit a slider.  We should restart calculation using this as the new base.
					//Breaking here will mean that the slider still has StackCount of 0, so will be handled in the i-outer-loop.
					break;
				}

				if ((objectI->startPosition - objectN->startPosition).length() < STACK_LENIENCE) {
					//Keep processing as if there are no sliders.  If we come across a slider, this gets cancelled out.
					//NOTE: Sliders with start positions stacking are a special case that is also handled here.

					objectN->stackId = objectI->stackId + 1;
					objectI = objectN;
				}
			}
		}
		else if (objectI->getHitType() == HIT_SLIDER) {
			/* We have hit the first slider in a possible stack.
			* From this point on, we ALWAYS stack positive regardless.
			*/
			while (--n >= 0) {
				HitObject* objectN = &hitObjects[n];

				if (objectN->getHitType() == HIT_SPINNER) continue;

				//HitObjectSpannable spanN = objectN as HitObjectSpannable;

				if (objectI->startTime - (preEmpt * stackLeniency) > objectN->startTime)
					break;

				if ((objectI->startPosition - (objectN->endTime != 0 ? objectN->getEndPos() : objectN->startPosition)).length() < STACK_LENIENCE) {
					objectN->stackId = objectI->stackId + 1;
					objectI = objectN;
				}
			}
		}
	}

	if (path)
		/* EventLog */	fwprintf(wEventLog, L"[EVENT]  Beatmap parsed without major errors.\n");
}


bool OpenSongAuto(wstring title) {
	wchar_t charsToRemove[] = { L"?.\"" };
	wchar_t charsToRemoveDiff[] = { L"?<>" };
	wstring beatmapName;
	wstring difficultyName;
	vector<wstring> beatmapSets;
	int beatmapSetCount = 0;


	if (title.find(L"[") != wstring::npos) {
		difficultyName = title.substr(title.find_last_of(L"["));
	}

	beatmapName = title.substr(title.find(title.at(8)), title.find_last_of(L"[") - 9);

	for (unsigned int i = 0; i < beatmapName.size() - 1; i++) {
		switch (beatmapName.at(i)) {
		case L'<':
			beatmapName.at(i) = L'-';
		case L'>':
			beatmapName.at(i) = L'-';
		case L'*':
			beatmapName.at(i) = L'-';
		case L':':
			beatmapName.at(i) = L'_';
		}
	}

	for (unsigned int i = 0; i < wcslen(charsToRemove); i++) {
		beatmapName.erase(remove(beatmapName.begin(), beatmapName.end(), charsToRemove[i]), beatmapName.end());
	}

	for (unsigned int i = 0; i < difficultyName.size() - 1U; i++) {
		switch (difficultyName.at(i)) {
		case L'*':
			difficultyName.at(i) = L'_';
		case L':':
			difficultyName.at(i) = L'_';
		}
	}

	for (unsigned int i = 0; i < wcslen(charsToRemoveDiff); i++) {
		difficultyName.erase(remove(difficultyName.begin(), difficultyName.end(), charsToRemoveDiff[i]), difficultyName.end());
	}

	difficultyName += L".osu";


	DeleteFile(L"Data\\BMData.txt");
	/* EventLog */	fwprintf(wEventLog, L"[EVENT]  Cleared BMData from previous writing.\n");

	/* EventLog */	fwprintf(wEventLog, L"[EVENT]  Starting SFData reading...\n");

	wfstream rSFData; rSFData.open(L"Data\\SFData.txt", wfstream::in);

	wstring readLine;
	while (readLine != L"[Songs Folders]") {
		getline(rSFData, readLine);
	}

	while (rSFData) {
		getline(rSFData, readLine);
		size_t findPos = readLine.find(beatmapName);
		if (findPos != string::npos) {
			if (readLine.substr(findPos) == beatmapName) {
				/* EventLog */	fprintf(wEventLog, "[EVENT]  Starting BMData writing...\n");

			wfstream wBMData; wBMData.open(L"Data\\BMData.txt", wfstream::out | wfstream::app);
			wBMData << L"[BeatmapDifficulties]\n";

			wstring directory = wstring(songsPath.begin(), songsPath.end()) + L"\\" + readLine;

			for (auto beatmapFile : experimental::filesystem::directory_iterator(directory)) {
				wstringstream sstream; sstream << beatmapFile;

				wstring beatmapString = sstream.str();
				wBMData << wstring((beatmapString.begin() + directory.size() + 1U), beatmapString.end()) << endl;
			}

				beatmapSets.push_back(directory);

			wBMData << L"\n";

				wBMData.close();

			/* EventLog */	fwprintf(wEventLog, L"[EVENT]    Finished BMData writing.\n");
		}
	}
	rSFData.close();
	readLine.clear();

	/* EventLog */	fwprintf(wEventLog, L"[EVENT]    Finished SFData reading.\n");
	/* EventLog */	fwprintf(wEventLog, L"[EVENT]  Starting BMData reading...\n");

	wfstream rBMData; rBMData.open("Data\\BMData.txt", wfstream::in);

	getline(rBMData, readLine);

	while (rBMData) {
		getline(rBMData, readLine);
		if (readLine == L"[BeatmapDifficulties]") {
			getline(rBMData, readLine);
			beatmapSetCount++;
		}

		if (readLine.find(difficultyName) != wstring::npos) {
			beatmapPath = beatmapSets.at(beatmapSetCount) + L"\\" + readLine;
			displayBeatmapPath = readLine;

			rBMData.close();

			return TRUE;
		}
	}
	rBMData.close();

	/* EventLog */	fwprintf(wEventLog, L"[EVENT]    Finished BMData reading.\n");

	return FALSE;
}

bool OpenSongManual() {
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd2)))) {
		LPWSTR pathName = NULL; if (CoTaskMemAlloc(NULL) == NULL) return FALSE;
		DWORD dwOptions2;
		COMDLG_FILTERSPEC rgFilterSpec[] = { { L"osu! Beatmap", L"*.osu" } };
		pfd2->GetOptions(&dwOptions2);
		pfd2->SetOptions(dwOptions2 | FOS_STRICTFILETYPES);
		pfd2->SetFileTypes(1U, rgFilterSpec);
		pfd2->Show(NULL);
		try {
			if (SUCCEEDED(pfd2->GetResult(&psi2))) {
				if (psi2->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pathName) == S_OK) { beatmapPath = pathName; }
				else beatmapPath.clear();
				psi2->Release();
			}
			else beatmapPath.clear();
		}
		catch (...) {}
		pfd2->Release();
		CoTaskMemFree(pathName);
		CoUninitialize();
	}

	if (!beatmapPath.empty()) {
		displayBeatmapPath = beatmapPath.substr(beatmapPath.find_last_of('\\') + 1U);
		return TRUE;
	}
	else return FALSE;
}


bool OpenSongFolder() {
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
		LPWSTR pathName = NULL; if (CoTaskMemAlloc(NULL) == NULL) return FALSE;
		DWORD dwOptions;
		pfd->GetOptions(&dwOptions);
		pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
		pfd->Show(NULL);
		if (SUCCEEDED(pfd->GetResult(&psi))) {
			if (psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pathName) == S_OK) {
				songsPath = pathName;

				wfstream wSFData; wSFData.open(L"Data\\SFData.txt", wfstream::out);
				wSFData << L"[Songs Folders]\n";

				for (auto beatmapFolder : experimental::filesystem::directory_iterator(pathName)) {
					wstringstream sstream; sstream << beatmapFolder;

					wstring beatmapString = sstream.str();
					wSFData << wstring((beatmapString.begin() + songsPath.size() + 1U), beatmapString.end()) << endl;
				}
				wSFData.close();
			}
			else
				songsPath.clear();

			psi->Release();
		}
		else
			songsPath.clear();

		pfd->Release();
		CoTaskMemFree(pathName);
		CoUninitialize();
	}

	if (_wfopen(L"Data\\SFData.txt", L"r"))
		return TRUE;
	else if (songsPath.empty()) {
		if (ReadFromConfigFile(songsFolderPath)) {
			if (songsPath.compare(L"") != 0)
				return TRUE;
			else
				return FALSE;
		}
		return FALSE;
	}
	else return FALSE;
}


void SongFileCheck(bool songFileCheck, wstring selectedBy) {
	if (songFileCheck) {
		DrawTextToWindow(hWnd, displayBeatmapPath.c_str(), rectSongFile);

		statusText = L"Beatmap Successfully Selected!";
		DrawTextToWindow(hWnd, statusText, rectStatus);

		/* EventLog */	fwprintf(wEventLog, (L"[EVENT]  Beatmap successfully selected by " + selectedBy).c_str());

		hitObjects.clear();
		timingPoints.clear();

		ParseSong((beatmapPath.c_str()));

		SendMessage(hwndProgressBar, PBM_SETRANGE, NULL, MAKELPARAM(0, hitObjects.size() + 1));
		SendMessage(hwndProgressBar, PBM_SETSTEP, WPARAM(1), NULL);
		SendMessage(hwndProgressBar, PBM_SETPOS, WPARAM(0), NULL);
	}
	else {
		statusText = L"No Beatmap Selected!";
		DrawTextToWindow(hWnd, statusText, rectStatus);

		/* EventLog */	fwprintf(wEventLog, (L"[EVENT]  No Beatmap was selected by " + selectedBy).c_str());
	}
}
