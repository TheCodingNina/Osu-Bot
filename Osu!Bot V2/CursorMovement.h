#pragma once

#include "stdafx.h"

#include "OsuBot.h"


float HermiteInterp(float _X) {
	float	_Y0 = 0.1f,	// In target
			_Y1 = 0.0f,	// Start
			_Y2 = 1.0f,	// End
			_Y3 = 1.1f;	// Out target
	
	float	tension =  -0.2f,	// 1 : high, 0 : normal, -1 : low
			bias	=   0.3f;	// >0 : first segment, 0 : mid, <0 : next segment

	float a0, a1, a2, a3;
	float m0, m1;

	float _X2 = _X * _X;
	float _X3 = _X2 * _X;

	m0	= (_Y1 - _Y0) * (1.f + bias) * (1.f - tension) / 2.f;
	m0 += (_Y2 - _Y1) * (1.f - bias) * (1.f - tension) / 2.f;
	m1	= (_Y2 - _Y1) * (1.f + bias) * (1.f - tension) / 2.f;
	m1 += (_Y3 - _Y2) * (1.f - bias) * (1.f - tension) / 2.f;

	a0 =  2.f *	_X3 - 3.f * _X2 + 1.f;
	a1 =		_X3 - 2.f * _X2 + _X;
	a2 =		_X3 -		_X2;
	a3 = -2.f * _X3 + 3.f * _X2;

	float _Y = (a0 * _Y1 + a1 * m0 + a2 * m1 + a3 * _Y2);

	return CLAMP(0.f, _Y, 1.f);
}


vector<vec2f> FindControlPoints(vec2f vpP, vec2f vpB, vec2f vpE, vec2f vpN, int index) {
	vec2f
		d0 = vpP.cpy().sub(vpB),
		d1 = vpB.cpy().sub(vpE),
		d2 = vpE.cpy().sub(vpN);

	float
		l0 = d0.length(),
		l1 = d1.length(),
		l2 = d2.length();

	vec2f
		m0 = vpP.midPoint(pB),
		m1 = vpB.midPoint(pE),
		m2 = vpE.midPoint(pN);

	float
		amplifier0 = (atan2f(l2 / 480.f, 1.85f * (l2 / 960.f)) / ((40000.f / Amplifier) / l1)) + 1.f,
		amplifier1 = (atan2f(l1 / 480.f, 1.85f * (l1 / 960.f)) / ((40000.f / Amplifier) / l1)) + 1.f;

	vec2f
		cp0 = m1 + (pB - (m1 + (m0 - m1) * ((l1 * amplifier1) / (l0 + l1)))),
		cp1 = m0 + (pB - (m1 + (m0 - m1) * ((l1 * amplifier1) / (l0 + l1)))),
		cp2 = m2 + (pE - (m2 + (m1 - m2) * ((l2 * amplifier0) / (l1 + l2)))),
		cp3 = m1 + (pE - (m2 + (m1 - m2) * ((l2 * amplifier0) / (l1 + l2))));

	return index == 1 ? vector<vec2f>{ cp2, cp3 } : vector<vec2f> { cp0, cp1 };
}


vec2f FlowVectorPoint(vec2f vpP, vec2f vpB, vec2f vpE, vec2f vpN, vec2f unreferencedVec2f, int index) {
	UNREFERENCED_PARAMETER(unreferencedVec2f);
	return FindControlPoints(vpP, vpB, vpE, vpN, index).at(index);
}

vec2f PredictionVectorPoint(vec2f vpP, vec2f vpB, vec2f vpE, vec2f vpN, vec2f cp1, int unreferencedInt) {
	UNREFERENCED_PARAMETER(vpP);
	UNREFERENCED_PARAMETER(unreferencedInt);
	return vpN.midPoint(vpE).sub(vpN).mult((vpB - vpE).length() / (860.0f / Amplifier)).add(vpE).midPoint(cp1);
}


