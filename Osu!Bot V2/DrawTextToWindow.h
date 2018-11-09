#pragma once
//
//   FUNCTION: DrawTextToWindow(HWND, wstring, RECT)
//
//   PURPOSE: Draw and Re-draw Text to the given HWND.
//
//   COMMENTS:
//
//        In this function, we draw or re-draw the text given in a
//		  RECT selected by the message in the given HWND.
//
static void DrawTextToWindow(HDC hdc, std::wstring str, RECT rect) {
	FillRect(hdc, &rect, HBRUSH(GetStockObject(WHITE_BRUSH)));

	DrawText(
		hdc,
		str.c_str(),
		(UINT)str.size(),
		&rect,
		DT_LEFT | DT_WORDBREAK
	);
}
static void DrawTextToWindow(HWND hWnd, std::wstring str, RECT rect) {
	HDC hdc = GetDC(hWnd);
	DrawTextToWindow(hdc, str, rect);
}
