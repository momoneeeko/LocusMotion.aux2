#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>
#include "Direct2D.h"
#include "Dialog.h"
#include <algorithm>

using Microsoft::WRL::ComPtr;

struct CUR_MOD {
	HWND hwnd = NULL;
	float x = 0.0f;
	float y = 0.0f;
	bool click = false;
	bool RectCheck(D2D1_RECT_F Rect) const {
		return (Rect.left < x && x < Rect.right && Rect.top < y && y < Rect.bottom);
	}
};

struct MODDIALOG {
	std::vector<MODIFIER> Modifier;
	bool AddHover = false;
	float AddHoverAnime = 0.0f;

	bool Updata(D2D1_RECT_F Rect, CUR_MOD* Cursor);
	void Paint(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect);
};

bool MODDIALOG::Updata(D2D1_RECT_F Rect, CUR_MOD* Cursor) {
	bool Return = false;
	D2D1_RECT_F AddButton = D2D1::RectF(Rect.left + 5.0f, Rect.top + 5.0f, Rect.right - 5.0f, Rect.top + 30.0f);

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
		Cursor->click = false;
		Cursor->x = 0.0f;
		Cursor->y = 0.0f;

		HMENU hMenu = CreatePopupMenu();
		if (hMenu) {
			AppendMenuW(hMenu, MF_STRING, 101, L"モディファイアは未実装です");

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
			case 101:
				break;
			}
		}

		isShowingMenu = false;
	}

	return Return;
}

void MODDIALOG::Paint(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect) {
	if (!pTarget || !pBrush) return;

	D2D1_RECT_F AddButton = D2D1::RectF(Rect.left + 5.0f, Rect.top + 5.0f, Rect.right - 5.0f, Rect.top + 30.0f);

	COLORREF normalC = 0x444444;
	COLORREF hoverC = 0x666666;
	COLORREF borderC = 0x888888;
	COLORREF textC = 0xFFFFFF;

	if (config && config->get_color_code) {
		normalC = config->get_color_code(config, "ButtonBody");
		hoverC = config->get_color_code(config, "ButtonBodyHover");
		borderC = config->get_color_code(config, "Border");
		textC = config->get_color_code(config, "Text");
	}

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
}

struct DialogState {
	ComPtr<ID2D1HwndRenderTarget> pRenderTarget;
	ComPtr<ID2D1SolidColorBrush> pBrush;
};

bool ModifierWindow(HWND hParent, const wchar_t* title, int& outValue, CUR Cursor) {
	static const wchar_t* className = L"ModifierDialog";
	static bool registered = false;

	static MODDIALOG Modifier;
	static CUR_MOD cursor;
	static D2D1_RECT_F WinRect;

	cursor = CUR_MOD{};

	DialogState state;

	if (!registered) {
		WNDCLASSW wc = { 0 };
		wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
			DialogState* pState = reinterpret_cast<DialogState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

			switch (msg) {
			case WM_NCCREATE: {
				CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
				SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
				return DefWindowProcW(hwnd, msg, wp, lp); // 修正
			}
			case WM_CREATE: {
				SetTimer(hwnd, 3, 1000 / 60, NULL);
				SetWindowTextW(hwnd, L"モディファイア");
				cursor.hwnd = hwnd;
				return 0;
			}
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
				if (Modifier.Updata(WinRect, &cursor)) {
					InvalidateRect(hwnd, NULL, FALSE);
				}
				return 0;
			}
			case WM_MOUSEMOVE: {
				cursor.x = static_cast<float>(static_cast<short>(LOWORD(lp)));
				cursor.y = static_cast<float>(static_cast<short>(HIWORD(lp)));
				return 0;
			}
			case WM_LBUTTONDOWN: {
				cursor.click = true;
				InvalidateRect(hwnd, NULL, FALSE);
				return 0;
			}
			case WM_LBUTTONUP: {
				cursor.click = false;
				return 0;
			}
			case WM_PAINT: {
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);

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
							bgColor = config->get_color_code(config, "Grouping");
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