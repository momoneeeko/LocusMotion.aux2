#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>
#include "Direct2D.h"
#include "Dialog.h"
#include <algorithm>
#include <string>
#include <functional>
#include <format>

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

using Microsoft::WRL::ComPtr;

#define ID_EDIT_INTERVAL 1001

struct EditParam {
	float* pVal;
	bool scale001;
	bool limit;
	float max;
	float min;
};

static void ValueInput_SaveAndCloseEdit(HWND hEdit) {
	if (!hEdit || !IsWindow(hEdit)) return;

	EditParam* param = static_cast<EditParam*>(GetPropW(hEdit, L"VALUE_PTR"));
	if (param) {
		RemovePropW(hEdit, L"VALUE_PTR");

		wchar_t buf[256] = {};
		GetWindowTextW(hEdit, buf, 256);

		if (buf[0] != L'\0') {
			float val = static_cast<float>(std::wcstod(buf, nullptr));
			if (param->scale001) {
				val *= 0.01f;
			}
			if (param->limit) {
				val = (std::min)((std::max)(val, param->min), param->max);
			}
			*(param->pVal) = val;
		}
		delete param;
		DestroyWindow(hEdit);
	}
}

static LRESULT CALLBACK ValueInput_EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (uMsg) {
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;

	case WM_KEYDOWN:
		if (wParam == VK_RETURN) {
			ValueInput_SaveAndCloseEdit(hWnd);
			return 0;
		}
		else if (wParam == VK_ESCAPE) {
			EditParam* param = static_cast<EditParam*>(GetPropW(hWnd, L"VALUE_PTR"));
			if (param) {
				RemovePropW(hWnd, L"VALUE_PTR");
				delete param;
			}
			DestroyWindow(hWnd);
			return 0;
		}
		break;

	case WM_KILLFOCUS:
		ValueInput_SaveAndCloseEdit(hWnd);
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, ValueInput_EditSubclassProc, uIdSubclass);
		break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static HWND ValueInput_CreateInPlaceEdit(HWND hParent, D2D1_RECT_F rect, const std::wstring& currentText, UINT controlID, float* pValueTarget = nullptr, bool scale001 = false, bool limit = true, float min = 0.0f, float max = 1.0f) {
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
			EditParam* param = new EditParam{ pValueTarget, scale001, limit, max, min };
			SetPropW(hEdit, L"VALUE_PTR", reinterpret_cast<HANDLE>(param));
		}

		SetWindowSubclass(hEdit, ValueInput_EditSubclassProc, 0, 0);
		SetFocus(hEdit);
	}
	return hEdit;
}

struct VALUEINPUT {
	bool Update(D2D1_RECT_F Rect, CUR_MOD* Cursor, LOCUS* targetLocus, int Type);
	void Paint(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect, LOCUS* targetLocus, int Type);
};