void MoveToCircle(vector<HitObject> *objects, const int nObject, function<vec2f(vec2f, vec2f, vec2f, vec2f, vec2f, int)> VectorPoint) {
	if (nObject == 0) {
		GetCursorPos(&cursorPoint);

		pBack = pP = pB = vec2f(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y));
	}
	else {
		if (objects->at(nObject - 1).getHitType() == HIT_SLIDER) {
			float tP = (objects->at(nObject - 1).getSliderTickCount() - 1.f) / objects->at(nObject - 1).getSliderTickCount();
			pP = vec2f((objects->at(nObject - 1).getPointByT(tP).x - stackOffset * objects->at(nObject - 1).getStack()) * multiplierX + osuWindowX, (objects->at(nObject - 1).getPointByT(tP).y - stackOffset * objects->at(nObject - 1).getStack()) * multiplierY + osuWindowY);
		}
		else if (nObject != 1) {
			pP = vec2f((objects->at(nObject - 2).getEndPos().x - stackOffset * objects->at(nObject - 2).getStack()) * multiplierX + osuWindowX, (objects->at(nObject - 2).getEndPos().y - stackOffset * objects->at(nObject - 2).getStack()) * multiplierY + osuWindowY);
		}

		GetCursorPos(&cursorPoint);
		pB = vec2f(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y));
	}

	pE = vec2f((objects->at(nObject).getStartPos().x - stackOffset * objects->at(nObject).getStack()) * multiplierX + osuWindowX, (objects->at(nObject).getStartPos().y - stackOffset * objects->at(nObject).getStack()) * multiplierY + osuWindowY);

	if (static_cast<UINT>(nObject + 1) >= objects->size()) {
		pN = vec2f(320.f * multiplierX + osuWindowX, 240.f * multiplierY + osuWindowY);
	}
	else if (objects->at(nObject).getHitType() == HIT_SLIDER) {
		float tickCount = ceilf(objects->at(nObject).getSliderTickCount());
		float tN = 1.f / (tickCount == 0.f ? 1.f : tickCount);

		vec2f pNT = objects->at(nObject).getPointByT(tN);
		pN = vec2f((pNT.x - stackOffset * objects->at(nObject).getStack()) * multiplierX + osuWindowX, (pNT.y - stackOffset * objects->at(nObject).getStack()) * multiplierY + osuWindowY);
	}
	else {
		pN = vec2f((objects->at(nObject + 1).getStartPos().x - stackOffset * objects->at(nObject + 1).getStack()) * multiplierX + osuWindowX, (objects->at(nObject + 1).getStartPos().y - stackOffset * objects->at(nObject + 1).getStack()) * multiplierY + osuWindowY);
	}


	bool interpBool = true;

	vec2f p1 = (pB - pBack).add(pB);

	vec2f p2;
	if ((pE - pB).length() < (1.f / circleSize) * 400.f) {
		p2 = FindControlPoints(pP, pB, pE, pN, 1).at(1);
		p1 = (pB - pBack).normalize().mult((pE - pB).length() / 2.f).add(pB);
		interpBool = false;
	}
	else if (objects->at(nObject).getHitType() == HIT_SLIDER) {
		p2 = FindControlPoints(pP, pB, pE, pN, 1).at(1);
	}
	else {
		p2 = VectorPoint(pP, pB, pE, pN, p1, 1);
	}

	vector<vec2f> pts {
		pB, p1, p2, pE
	};


	float dt = static_cast<float>(objects->at(nObject).getStartTime() - songTime);
	while (songTime <= objects->at(nObject).getStartTime() && songStarted) {
		float t = (dt - static_cast<float>(objects->at(nObject).getStartTime() - songTime)) / dt;
		t = interpBool ? HermiteInterp(t) : t;
		t = CLAMP(0.f, t, 1.f);

		pCursor = PolyBezier(pts, INT(pts.size()) - 1, 0, t);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		this_thread::sleep_for(chrono::milliseconds(1));
	}

	pBack = pts.at(pts.size() - 2);
}

