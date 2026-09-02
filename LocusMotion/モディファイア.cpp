#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>
#include "Direct2D.h"
#include "Dialog.h"
#include <algorithm>
#include <string>

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

// EDITの入力値を保存して破棄する共通関数
void SaveAndCloseEdit(HWND hEdit) {
	if (!IsWindow(hEdit)) return;

	double* pVal = reinterpret_cast<double*>(GetPropW(hEdit, L"VALUE_PTR"));
	if (pVal) {
		wchar_t buf[256] = { 0 };
		GetWindowTextW(hEdit, buf, 256);
		*pVal = static_cast<double>(_wtof(buf));
		RemovePropW(hEdit, L"VALUE_PTR");
	}

	HWND hParent = GetParent(hEdit);
	DestroyWindow(hEdit);

	if (hParent && IsWindow(hParent)) {
		InvalidateRect(hParent, NULL, FALSE);
	}
}

// サブクラス化プロシージャ
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (uMsg) {
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;

	case WM_KEYDOWN:
		if (wParam == VK_RETURN) {
			SaveAndCloseEdit(hWnd);
			return 0;
		}
		else if (wParam == VK_ESCAPE) {
			RemovePropW(hWnd, L"VALUE_PTR");
			DestroyWindow(hWnd);
			return 0;
		}
		break;

	case WM_KILLFOCUS:
		SaveAndCloseEdit(hWnd);
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, EditSubclassProc, uIdSubclass);
		break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// EDITコントロール生成関数
HWND CreateInPlaceEdit(HWND hParent, D2D1_RECT_F rect, const std::wstring& currentText, UINT controlID, double* pValueTarget = nullptr) {
	int x = static_cast<int>(rect.left);
	int y = static_cast<int>(rect.top);
	int width = static_cast<int>(rect.right - rect.left);
	int height = static_cast<int>(rect.bottom - rect.top);
	
	HWND hEdit = CreateWindowExW(
		0, L"EDIT", currentText.c_str(),
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER,
		x, y, width, height,
		hParent, (HMENU)(UINT_PTR)controlID, GetModuleHandle(NULL), NULL
	);

	if (hEdit) {
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
		SendMessage(hEdit, EM_SETSEL, 0, -1);

		if (pValueTarget) {
			SetPropW(hEdit, L"VALUE_PTR", reinterpret_cast<HANDLE>(pValueTarget));
		}

		SetWindowSubclass(hEdit, EditSubclassProc, 0, 0);
		SetFocus(hEdit);
	}
	return hEdit;
}

using Microsoft::WRL::ComPtr;

struct MODMENU {
	MODIFIER Modifier;

	bool Open = false;
	float OpenAnime = 0.0f;
	bool Hover = false;
	float HoverAnime = 0.0f;
	bool Disable = false;
	bool DisableHover = false;
	float DisableHoverAnime = 0.0f;
	bool DeleteHover = false;
	float DeleteHoverAnime = 0.0f;
};

#define ID_EDIT_INTERVAL 1001