bool VALUEINPUT::Update(D2D1_RECT_F Rect, CUR_MOD* Cursor, LOCUS* targetLocus, int Type) {
	bool Return = false;

	D2D1_RECT_F Value1 = D2D1::RectF(Rect.left + 10.0f, Rect.top + 5.0f, Rect.right / 2.0f - 10.0f, Rect.bottom - 5.0f);
	D2D1_RECT_F Value2 = D2D1::RectF(Rect.right / 2.0f + 10.0f, Rect.top + 5.0f, Rect.right - 10.0f, Rect.bottom - 5.0f);
	D2D1_RECT_F Value3 = D2D1::RectF(Rect.left + 30.0f, Rect.top + 5.0f, Rect.right - 35.0f, Rect.bottom - 5.0f);

	if (Cursor->click && Cursor->RectCheck(Value1) && Type != 4) {
		HWND hExisting = GetDlgItem(Cursor->hwnd, ID_EDIT_INTERVAL);
		if (!hExisting && targetLocus) {
			wchar_t valStr[32];
			float* pTargetVal = nullptr;
			bool limit = true;
			float min = 0.0f;
			float max = 1.0f;

			if (Type == 0) {
				swprintf_s(valStr, L"%.4f", targetLocus->S.x);
				pTargetVal = &targetLocus->S.x;
			}
			else if (Type == 1) {
				swprintf_s(valStr, L"%.4f", targetLocus->H1.x);
				pTargetVal = &targetLocus->H1.x;
			}
			else if (Type == 2) {
				swprintf_s(valStr, L"%.4f", targetLocus->H2.x);
				pTargetVal = &targetLocus->H2.x;
			}
			else if (Type == 3) {
				swprintf_s(valStr, L"%.4f", targetLocus->F.x);
				pTargetVal = &targetLocus->F.x;
			}
			else {
				swprintf_s(valStr, L"%.4f", targetLocus->H2.x);
				pTargetVal = &targetLocus->H2.x;
			}

			if (pTargetVal) {
				ValueInput_CreateInPlaceEdit(Cursor->hwnd, Value1, valStr, ID_EDIT_INTERVAL, pTargetVal, false, limit, min, max);
			}
		}
		Cursor->click = false;
		Return = true;
	}

	if (Cursor->click && Cursor->RectCheck(Value2) && Type != 4) {
		HWND hExisting = GetDlgItem(Cursor->hwnd, ID_EDIT_INTERVAL);
		if (!hExisting && targetLocus && Type != 4) {
			wchar_t valStr[32];
			float* pTargetVal = nullptr;
			bool limit = true;
			float min = 0.0f;
			float max = 1.0f;

			if (Type == 0) {
				swprintf_s(valStr, L"%.4f", targetLocus->S.y);
				pTargetVal = &targetLocus->S.y;
			}
			else if (Type == 1) {
				swprintf_s(valStr, L"%.4f", targetLocus->H1.y);
				pTargetVal = &targetLocus->H1.y;
				limit = false;
			}
			else if (Type == 2) {
				swprintf_s(valStr, L"%.4f", targetLocus->H2.y);
				pTargetVal = &targetLocus->H2.y;
				limit = false;
			}
			else if(Type == 3) {
				swprintf_s(valStr, L"%.4f", targetLocus->F.y);
				pTargetVal = &targetLocus->F.y;
			}
			else {
				swprintf_s(valStr, L"%.4f", targetLocus->H2.y);
				pTargetVal = &targetLocus->H2.y;
			}

			if (pTargetVal) {
				ValueInput_CreateInPlaceEdit(Cursor->hwnd, Value2, valStr, ID_EDIT_INTERVAL, pTargetVal, false, limit, min, max);
			}
		}
		Cursor->click = false;
		Return = true;
	}

	if (Cursor->click && Cursor->RectCheck(Value3) && Type == 4) {
		HWND hExisting = GetDlgItem(Cursor->hwnd, ID_EDIT_INTERVAL);
		if (!hExisting && targetLocus) {
			wchar_t valStr[32];
			float* pTargetVal = nullptr;

			swprintf_s(valStr, L"%.4f", targetLocus->H1.y * 100.0f);
			pTargetVal = &targetLocus->H1.y;

			if (pTargetVal) {
				ValueInput_CreateInPlaceEdit(Cursor->hwnd, Value3, valStr, ID_EDIT_INTERVAL, pTargetVal, true, true, -0.8999f, 0.8999f);
			}
		}
		Cursor->click = false;
		Return = true;
	}

	return Return;
}

void VALUEINPUT::Paint(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect, LOCUS* targetLocus, int Type) {
	if (!pTarget || !pBrush) return;

	D2D1_RECT_F SRect = D2D1::RectF(Rect.left + 5.0f, Rect.top, Rect.right - 5.0f, Rect.bottom);
	if(Type == 4)
		SRect = D2D1::RectF(Rect.left + 25.0f, Rect.top, Rect.right - 25.0f, Rect.bottom);
	D2D1_RECT_F Value1 = D2D1::RectF(Rect.left + 10.0f, Rect.top, Rect.right / 2.0f - 10.0f, Rect.bottom);
	D2D1_RECT_F Value2 = D2D1::RectF(Rect.right / 2.0f + 5.0f, Rect.top, Rect.right - 5.0f, Rect.bottom);
	D2D1_RECT_F Value3 = D2D1::RectF(Rect.left + 30.0f, Rect.top, Rect.right - 35.0f, Rect.bottom);

	Draw_Text_Dialog(pTarget, pBrush, SRect, L"(", TextAlign::Left);
	if(Type != 4)
		Draw_Text_Dialog(pTarget, pBrush, SRect, L",", TextAlign::Center);
	Draw_Text_Dialog(pTarget, pBrush, SRect, (Type == 4)?L")‹":L")", TextAlign::Right);

	pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Text")));
	if (Type == 0) {
		Draw_Text_Dialog(pTarget, pBrush, Value1, std::format(L"{:.3f}", targetLocus->S.x), TextAlign::Center);
		Draw_Text_Dialog(pTarget, pBrush, Value2, std::format(L"{:.3f}", targetLocus->S.y), TextAlign::Center);
	}
	else if (Type == 1) {
		Draw_Text_Dialog(pTarget, pBrush, Value1, std::format(L"{:.3f}", targetLocus->H1.x), TextAlign::Center);
		Draw_Text_Dialog(pTarget, pBrush, Value2, std::format(L"{:.3f}", targetLocus->H1.y), TextAlign::Center);
	}
	else if (Type == 2 || Type == 5) {
		Draw_Text_Dialog(pTarget, pBrush, Value1, std::format(L"{:.3f}", targetLocus->H2.x), TextAlign::Center);
		Draw_Text_Dialog(pTarget, pBrush, Value2, std::format(L"{:.3f}", targetLocus->H2.y), TextAlign::Center);
	}
	else if (Type == 3) {
		Draw_Text_Dialog(pTarget, pBrush, Value1, std::format(L"{:.3f}", targetLocus->F.x), TextAlign::Center);
		Draw_Text_Dialog(pTarget, pBrush, Value2, std::format(L"{:.3f}", targetLocus->F.y), TextAlign::Center);
	}
	else if (Type == 4) {
		Draw_Text_Dialog(pTarget, pBrush, Value3, std::format(L"{:.3f}", targetLocus->H1.y * 100.0f), TextAlign::Center);
	}
}

