#pragma once

#include "stdafx.h"

#include "OsuBot.h"


float __cdecl Interp(float _X) {
	return 0.5f * (1.f + (1.08f * sinf(0.75f * static_cast<float>(M_PI) * (_X - 0.5f))));
}


vector<vec2f> FindControlPoints(vec2f vec1, vec2f vec2, vec2f vec3, float Amplifier = 240.f) {
	vec2f
		d1(vec1.x - vec2.x, vec1.y - vec2.y),
		d2(vec2.x - vec3.x, vec2.y - vec3.y);

	float
		l1 = sqrtf(d1.x * d1.x + d1.y * d1.y),
		l2 = sqrtf(d2.x * d2.x + d2.y * d2.y);

	vec2f
		m1 = (vec1 + vec2) / 2.f,
		m2 = (vec2 + vec3) / 2.f;

	vec2f
		p0 = m2 + (vec2 - (m2 + (m1 - m2) * ((l2 * 1.85f) / (l1 + l2)))),
		p1 = m1 + (vec2 - (m2 + (m1 - m2) * ((l2 * ((atan2f(l2 / 480.f, 1.85f * (l2 / 960.f)) / (40000.f / Amplifier)) + 1.f)) / (l1 + l2))));

	return { p0, p1 };
}


void MoveToStandard(HitObject *hitObject) {
	GetCursorPos(&cursorPoint);

	pB = vec2f(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y));
	pE = vec2f((hitObject->getStartPos().x - stackOffset * hitObject->getStack()) * multiplierX + osuWindowX,
		(hitObject->getStartPos().y - stackOffset * hitObject->getStack())  * multiplierY + osuWindowY);

	float dt = static_cast<float>(hitObject->getStartTime() - songTime);

	while (songTime < hitObject->getStartTime() && songStarted) {
		float t = (dt - static_cast<float>(hitObject->getStartTime() - songTime)) / dt;
		pCursor = pB + t * (pE - pB);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));
		this_thread::sleep_for(chrono::microseconds(100));
	}

	if (hitObject->getHitType() == HIT_CIRCLE) {
		SendKeyPress(hitObject);
		this_thread::sleep_for(chrono::milliseconds(12));
		SendKeyRelease(hitObject);
	}
}

void MoveToFlowing(vector<HitObject>* objects, const int nObject) {
	GetCursorPos(&cursorPoint);

	vec2f p0 = vec2f((pE - pBack) + pE);

	pP = nObject == 0 ? vec2f(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y)) : vec2f((hitObjects->at(nObject - 1).getEndPos().x - stackOffset * hitObjects->at(nObject - 1).getStack()) * multiplierX + osuWindowX,
		(hitObjects->at(nObject - 1).getEndPos().y - stackOffset * hitObjects->at(nObject - 1).getStack()) * multiplierY + osuWindowY);
	pB = vec2f(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y));
	pE = vec2f((objects->at(nObject).getStartPos().x - stackOffset * objects->at(nObject).getStack()) * multiplierX + osuWindowX,
		(objects->at(nObject).getStartPos().y - stackOffset * objects->at(nObject).getStack()) * multiplierY + osuWindowY);
	pN = nObject + 1 == static_cast<signed int>(objects->size()) ? pE : vec2f((objects->at(nObject + 1).getStartPos().x - stackOffset * objects->at(nObject + 1).getStack()) * multiplierX + osuWindowX,
		(objects->at(nObject + 1).getStartPos().y - stackOffset * objects->at(nObject + 1).getStack()) * multiplierY + osuWindowY);

	if (UINT(nObject + 1) < objects->size() && objects->at(nObject + 1).getHitType() == HIT_SLIDER) {
		float tN = 1.f / ceilf(objects->at(nObject + 1).getSliderTickCount());
		pN = vec2f(((objects->at(nObject + 1).getPointByT(tN).x - objects->at(nObject + 1).getStack() * stackOffset) * multiplierX) + osuWindowX,
			((objects->at(nObject + 1).getPointByT(tN).y - objects->at(nObject + 1).getStack() * stackOffset) * multiplierY) + osuWindowY);
	}

	if (nObject == 0)
		p0 = FindControlPoints(pP, pB, pE).at(0);



	/*if ((pN - pE).length() < (1.f / circleSize) * 400.f && (pE - pB).length() < (1.f / circleSize) * 400.f) {
		p0 = vec2f((pE - pBack).dev((pN - pE).length() * ((1.f / circleSize) * 400.f) + 1.f) + pE);
	}*/
	float delta = 0.f;
	if (nObject + 1 < static_cast<signed int>(hitObjects->size()))
		delta = static_cast<float>(hitObjects->at(nObject + 1).getStartTime() - hitObjects->at(nObject).getStartTime());
	if ((pE - pB).length() < (1.f / circleSize) * 300.f && delta <= 400.f) {
		p0 = vec2f((pE - pBack).dev((pE - pBack).length() * (1.f / circleSize) * 400.f + 1.f) + pE);
	}

	vec2f p1 = FindControlPoints(pB, pE, pN, (pE - pN).length()).at(1);

	vector<vec2f> pts {
		pB, p0, p1, pE
	};

	float dt = static_cast<float>(objects->at(nObject).getStartTime() - songTime);
	while (songTime < objects->at(nObject).getStartTime() && songStarted) {
		float t = (dt - static_cast<float>(objects->at(nObject).getStartTime() - songTime)) / dt;

		pCursor = PolyBezier(pts, pts.size() - 1, 0, Interp(t));

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		this_thread::sleep_for(chrono::microseconds(100));
	}

	if (objects->at(nObject).getHitType() == HIT_CIRCLE) {
		SendKeyPress(&objects->at(nObject));
		this_thread::sleep_for(chrono::milliseconds(12));
		SendKeyRelease(&objects->at(nObject));
	}

	pBack = pts.at(pts.size() - 2);
}