void MoveToStandard(HitObject *objects) {
	GetCursorPos(&cursorPoint);

	pB = vec2f(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y));
	pE = vec2f((objects->getStartPos().x - stackOffset * objects->getStack()) * multiplierX + osuWindowX,
		(objects->getStartPos().y - stackOffset * objects->getStack())  * multiplierY + osuWindowY);

	float dt = static_cast<float>(objects->getStartTime() - songTime);

	while (songTime < objects->getStartTime() && songStarted) {
		float t = (dt - static_cast<float>(objects->getStartTime() - songTime)) / dt;
		pCursor = pB + t * (pE - pB);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));
		this_thread::sleep_for(chrono::milliseconds(1));
	}
}

void SliderStandard(HitObject *objects) {
	SendKeyPress(objects);

	while (songTime <= objects->getEndTime() && songStarted) {
		auto t = static_cast<float>(songTime - objects->getStartTime()) / objects->getSliderTime();

		vec2f vec = objects->getPointByT(t);
		pCursor = vec2f((((vec.x - objects->getStack() * stackOffset) * multiplierX) + osuWindowX),
			((vec.y - objects->getStack() * stackOffset) * multiplierY) + osuWindowY);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		this_thread::sleep_for(chrono::milliseconds(1));
	}

	SendKeyRelease(objects);
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
			pts.at(nPolyCount + 2) = pN.midPoint(pE).sub(pN).mult((pB - pE).length() / 860.0f).add(pE).midPoint(pts.at(nPolyCount + 1)); //FindControlPoints(pts.at(nPolyCount), pE, pN).at(1);
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
					pts.at(nPolyCount + 2) = pN.midPoint(pE).sub(pN).mult((pB - pE).length() / 860.0f).add(pE).midPoint(pts.at(nPolyCount + 1)); //FindControlPoints(pts.at(nPolyCount), pE, pN).at(1);
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
					pts.at(nPolyCount + 2) = pN.midPoint(pE).sub(pN).mult((pB - pE).length() / 860.0f).add(pE).midPoint(pts.at(nPolyCount + 1)); //FindControlPoints(pts.at(nPolyCount), pE, pN).at(1);
					pts.at(nPolyCount + cpCount) = pE;

					pP = pts.at(nPolyCount);
					nPolyCount += cpCount;
				}
			}
		}
	}

	SendKeyPress(&objects->at(nObject));

	while (songTime < objects->at(nObject).getEndTime() && songStarted) {
		float tickCount = ceilf(objects->at(nObject).getSliderTickCount());

		float t = static_cast<float>(songTime - objects->at(nObject).getStartTime()) / static_cast<float>(objects->at(nObject).getSliderTime());

		float T = t * tickCount - nPoly;
		if (T > 1.f && static_cast<float>(nPoly + 1) < objects->at(nObject).getSliderRepeatCount() * tickCount) {
			++nPoly;
			T = t * tickCount - nPoly;
		}
		T = CLAMP(0.f, HermiteInterp(T), 1.f);

		pCursor = PolyBezier(pts, cpCount, nPoly, T);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		this_thread::sleep_for(chrono::milliseconds(1));
	}

	SendKeyRelease(&objects->at(nObject));

	pBack = pts.at(pts.size() - 2);
	pP = pts.at(pts.size() - 1 - cpCount);
}

void SpinnerStandard(HitObject *objects) {
	vec2f center(objects->getStartPos().x * multiplierX + osuWindowX,
		(objects->getStartPos().y + 6.f) * multiplierY + osuWindowY);
	float angle = TWO_PI;
	float radius = (1.f / circleSize) * 450.f;

	SendKeyPress(objects);

	while (songTime < objects->getEndTime() && songStarted) {
		float t = static_cast<float>(songTime - objects->getStartTime()) / static_cast<float>(objects->getEndTime() - objects->getStartTime());
		t *= 3.f; if (t >= 1.f) t = 1.f;
		float radiusT = radius * t;

		pCursor = CirclePoint(center, radiusT, angle);

		SetCursorPos(static_cast<int>(pCursor.x), static_cast<int>(pCursor.y));

		angle -= static_cast<float>(M_PI / 20.f);

		this_thread::sleep_for(chrono::milliseconds(1));
	}

	SendKeyRelease(objects);
}