struct DialogState {
	ComPtr<ID2D1HwndRenderTarget> pRenderTarget;
	ComPtr<ID2D1SolidColorBrush> pBrush;
	LOCUS* pTargetLocus = nullptr;
	int Type;
	CUR_MOD cursor = {};
	bool isActivated = false;
	D2D1_RECT_F winRect = { 0.0f, 0.0f, 0.0f, 0.0f };
};

bool ValueInputWindow(HWND hParent, const wchar_t* title, CUR Cursor, LOCUS& Locus, int Type = 0) {
	static const wchar_t* className = L"ValueInputDialog";
	static bool registered = false;

	static VALUEINPUT ValueImput;

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
				SetWindowTextW(hwnd, L"’l“ü—Í");
				if (pState) {
					pState->cursor.hwnd = hwnd;
				}
				return 0;
			}
			case WM_SIZE: {
				if (pState) {
					UINT width = LOWORD(lp);
					UINT height = HIWORD(lp);
					pState->winRect = D2D1::RectF(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

					if (pState->pRenderTarget) {
						pState->pRenderTarget->Resize(D2D1::SizeU(width, height));
					}
				}
				return 0;
			}
			case WM_ACTIVATE: {
				if (pState) {
					if (LOWORD(wp) != WA_INACTIVE) {
						pState->isActivated = true;
					}
					else {
						if (pState->isActivated) {
							KillTimer(hwnd, 3);
							PostMessage(hwnd, WM_CLOSE, 0, 0);
						}
					}
				}
				return 0;
			}
			case WM_TIMER: {
				if (pState && pState->pTargetLocus) {
					ValueImput.Update(pState->winRect, &pState->cursor, pState->pTargetLocus, pState->Type);
					InvalidateRect(hwnd, NULL, FALSE);
				}
				if (pState) {
					pState->cursor.wheel = 0.0f;
					pState->cursor.click = false;
				}
				return 0;
			}
			case WM_MOUSEMOVE: {
				if (pState) {
					pState->cursor.x = static_cast<float>(static_cast<short>(LOWORD(lp)));
					pState->cursor.y = static_cast<float>(static_cast<short>(HIWORD(lp)));
				}
				return 0;
			}
			case WM_LBUTTONDOWN: {
				SetFocus(hwnd);
				if (pState) pState->cursor.click = true;
				return 0;
			}
			case WM_LBUTTONUP: {
				if (pState) pState->cursor.click = false;
				return 0;
			}
			case WM_MOUSEWHEEL: {
				short delta = GET_WHEEL_DELTA_WPARAM(wp);
				if (pState) {
					pState->cursor.wheel = (std::min)((std::max)(pState->cursor.wheel + (float)(delta / 120), -1.5f), 1.5f);
				}
				return 0;
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

						ValueImput.Paint(pState->pRenderTarget.Get(), pState->pBrush.Get(), pState->winRect, pState->pTargetLocus, pState->Type);

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
				KillTimer(hwnd, 3);
				DestroyWindow(hwnd);
				return 0;

			case WM_DESTROY: {
				KillTimer(hwnd, 3);
				return 0;
			}
			case WM_NCDESTROY: {
				if (pState) {
					pState->pBrush.Reset();
					pState->pRenderTarget.Reset();
					delete pState;
					SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
				}
				return 0;
			}
			}
			return DefWindowProcW(hwnd, msg, wp, lp);
			};

		wc.hInstance = GetModuleHandle(NULL);
		wc.hbrBackground = NULL;
		wc.lpszClassName = className;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		RegisterClassW(&wc);
		registered = true;
	}

	DialogState* pState = new DialogState();
	pState->pTargetLocus = &Locus;
	pState->cursor = {};
	pState->Type = Type;
	pState->isActivated = false;

	RECT rcParent;
	GetWindowRect(hParent, &rcParent);
	int x = (int)(std::max)(static_cast<float>(rcParent.left) + Cursor.x - 75, 0.0f);
	int y = (int)(std::max)(static_cast<float>(rcParent.top) + Cursor.y - 35, 0.0f);

	HWND hDlg = CreateWindowExW(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		className, title,
		WS_POPUP | WS_BORDER,
		x, y, 150, 30,
		hParent, NULL, GetModuleHandle(NULL), pState
	);

	if (!hDlg) {
		delete pState;
		return false;
	}

	ShowWindow(hDlg, SW_SHOW);
	UpdateWindow(hDlg);

	MSG msg;
	BOOL bRet;
	while (IsWindow(hDlg) && (bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (bRet == -1) break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	SetForegroundWindow(hParent);

	return true;
}