bool MODIFIER::UpdataParam(D2D1_RECT_F Rect, CUR_MOD* Cursor) {
	bool Return = false;
	switch (Mode) {
	case 1: {
		D2D1_RECT_F ParamInterval = D2D1::RectF(Rect.left + 80.0f, Rect.top + 10.0f, Rect.right - 90.0f, Rect.top + 10.0f + 25.0f);
		D2D1_RECT_F ParamIntervalUnit = D2D1::RectF(ParamInterval.right + 5.0f, ParamInterval.top, Rect.right - 10.0f, ParamInterval.bottom);
		D2D1_RECT_F ParamTime = D2D1::RectF(ParamInterval.left, ParamInterval.bottom + 10.0f, ParamInterval.right, ParamInterval.bottom + 10.0f + 25.0f);
		D2D1_RECT_F ParamTimeUnit = D2D1::RectF(ParamTime.right + 5.0f, ParamTime.top, Rect.right - 10.0f, ParamTime.bottom);

		ParamHover[0] = Cursor->RectCheck(ParamInterval);
		ParamHover[1] = Cursor->RectCheck(ParamIntervalUnit);
		ParamHover[2] = Cursor->RectCheck(ParamTime);
		ParamHover[3] = Cursor->RectCheck(ParamTimeUnit);

		if (Cursor->click && ParamHover[0]) {
			HWND hExisting = GetDlgItem(Cursor->hwnd, ID_EDIT_INTERVAL);
			if (!hExisting) {
				wchar_t valStr[32];
				swprintf_s(valStr, L"%.2f", Param[0]);
				CreateInPlaceEdit(Cursor->hwnd, ParamInterval, valStr, ID_EDIT_INTERVAL, &Param[0]);
			}
			Cursor->click = false;
			Return = true;
		}
		if (Cursor->click && ParamHover[1]) {
			HMENU hMenu = CreatePopupMenu();
			if (hMenu) {
				AppendMenuW(hMenu, MF_STRING, 101, L"秒");
				AppendMenuW(hMenu, MF_STRING, 102, L"ミリ秒");
				AppendMenuW(hMenu, MF_STRING, 103, L"フレーム");
				AppendMenuW(hMenu, MF_STRING, 104, L"FPS");
				AppendMenuW(hMenu, MF_STRING, 105, L"BPM");

				POINT pt = { (LONG)ParamIntervalUnit.left, (LONG)ParamIntervalUnit.bottom };
				if (Cursor->hwnd && IsWindow(Cursor->hwnd)) {
					ClientToScreen(Cursor->hwnd, &pt);
				}

				SetForegroundWindow(Cursor->hwnd);
				int cmd = TrackPopupMenu(
					hMenu,
					TPM_RIGHTBUTTON | TPM_RETURNCMD,
					pt.x, pt.y,
					0, Cursor->hwnd, NULL
				);
				DestroyMenu(hMenu);

				switch (cmd) {
				case 101: {
					Param[0] = ChengeUnit(Param[1], 0, Param[0]);
					Param[1] = 0.0;
					break;
				}
				case 102: {
					Param[0] = ChengeUnit(Param[1], 1, Param[0]);
					Param[1] = 1.0;
					break;
				}
				case 103: {
					Param[0] = ChengeUnit(Param[1], 2, Param[0]);
					Param[1] = 2.0;
					break;
				}
				case 104: {
					Param[0] = ChengeUnit(Param[1], 3, Param[0]);
					Param[1] = 3.0;
					break;
				}
				case 105: {
					Param[0] = ChengeUnit(Param[1], 4, Param[0]);
					Param[1] = 4.0;
					break;
				}
				}
			}
			Cursor->x = 0.0f;
			Cursor->y = 0.0f;
			Cursor->click = false;
			Return = true;
		}
		if (Cursor->click && ParamHover[2]) {
			HWND hExisting = GetDlgItem(Cursor->hwnd, ID_EDIT_INTERVAL);
			if (!hExisting) {
				wchar_t valStr[32];
				swprintf_s(valStr, L"%.2f", Param[2]);
				CreateInPlaceEdit(Cursor->hwnd, ParamTime, valStr, ID_EDIT_INTERVAL, &Param[2]);
			}
			Cursor->click = false;
			Return = true;
		}
		if (Cursor->click && ParamHover[3]) {
			HMENU hMenu = CreatePopupMenu();
			if (hMenu) {
				AppendMenuW(hMenu, MF_STRING, 101, L"秒");
				AppendMenuW(hMenu, MF_STRING, 102, L"ミリ秒");
				AppendMenuW(hMenu, MF_STRING, 103, L"フレーム");

				POINT pt = { (LONG)ParamTimeUnit.left, (LONG)ParamTimeUnit.bottom };
				if (Cursor->hwnd && IsWindow(Cursor->hwnd)) {
					ClientToScreen(Cursor->hwnd, &pt);
				}

				SetForegroundWindow(Cursor->hwnd);
				int cmd = TrackPopupMenu(
					hMenu,
					TPM_RIGHTBUTTON | TPM_RETURNCMD,
					pt.x, pt.y,
					0, Cursor->hwnd, NULL
				);
				DestroyMenu(hMenu);

				switch (cmd) {
				case 101: {
					Param[2] = ChengeUnit(Param[3], 0, Param[2]);
					Param[3] = 0.0;
					break;
				}
				case 102: {
					Param[2] = ChengeUnit(Param[3], 1, Param[2]);
					Param[3] = 1.0;
					break;
				}
				case 103: {
					Param[2] = ChengeUnit(Param[3], 2, Param[2]);
					Param[3] = 2.0;
					break;
				}
				}
			}
			Cursor->x = 0.0f;
			Cursor->y = 0.0f;
			Cursor->click = false;
			Return = true;
		}

		// アニメーション進行判定の補正
		for (int k = 0; k < 4; ++k) {
			ParamHoverAnime[k] += ((int)ParamHover[k] - ParamHoverAnime[k]) / 1.8f;
			if (abs(ParamHoverAnime[k] - (int)ParamHover[k]) < 0.001f) {
				ParamHoverAnime[k] = (float)ParamHover[k];
			}
			else {
				Return = true; // アニメーション移動中なら再描画を要求
			}
		}
		break;
	}
	case 2: {
		D2D1_RECT_F ParamInterval = D2D1::RectF(Rect.left + 80.0f, Rect.top + 10.0f, Rect.right - 10.0f, Rect.top + 10.0f + 25.0f);
		D2D1_RECT_F ParamMethod = D2D1::RectF(ParamInterval.left, ParamInterval.bottom + 10.0f, ParamInterval.right, ParamInterval.bottom + 10.0f + 25.0f);

		ParamHover[0] = Cursor->RectCheck(ParamInterval);
		ParamHover[1] = Cursor->RectCheck(ParamMethod);

		if (Cursor->click && ParamHover[0]) {
			HWND hExisting = GetDlgItem(Cursor->hwnd, ID_EDIT_INTERVAL);
			if (!hExisting) {
				wchar_t valStr[32];
				swprintf_s(valStr, L"%.2f", Param[0]);
				CreateInPlaceEdit(Cursor->hwnd, ParamInterval, valStr, ID_EDIT_INTERVAL, &Param[0]);
			}
			Cursor->click = false;
			Return = true;
		}
		if (Cursor->click && ParamHover[1]) {
			HMENU hMenu = CreatePopupMenu();
			if (hMenu) {
				AppendMenuW(hMenu, MF_STRING, 101, L"切り捨て");
				AppendMenuW(hMenu, MF_STRING, 102, L"四捨五入");
				AppendMenuW(hMenu, MF_STRING, 103, L"切り上げ");

				POINT pt = { (LONG)ParamMethod.left, (LONG)ParamMethod.bottom };
				if (Cursor->hwnd && IsWindow(Cursor->hwnd)) {
					ClientToScreen(Cursor->hwnd, &pt);
				}

				SetForegroundWindow(Cursor->hwnd);
				int cmd = TrackPopupMenu(
					hMenu,
					TPM_RIGHTBUTTON | TPM_RETURNCMD,
					pt.x, pt.y,
					0, Cursor->hwnd, NULL
				);
				DestroyMenu(hMenu);

				switch (cmd) {
				case 101: {
					Param[1] = 0.0;
					break;
				}
				case 102: {
					Param[1] = 1.0;
					break;
				}
				case 103: {
					Param[1] = 2.0;
					break;
				}
				}
			}
			Cursor->x = 0.0f;
			Cursor->y = 0.0f;
			Cursor->click = false;
			Return = true;
		}

		for (int k = 0; k < 2; ++k) {
			ParamHoverAnime[k] += ((int)ParamHover[k] - ParamHoverAnime[k]) / 1.8f;
			if (abs(ParamHoverAnime[k] - (int)ParamHover[k]) < 0.001f) {
				ParamHoverAnime[k] = (float)ParamHover[k];
			}
			else {
				Return = true;
			}
		}
		break;
	}
	case 3: {
		D2D1_RECT_F ParamInterval = D2D1::RectF(Rect.left + 80.0f, Rect.top + 10.0f, Rect.right - 10.0f, Rect.top + 10.0f + 25.0f);

		ParamHover[0] = Cursor->RectCheck(ParamInterval);

		if (Cursor->click && ParamHover[0]) {
			HWND hExisting = GetDlgItem(Cursor->hwnd, ID_EDIT_INTERVAL);
			if (!hExisting) {
				wchar_t valStr[32];
				swprintf_s(valStr, L"%.2f", Param[0]);
				CreateInPlaceEdit(Cursor->hwnd, ParamInterval, valStr, ID_EDIT_INTERVAL, &Param[0]);
			}
			Cursor->click = false;
			Return = true;
		}

		for (int k = 0; k < 2; ++k) {
			ParamHoverAnime[k] += ((int)ParamHover[k] - ParamHoverAnime[k]) / 1.8f;
			if (abs(ParamHoverAnime[k] - (int)ParamHover[k]) < 0.001f) {
				ParamHoverAnime[k] = (float)ParamHover[k];
			}
			else {
				Return = true;
			}
		}
		break;
	}
	}
	return Return;
}