void SliderStandard(HitObject *hitObject) {
	SendKeyPress(hitObject);

	while (songTime <= hitObject->getEndTime() && songStarted) {
		auto t = static_cast<float>(songTime - hitObject->getStartTime()) / hitObject->getSliderTime();

		vec2f vec = hitObject->getPointByT(t);
		pCursor = vec2f((((vec.x - hitObject->getStack() * stackOffset) * multiplierX) + osuWindowX),
			((vec.y - hitObject->getStack() * stackOffset) * multiplierY) + osuWindowY);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		this_thread::sleep_for(chrono::microseconds(100));
	}

	SendKeyRelease(hitObject);
}

void SliderFlowing(vector<HitObject> *objects, const int nObject) {
	GetCursorPos(&cursorPoint);

	int cpCount = 3;

	int nPolyCount = 0;
	int nPolyCountReverse;
	int nPoly = 0;
	vector<vec2f> pts;
	pts.resize(static_cast<unsigned int>(ceilf(objects->at(nObject).getSliderTickCount()) * objects->at(nObject).getSliderRepeatCount() * static_cast<float>(cpCount)) + (UINT)1);

	vec2f pPBack = pP;
	vec2f pNBack = pN;

	if (objects->at(nObject).getSliderRepeatCount() == 1.f) {
		for (float i = 0.f; i < objects->at(nObject).getSliderTickCount(); i++) {
			float tB = i / objects->at(nObject).getSliderTickCount();
			float tE = (i + 1.f) > objects->at(nObject).getSliderTickCount() ? 1.f : (i + 1.f) / objects->at(nObject).getSliderTickCount();
			float tN = (i + 2.f) / objects->at(nObject).getSliderTickCount();

			pN = pNBack;
			if (tN < 1.f) {
				pN = vec2f(((objects->at(nObject).getPointByT(tN).x - objects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
					((objects->at(nObject).getPointByT(tN).y - objects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);
			}

			if (nPolyCount != 0) {
				pts.at(nPolyCount + 1) = vec2f((pE - pts.at(nPolyCount - cpCount + 2)) + pE);
			}

			pE = vec2f(((objects->at(nObject).getPointByT(tE).x - objects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
				((objects->at(nObject).getPointByT(tE).y - objects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);

			if (nPolyCount == 0) {
				pB = vec2f(((objects->at(nObject).getPointByT(tB).x - objects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
					((objects->at(nObject).getPointByT(tB).y - objects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);
				pts.at(nPolyCount) = pB;

				//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
				pts.at(nPolyCount + 1) = vec2f((pts.at(nPolyCount) - pBack) + pts.at(nPolyCount));
			}

			//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
			pts.at(nPolyCount + 2) = FindControlPoints(pts.at(nPolyCount), pE, pN).at(1);
			pts.at(nPolyCount + cpCount) = pE;

			pP = pts.at(nPolyCount);
			nPolyCount += cpCount;
		}
	}
	else {
		for (int repeated = 0; static_cast<float>(repeated) < objects->at(nObject).getSliderRepeatCount(); repeated++) {
			if (repeated % 2 == 0) {
				for (float i = 0.f; i < objects->at(nObject).getSliderTickCount(); i++) {
					float tB = i / objects->at(nObject).getSliderTickCount();
					float tE = (i + 1.f) > objects->at(nObject).getSliderTickCount() ? 1.f : (i + 1.f) / objects->at(nObject).getSliderTickCount();
					float tN = (i + 2.f) / objects->at(nObject).getSliderTickCount();

					pN = pNBack;
					if (tN < objects->at(nObject).getSliderRepeatCount()) {
						pN = vec2f(((objects->at(nObject).getPointByT(tN).x - objects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
							((objects->at(nObject).getPointByT(tN).y - objects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);
					}

					if (nPolyCount != 0) {
						pts.at(nPolyCount + 1) = vec2f((pE - pts.at(nPolyCount - cpCount + 2)) + pE);
					}

					pE = vec2f(((objects->at(nObject).getPointByT(tE).x - objects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
						((objects->at(nObject).getPointByT(tE).y - objects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);

					if (nPolyCount == 0) {
						pB = vec2f(((objects->at(nObject).getPointByT(tB).x - objects->at(nObject).getStack() * stackOffset) * multiplierX) + osuWindowX,
							((objects->at(nObject).getPointByT(tB).y - objects->at(nObject).getStack() * stackOffset) * multiplierY) + osuWindowY);
						pts.at(nPolyCount) = pB;

						//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
						pts.at(nPolyCount + 1) = vec2f((pts.at(nPolyCount) - pBack) + pts.at(nPolyCount));
					}

					//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
					pts.at(nPolyCount + 2) = FindControlPoints(pts.at(nPolyCount), pE, pN).at(1);
					pts.at(nPolyCount + cpCount) = pE;

					pP = pts.at(nPolyCount);
					nPolyCount += cpCount;
				}
			}
			if (repeated % 2 != 0) {
				nPolyCountReverse = nPolyCount;
				for (float i = 0.f; i < objects->at(nObject).getSliderTickCount(); i++) {
					nPolyCountReverse -= cpCount;

					pts.at(nPolyCount + 1) = vec2f((pE - pts.at(nPolyCount - cpCount + 2)) + pE);

					pP = pts.at(nPolyCount);
					pE = pts.at(nPolyCountReverse);

					if ((i + 1.f) / objects->at(nObject).getSliderTickCount() >= objects->at(nObject).getSliderRepeatCount()) {
						pN = pNBack;
					}
					else if (nPolyCountReverse - cpCount > 0) {
						pN = pts.at(nPolyCountReverse - cpCount);
					}
					else {
						pN = pPBack;
					}

					//pts.at(nPolyCount + 1) = FindControlPoints(pP, pts.at(nPolyCount), pE).at(0);
					pts.at(nPolyCount + 2) = FindControlPoints(pts.at(nPolyCount), pE, pN).at(1);
					pts.at(nPolyCount + cpCount) = pE;

					pP = pts.at(nPolyCount);
					nPolyCount += cpCount;
				}
			}
		}
	}

	SendKeyPress(&objects->at(nObject));

	while (songTime < objects->at(nObject).getEndTime() && songStarted) {
		float t = static_cast<float>(songTime - objects->at(nObject).getStartTime()) / static_cast<float>(objects->at(nObject).getSliderTime());

		float T = t * ceilf(hitObjects->at(nObject).getSliderTickCount()) - nPoly;
		if (T > 1.f && static_cast<float>(nPoly + 1) < hitObjects->at(nObject).getSliderRepeatCount() * hitObjects->at(nObject).getSliderTickCount()) {
			++nPoly;
			T = t * ceilf(hitObjects->at(nObject).getSliderTickCount()) - nPoly;
			/* EventLog */  //fprintf(wEventLog, (to_string(nObject).append(" : ").append(to_string(nPoly)).append(",").append(to_string(t)).append("\n")).c_str()); fflush(wEventLog);
		}

		if (T > 1.f) T = 1.f;
		else if (T < 0.f) T = 0.f;

		pCursor = PolyBezier(pts, cpCount, nPoly, Interp(T));

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		this_thread::sleep_for(chrono::microseconds(100));
	}

	SendKeyRelease(&objects->at(nObject));

	pBack = pts.at(pts.size() - 2);
	pP = pts.at(pts.size() - 1 - cpCount);
}

void SpinnerStandard(HitObject *hitObject) {
	vec2f center(hitObject->getStartPos().x * multiplierX + osuWindowX,
		(hitObject->getStartPos().y + 6.f) * multiplierY + osuWindowY);
	float angle = TWO_PI;
	float radius = (1.f / circleSize) * 450.f;

	SendKeyPress(hitObject);

	while (songTime < hitObject->getEndTime() && songStarted) {
		float t = static_cast<float>(songTime - hitObject->getStartTime()) / static_cast<float>(hitObject->getEndTime() - hitObject->getStartTime());
		t *= 3.f; if (t >= 1.f) t = 1.f;
		float radiusT = radius * t;

		pCursor = CirclePoint(center, radiusT, angle);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		angle -= static_cast<float>(M_PI / 30.f);

		this_thread::sleep_for(chrono::microseconds(100));
	}

	SendKeyRelease(hitObject);
}