void MODIFIER::PaintParam(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect) {
	COLORREF normalC = 0xff0000;
	COLORREF hoverC = 0xff0000;
	COLORREF borderC = 0xff0000;
	COLORREF textC = 0xff0000;

	if (config && config->get_color_code) {
		normalC = config->get_color_code(config, "ButtonBody");
		hoverC = config->get_color_code(config, "ButtonBodyHover");
		borderC = config->get_color_code(config, "Border");
		textC = config->get_color_code(config, "Text");
	}

	switch (Mode) {
	case 1: {
		D2D1_RECT_F ParamInterval = D2D1::RectF(Rect.left + 80.0f, Rect.top + 10.0f, Rect.right - 90.0f, Rect.top + 10.0f + 25.0f);
		D2D1_RECT_F ParamIntervalUnit = D2D1::RectF(ParamInterval.right + 5.0f, ParamInterval.top, Rect.right - 10.0f, ParamInterval.bottom);
		D2D1_RECT_F ParamTime = D2D1::RectF(ParamInterval.left, ParamInterval.bottom + 10.0f, ParamInterval.right, ParamInterval.bottom + 10.0f + 25.0f);
		D2D1_RECT_F ParamTimeUnit = D2D1::RectF(ParamTime.right + 5.0f, ParamTime.top, Rect.right - 10.0f, ParamTime.bottom);

		pBrush->SetColor(D2D1::ColorF(textC));
		Draw_Text_Dialog(pTarget, pBrush, D2D1::RectF(Rect.left + 10.0f, ParamInterval.top, ParamInterval.left, ParamInterval.bottom), L"間隔", TextAlign::Left);
		Draw_Text_Dialog(pTarget, pBrush, D2D1::RectF(Rect.left + 10.0f, ParamTime.top, ParamTime.left, ParamTime.bottom), L"開始位置", TextAlign::Left);

		D2D1::ColorF Button(normalC);
		D2D1::ColorF ButtonHover(hoverC);
		float r = Button.r * (1.0f - ParamHoverAnime[0]) + ButtonHover.r * ParamHoverAnime[0];
		float g = Button.g * (1.0f - ParamHoverAnime[0]) + ButtonHover.g * ParamHoverAnime[0];
		float b = Button.b * (1.0f - ParamHoverAnime[0]) + ButtonHover.b * ParamHoverAnime[0];
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(ParamInterval, 2.0f, 2.0f), pBrush);

		r = Button.r * (1.0f - ParamHoverAnime[1]) + ButtonHover.r * ParamHoverAnime[1];
		g = Button.g * (1.0f - ParamHoverAnime[1]) + ButtonHover.g * ParamHoverAnime[1];
		b = Button.b * (1.0f - ParamHoverAnime[1]) + ButtonHover.b * ParamHoverAnime[1];
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(ParamIntervalUnit, 2.0f, 2.0f), pBrush);

		r = Button.r * (1.0f - ParamHoverAnime[2]) + ButtonHover.r * ParamHoverAnime[2];
		g = Button.g * (1.0f - ParamHoverAnime[2]) + ButtonHover.g * ParamHoverAnime[2];
		b = Button.b * (1.0f - ParamHoverAnime[2]) + ButtonHover.b * ParamHoverAnime[2];
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(ParamTime, 2.0f, 2.0f), pBrush);

		r = Button.r * (1.0f - ParamHoverAnime[3]) + ButtonHover.r * ParamHoverAnime[3];
		g = Button.g * (1.0f - ParamHoverAnime[3]) + ButtonHover.g * ParamHoverAnime[3];
		b = Button.b * (1.0f - ParamHoverAnime[3]) + ButtonHover.b * ParamHoverAnime[3];
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(ParamTimeUnit, 2.0f, 2.0f), pBrush);
		pBrush->SetColor(D2D1::ColorF(borderC));
		pTarget->DrawRoundedRectangle(D2D1::RoundedRect(ParamTimeUnit, 2.0f, 2.0f), pBrush, 1.5f);
		pTarget->DrawRoundedRectangle(D2D1::RoundedRect(ParamTime, 2.0f, 2.0f), pBrush, 1.5f);
		pTarget->DrawRoundedRectangle(D2D1::RoundedRect(ParamIntervalUnit, 2.0f, 2.0f), pBrush, 1.5f);
		pTarget->DrawRoundedRectangle(D2D1::RoundedRect(ParamInterval, 2.0f, 2.0f), pBrush, 1.5f);

		wchar_t valStr[32];
		std::wstring IntervalUnit[6] = { L"秒", L"ミリ秒", L"フレーム", L"FPS", L"BPM", L"BPM参照" };
		std::wstring TimeUnit[4] = { L"秒", L"ミリ秒", L"フレーム", L"BPM参照" };
		swprintf_s(valStr, L"%.2f", Param[0]);
		pBrush->SetColor(D2D1::ColorF(textC));
		Draw_Text_Dialog(pTarget, pBrush, ParamInterval, valStr, TextAlign::Center);
		Draw_Text_Dialog(pTarget, pBrush, ParamIntervalUnit, IntervalUnit[(int)Param[1]], TextAlign::Center);
		swprintf_s(valStr, L"%.2f", Param[2]);
		Draw_Text_Dialog(pTarget, pBrush, ParamTime, valStr, TextAlign::Center);
		Draw_Text_Dialog(pTarget, pBrush, ParamTimeUnit, TimeUnit[(int)Param[3]], TextAlign::Center);

		break;
	}
	case 2: {
		D2D1_RECT_F ParamInterval = D2D1::RectF(Rect.left + 80.0f, Rect.top + 10.0f, Rect.right - 10.0f, Rect.top + 10.0f + 25.0f);
		D2D1_RECT_F ParamMethod = D2D1::RectF(ParamInterval.left, ParamInterval.bottom + 10.0f, ParamInterval.right, ParamInterval.bottom + 10.0f + 25.0f);

		pBrush->SetColor(D2D1::ColorF(textC));
		Draw_Text_Dialog(pTarget, pBrush, D2D1::RectF(Rect.left + 10.0f, ParamInterval.top, ParamInterval.left, ParamInterval.bottom), L"分割数", TextAlign::Left);
		pBrush->SetColor(D2D1::ColorF(textC));
		Draw_Text_Dialog(pTarget, pBrush, D2D1::RectF(Rect.left + 10.0f, ParamMethod.top, ParamMethod.left, ParamMethod.bottom), L"丸め", TextAlign::Left);

		D2D1::ColorF Button(normalC);
		D2D1::ColorF ButtonHover(hoverC);
		float r = Button.r * (1.0f - ParamHoverAnime[0]) + ButtonHover.r * ParamHoverAnime[0];
		float g = Button.g * (1.0f - ParamHoverAnime[0]) + ButtonHover.g * ParamHoverAnime[0];
		float b = Button.b * (1.0f - ParamHoverAnime[0]) + ButtonHover.b * ParamHoverAnime[0];
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(ParamInterval, 2.0f, 2.0f), pBrush);
		r = Button.r * (1.0f - ParamHoverAnime[1]) + ButtonHover.r * ParamHoverAnime[1];
		g = Button.g * (1.0f - ParamHoverAnime[1]) + ButtonHover.g * ParamHoverAnime[1];
		b = Button.b * (1.0f - ParamHoverAnime[1]) + ButtonHover.b * ParamHoverAnime[1];
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(ParamMethod, 2.0f, 2.0f), pBrush);

		pBrush->SetColor(D2D1::ColorF(borderC));
		pTarget->DrawRoundedRectangle(D2D1::RoundedRect(ParamInterval, 2.0f, 2.0f), pBrush, 1.5f);
		pTarget->DrawRoundedRectangle(D2D1::RoundedRect(ParamMethod, 2.0f, 2.0f), pBrush, 1.5f);

		wchar_t valStr[32];
		std::wstring Method[3] = { L"切り捨て", L"四捨五入", L"切り上げ" };
		swprintf_s(valStr, L"%.2f", Param[0]);
		pBrush->SetColor(D2D1::ColorF(textC));
		Draw_Text_Dialog(pTarget, pBrush, ParamInterval, valStr, TextAlign::Center);
		Draw_Text_Dialog(pTarget, pBrush, ParamMethod, Method[(int)Param[1]], TextAlign::Center);

		break;
	}
	case 3: {
		D2D1_RECT_F ParamInterval = D2D1::RectF(Rect.left + 80.0f, Rect.top + 10.0f, Rect.right - 10.0f, Rect.top + 10.0f + 25.0f);

		pBrush->SetColor(D2D1::ColorF(textC));
		Draw_Text_Dialog(pTarget, pBrush, D2D1::RectF(Rect.left + 10.0f, ParamInterval.top, ParamInterval.left, ParamInterval.bottom), L"増分(%)", TextAlign::Left);

		D2D1::ColorF Button(normalC);
		D2D1::ColorF ButtonHover(hoverC);
		float r = Button.r * (1.0f - ParamHoverAnime[0]) + ButtonHover.r * ParamHoverAnime[0];
		float g = Button.g * (1.0f - ParamHoverAnime[0]) + ButtonHover.g * ParamHoverAnime[0];
		float b = Button.b * (1.0f - ParamHoverAnime[0]) + ButtonHover.b * ParamHoverAnime[0];
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(ParamInterval, 2.0f, 2.0f), pBrush);

		pBrush->SetColor(D2D1::ColorF(borderC));
		pTarget->DrawRoundedRectangle(D2D1::RoundedRect(ParamInterval, 2.0f, 2.0f), pBrush, 1.5f);

		wchar_t valStr[32];
		swprintf_s(valStr, L"%.2f", Param[0]);
		pBrush->SetColor(D2D1::ColorF(textC));
		Draw_Text_Dialog(pTarget, pBrush, ParamInterval, valStr, TextAlign::Center);

		break;
	}
	}
}

struct MODDIALOG {
	std::vector<MODMENU> Modifiers;
	bool AddHover = false;
	float AddHoverAnime = 0.0f;

	float RangeY[3] = { 80.0f, 80.0f, 45.0f}; //パラメータ1つ35.0f、余白上下合わせて10.0f
	float Y = 0.0f;
	float YF = 0.0f;

	bool Updata(D2D1_RECT_F Rect, CUR_MOD* Cursor, LOCUSES& targetLocuses);
	void Paint(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect);
};

bool MODDIALOG::Updata(D2D1_RECT_F Rect, CUR_MOD* Cursor, LOCUSES& targetLocuses) {
	bool Return = false;
	float RangeAll = 40.0f;
	for (auto& Mod : Modifiers) {
		RangeAll += 27.0f + RangeY[Mod.Modifier.Mode - 1] * Mod.OpenAnime;
	}
	RangeAll = (std::max)(RangeAll - (Rect.bottom - Rect.top),0.0f);

	YF += (std::min)((std::max)(Cursor->wheel, -1.0f), 1.0f) * -50.0f;
	YF = (std::min)((std::max)(YF, 0.0f), RangeAll);

	Y += (YF - Y) / 3.0f;
	if (abs(Y - YF) < 0.001f) {
		Y = YF;
	}
	else {
		Return = true;
	}

	float y = -Y + 2.0f;
	int i = 0;
	for (auto& Mod : Modifiers) {
		D2D1_RECT_F ModName = D2D1::RectF(Rect.left + 0.0f, y, Rect.right - 10.0f - 50.0f, y + 25.0f);
		D2D1_RECT_F Disable = D2D1::RectF(ModName.right + 26.0f, ModName.top, ModName.right + 51.0f, ModName.bottom);
		D2D1_RECT_F Delete = D2D1::RectF(ModName.right, ModName.top, ModName.right + 25.0f, ModName.bottom);
		Mod.Hover = Cursor->RectCheck(ModName);
		Mod.DisableHover = Cursor->RectCheck(Disable);
		Mod.DeleteHover = Cursor->RectCheck(Delete);

		if (Cursor->click && Mod.Hover) {
			Mod.Open = !Mod.Open;
			Cursor->click = false;
			Return = true;
		}
		if (Cursor->click && Mod.DisableHover) {
			Mod.Disable = !Mod.Disable;
			Cursor->click = false;
			Return = true;
		}
		if (Cursor->click && Mod.DeleteHover) {
			Cursor->click = false;
			Modifiers.erase(Modifiers.begin() + i);

			if (i < static_cast<int>(targetLocuses.Modifier.size())) {
				targetLocuses.Modifier.erase(targetLocuses.Modifier.begin() + i);
			}

			Return = true;
			break;
		}

		// アニメーション進行状態のチェック
		auto updateAnime = [&](float& current, bool target) {
			current += ((int)target - current) / 1.8f;
			if (abs(current - (int)target) < 0.001f) {
				current = (float)target;
			}
			else {
				Return = true; // 補間中なら再描画を要求
			}
			};

		updateAnime(Mod.HoverAnime, Mod.Hover);
		updateAnime(Mod.DisableHoverAnime, Mod.DisableHover);
		updateAnime(Mod.DeleteHoverAnime, Mod.DeleteHover);

		// OpenAnimeは補間率が違うため別個処理
		Mod.OpenAnime += ((int)Mod.Open - Mod.OpenAnime) / 2.0f;
		if (abs(Mod.OpenAnime - (int)Mod.Open) < 0.001f) {
			Mod.OpenAnime = (float)Mod.Open;
		}
		else {
			Return = true;
		}

		D2D1_RECT_F ModMenu = D2D1::RectF(ModName.left, ModName.bottom, Rect.right - 10.0f, ModName.bottom + RangeY[Mod.Modifier.Mode - 1] * Mod.OpenAnime);
		D2D1_RECT_F ModMenuParam = D2D1::RectF(ModMenu.left, ModMenu.bottom - RangeY[Mod.Modifier.Mode - 1], ModMenu.right, ModMenu.bottom);

		if (Mod.Modifier.UpdataParam(ModMenuParam, Cursor)) {
			if (i < static_cast<int>(targetLocuses.Modifier.size())) {
				targetLocuses.Modifier[i] = Mod.Modifier;
			}
			Return = true;
		}

		y += 27.0f + RangeY[Mod.Modifier.Mode - 1] * Mod.OpenAnime;
		i += 1;
	}

	// 追加ボタンの処理
	D2D1_RECT_F AddButton = D2D1::RectF(Rect.left + 30.0f, y + 5.0f, Rect.right - 40.0f, y + 30.0f);
	AddHover = Cursor->RectCheck(AddButton);

	AddHoverAnime += ((int)AddHover - AddHoverAnime) / 1.8f;
	if (abs(AddHoverAnime - (int)AddHover) < 0.001f) {
		AddHoverAnime = (float)AddHover;
	}
	else {
		Return = true;
	}

	static bool isShowingMenu = false;
	if (AddHover && Cursor->click && !isShowingMenu) {
		isShowingMenu = true;

		HMENU hMenu = CreatePopupMenu();
		if (hMenu) {
			AppendMenuW(hMenu, MF_STRING, 101, L"コマ落ち");
			AppendMenuW(hMenu, MF_STRING, 102, L"離散化");
			AppendMenuW(hMenu, MF_STRING, 103, L"速度化");

			POINT pt = { (LONG)AddButton.left, (LONG)AddButton.bottom };
			if (Cursor->hwnd && IsWindow(Cursor->hwnd)) {
				ClientToScreen(Cursor->hwnd, &pt);
			}

			SetForegroundWindow(Cursor->hwnd);
			int cmd = TrackPopupMenu(
				hMenu,
				TPM_RIGHTBUTTON | TPM_RETURNCMD,
				pt.x, pt.y,
				0, Cursor->hwnd, NULL
			);
			DestroyMenu(hMenu);

			switch (cmd) {
			case 101: {
				MODIFIER NewModifier(1);
				MODMENU NewModifierMenu;
				NewModifierMenu.Open = true;
				NewModifierMenu.OpenAnime = 1.0f;
				NewModifierMenu.Modifier = NewModifier;
				Modifiers.push_back(NewModifierMenu);

				targetLocuses.Modifier.push_back(NewModifier);
				Return = true;
				break;
			}
			case 102: {
				MODIFIER NewModifier(2);
				MODMENU NewModifierMenu;
				NewModifierMenu.Open = true;
				NewModifierMenu.OpenAnime = 1.0f;
				NewModifierMenu.Modifier = NewModifier;
				Modifiers.push_back(NewModifierMenu);

				targetLocuses.Modifier.push_back(NewModifier);
				Return = true;
				break;
			}
			case 103: {
				MODIFIER NewModifier(3);
				MODMENU NewModifierMenu;
				NewModifierMenu.Open = false;
				NewModifierMenu.OpenAnime = 0.0f;
				NewModifierMenu.Modifier = NewModifier;
				Modifiers.push_back(NewModifierMenu);

				targetLocuses.Modifier.push_back(NewModifier);
				Return = true;
				break;
			}
			}
		}
		Cursor->click = false;
		isShowingMenu = false;
	}

	return Return;
}

void MODDIALOG::Paint(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect) {
	if (!pTarget || !pBrush) return;

	COLORREF normalC = 0xff0000;
	COLORREF hoverC = 0xff0000;
	COLORREF borderC = 0xff0000;
	COLORREF textC = 0xff0000;
	COLORREF GroupingC = 0xff0000;
	COLORREF GroupingHoverC = 0xff0000;

	if (config && config->get_color_code) {
		normalC = config->get_color_code(config, "ButtonBody");
		hoverC = config->get_color_code(config, "ButtonBodyHover");
		borderC = config->get_color_code(config, "Border");
		textC = config->get_color_code(config, "Text");
		GroupingC = config->get_color_code(config, "Grouping");
		GroupingHoverC = config->get_color_code(config, "GroupingHover");
	}

	float RangeAll = 40.0f;
	for (auto& Mod : Modifiers) {
		RangeAll += 27.0f + RangeY[Mod.Modifier.Mode - 1] * Mod.OpenAnime;
	}
	RangeAll -= Rect.bottom - Rect.top;

	float y = -Y + 2.0f;
	pBrush->SetColor(D2D1::ColorF(GroupingC));
	for (auto& Mod : Modifiers) {
		D2D1_RECT_F ModName = D2D1::RectF(Rect.left + 0.0f, y, Rect.right - 10.0f, y + 25.0f);
		D2D1_RECT_F Disable = D2D1::RectF(ModName.right - 25.0f + 2.0f, ModName.top + 2.0f, ModName.right - 2.0f, ModName.bottom - 2.0f);
		D2D1_RECT_F Delete = D2D1::RectF(ModName.right - 51.0f, ModName.top, ModName.right - 26.0f, ModName.bottom);
		D2D1::ColorF Grouping(GroupingC);
		D2D1::ColorF GroupingHover(GroupingHoverC);
		D2D1::ColorF Button(normalC);
		D2D1::ColorF ButtonHover(hoverC);
		float r = Grouping.r * (1.0f - Mod.HoverAnime) + GroupingHover.r * Mod.HoverAnime;
		float g = Grouping.g * (1.0f - Mod.HoverAnime) + GroupingHover.g * Mod.HoverAnime;
		float b = Grouping.b * (1.0f - Mod.HoverAnime) + GroupingHover.b * Mod.HoverAnime;
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(ModName, 4.0f, 4.0f), pBrush);
		std::wstring Name[3] = { L"コマ落ち", L"離散化", L"速度化"};
		pBrush->SetColor(D2D1::ColorF(textC));
		Draw_Text_Dialog(pTarget, pBrush, D2D1::RectF(ModName.left + 30.0f, ModName.top, ModName.right, ModName.bottom), Name[Mod.Modifier.Mode - 1], TextAlign::Left);
		D2D1_RECT_F ArrowRect = D2D1::RectF(ModName.left + 5.0f, ModName.top, ModName.left + 25.0f, ModName.bottom);
		Draw_Svg_Dialog(pTarget, 201, ArrowRect, 0.75f, 90.0f * Mod.OpenAnime - 90.0f, D2D1::ColorF(normalC));
		r = (Mod.Disable ? Grouping.r : Button.r) * (1.0f - Mod.DisableHoverAnime) + ButtonHover.r * Mod.DisableHoverAnime;
		g = (Mod.Disable ? Grouping.r : Button.g) * (1.0f - Mod.DisableHoverAnime) + ButtonHover.g * Mod.DisableHoverAnime;
		b = (Mod.Disable ? Grouping.r : Button.b) * (1.0f - Mod.DisableHoverAnime) + ButtonHover.b * Mod.DisableHoverAnime;
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(Disable, 2.0f, 2.0f), pBrush);
		pBrush->SetColor(D2D1::ColorF(borderC));
		pTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(Disable.left + 1.5f / 2.0f, Disable.top + 1.5f / 2.0f, Disable.right - 1.5f / 2.0f, Disable.bottom - 1.5f / 2.0f), 2.0f, 2.0f), pBrush, 1.5f);
		if (!Mod.Disable)
			Draw_Svg_Dialog(pTarget, 341, Disable, 1.0f, 0.0f, D2D1::ColorF(textC));
		r = Grouping.r * (1.0f - Mod.DeleteHoverAnime) + ButtonHover.r * Mod.DeleteHoverAnime;
		g = Grouping.g * (1.0f - Mod.DeleteHoverAnime) + ButtonHover.g * Mod.DeleteHoverAnime;
		b = Grouping.b * (1.0f - Mod.DeleteHoverAnime) + ButtonHover.b * Mod.DeleteHoverAnime;
		pBrush->SetColor(D2D1::ColorF(r, g, b));
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(Delete, 4.0f, 4.0f), pBrush);
		Draw_Svg_Dialog(pTarget, 342, Delete, 0.614f, 0.0f, D2D1::ColorF(textC));
		y += 27.0f;

		pBrush->SetColor(D2D1::ColorF(GroupingC));
		D2D1_RECT_F ModMenu = D2D1::RectF(ModName.left, ModName.bottom, ModName.right, ModName.bottom + RangeY[Mod.Modifier.Mode - 1] * Mod.OpenAnime);
		D2D1_RECT_F ModMenuParam = D2D1::RectF(ModMenu.left, ModMenu.bottom - RangeY[Mod.Modifier.Mode - 1], ModMenu.right, ModMenu.bottom);
		pTarget->FillRoundedRectangle(D2D1::RoundedRect(ModMenu, 4.0f, 4.0f), pBrush);
		if (Mod.OpenAnime != 0.0f) {
			pTarget->PushAxisAlignedClip(
				ModMenu,
				D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
			);

			pBrush->SetColor(D2D1::ColorF(hoverC));
			pTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(ModMenu.left + 10.0f, ModMenu.top, ModMenu.right - 10.0f, ModMenu.top + 1.0f), 0.5f, 0.5f), pBrush);
			Mod.Modifier.PaintParam(pTarget, pBrush, ModMenuParam);

			pTarget->PopAxisAlignedClip();
		}
		y += RangeY[Mod.Modifier.Mode - 1] * Mod.OpenAnime;
	}

	D2D1_RECT_F AddButton = D2D1::RectF(Rect.left + 30.0f, y + 5.0f, Rect.right - 40.0f, y + 30.0f);
	D2D1::ColorF colorNormal(normalC);
	D2D1::ColorF colorHover(hoverC);
	float r = colorNormal.r * (1.0f - AddHoverAnime) + colorHover.r * AddHoverAnime;
	float g = colorNormal.g * (1.0f - AddHoverAnime) + colorHover.g * AddHoverAnime;
	float b = colorNormal.b * (1.0f - AddHoverAnime) + colorHover.b * AddHoverAnime;
	pBrush->SetColor(D2D1::ColorF(r, g, b));
	pTarget->FillRoundedRectangle(D2D1::RoundedRect(AddButton, 4.0f, 4.0f), pBrush);
	pBrush->SetColor(D2D1::ColorF(borderC));
	pTarget->DrawRoundedRectangle(D2D1::RoundedRect(AddButton, 4.0f, 4.0f), pBrush, 1.5f);

	pBrush->SetColor(D2D1::ColorF(textC));
	Draw_Text_Dialog(pTarget, pBrush, AddButton, L"＋", TextAlign::Center, 20);

	float RectYRange = (Rect.bottom - Rect.top);
	float ScrollRatio;
	if (RangeAll != 0.0) {
		ScrollRatio = Y / RangeAll;
	}
	else {
		ScrollRatio = 0;
	}
	float ScrollRange = RectYRange / (RangeAll + RectYRange) * RectYRange - 10.0f;
	float ScrollY = Rect.top + ScrollRange / 2.0f + 5.0f + ScrollRatio * (RectYRange - ScrollRange - 10.0f);
	D2D1_RECT_F ScrollRect = D2D1::RectF(Rect.right - 8.0f, ScrollY - ScrollRange / 2.0f, Rect.right - 2.0f, ScrollY + ScrollRange / 2.0f);
	pBrush->SetColor(D2D1::ColorF(normalC));
	D2D1_ROUNDED_RECT ScrollRoundedRect = D2D1::RoundedRect(ScrollRect, 1.0f, 1.0f);
	pTarget->FillRoundedRectangle(ScrollRoundedRect, pBrush);
}

struct DialogState {
	ComPtr<ID2D1HwndRenderTarget> pRenderTarget;
	ComPtr<ID2D1SolidColorBrush> pBrush;
	LOCUSES* pTargetLocuses = nullptr;
};

bool ModifierWindow(HWND hParent, const wchar_t* title, CUR Cursor, LOCUSES& targetLocuses) {
	static const wchar_t* className = L"ModifierDialog";
	static bool registered = false;

	static MODDIALOG Modifier;
	static CUR_MOD cursor;
	static D2D1_RECT_F WinRect;

	Modifier.Modifiers.clear();
	for (const auto& mod : targetLocuses.Modifier) {
		MODMENU menu;
		menu.Open = true;
		menu.OpenAnime = 1.0f;
		menu.Modifier = mod;
		Modifier.Modifiers.push_back(menu);
	}

	cursor = CUR_MOD{};

	DialogState state;
	state.pTargetLocuses = &targetLocuses;

	if (!registered) {
		WNDCLASSW wc = { 0 };
		wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
			DialogState* pState = reinterpret_cast<DialogState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

			switch (msg) {
			case WM_NCCREATE: {
				CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
				SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
				return DefWindowProcW(hwnd, msg, wp, lp);
			}
			case WM_CREATE: {
				SetTimer(hwnd, 3, 1000 / 60, NULL);
				SetWindowTextW(hwnd, L"モディファイア");
				cursor.hwnd = hwnd;
				return 0;
			}
			case WM_ERASEBKGND:
				return 1;

			case WM_SIZE: {
				UINT width = LOWORD(lp);
				UINT height = HIWORD(lp);
				WinRect = D2D1::RectF(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

				if (pState && pState->pRenderTarget) {
					pState->pRenderTarget->Resize(D2D1::SizeU(width, height));
				}
				return 0;
			}
			case WM_TIMER: {
				if (pState && pState->pTargetLocuses) {
					if (Modifier.Updata(WinRect, &cursor, *pState->pTargetLocuses)) {
						InvalidateRect(hwnd, NULL, FALSE);
					}
				}
				cursor.wheel = 0.0f;
				cursor.click = false;
				return 0;
			}
			case WM_MOUSEMOVE: {
				cursor.x = static_cast<float>(static_cast<short>(LOWORD(lp)));
				cursor.y = static_cast<float>(static_cast<short>(HIWORD(lp)));
				return 0;
			}
			case WM_LBUTTONDOWN: {
				SetFocus(hwnd);
				cursor.click = true;
				return 0;
			}
			case WM_LBUTTONUP: {
				cursor.click = false;
				return 0;
			}
			case WM_MOUSEWHEEL: {
				short delta = GET_WHEEL_DELTA_WPARAM(wp);
				cursor.wheel = (std::min)((std::max)(cursor.wheel + (float)(delta / 120), -1.5f), 1.5f);
				break;
			}
			case WM_PAINT: {
				PAINTSTRUCT ps;
				BeginPaint(hwnd, &ps);

				if (pState) {
					if (!pState->pRenderTarget && g_pD2DFactory) {
						RECT rc;
						GetClientRect(hwnd, &rc);
						HRESULT hr = g_pD2DFactory->CreateHwndRenderTarget(
							D2D1::RenderTargetProperties(),
							D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)),
							&pState->pRenderTarget
						);

						if (SUCCEEDED(hr) && pState->pRenderTarget) {
							pState->pRenderTarget->CreateSolidColorBrush(
								D2D1::ColorF(D2D1::ColorF::White),
								&pState->pBrush
							);
						}
					}

					if (pState->pRenderTarget && pState->pBrush) {
						pState->pRenderTarget->BeginDraw();

						COLORREF bgColor = 0x333333;
						if (config && config->get_color_code) {
							bgColor = config->get_color_code(config, "Background");
						}
						pState->pRenderTarget->Clear(D2D1::ColorF(bgColor));

						Modifier.Paint(pState->pRenderTarget.Get(), pState->pBrush.Get(), WinRect);

						HRESULT hr = pState->pRenderTarget->EndDraw();
						if (hr == D2DERR_RECREATE_TARGET) {
							pState->pBrush.Reset();
							pState->pRenderTarget.Reset();
						}
					}
				}

				EndPaint(hwnd, &ps);
				return 0;
			}
			case WM_CLOSE:
				DestroyWindow(hwnd);
				return 0;

			case WM_DESTROY: {
				KillTimer(hwnd, 3);
				if (pState) {
					pState->pBrush.Reset();
					pState->pRenderTarget.Reset();
				}
				return 0;
			}
			}
			return DefWindowProcW(hwnd, msg, wp, lp);
			};

		wc.hInstance = GetModuleHandle(NULL);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
		wc.lpszClassName = className;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		RegisterClassW(&wc);
		registered = true;
	}

	RECT rcParent;
	GetWindowRect(hParent, &rcParent);
	int x = (int)(std::max)(static_cast<float>(rcParent.left) + Cursor.x - 250.0f, 0.0f);
	int y = (int)(std::max)(static_cast<float>(rcParent.top) + Cursor.y - 175.0f, 0.0f);

	HWND hDlg = CreateWindowExW(
		WS_EX_DLGMODALFRAME,
		className, title,
		WS_POPUP | WS_CAPTION | WS_SYSMENU,
		x, y, 300, 400,
		hParent, NULL, GetModuleHandle(NULL), &state
	);

	if (!hDlg) return false;

	EnableWindow(hParent, FALSE);
	ShowWindow(hDlg, SW_SHOW);
	UpdateWindow(hDlg);

	MSG msg;
	BOOL bRet;
	while (IsWindow(hDlg) && (bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (bRet == -1) break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	EnableWindow(hParent, TRUE);
	SetForegroundWindow(hParent);

	return true;
}