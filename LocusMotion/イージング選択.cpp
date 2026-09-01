#define NOMINMAX
#include <windows.h>
#include "Direct2D.h"
#include "Dialog.h"
#include <d2d1_3.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>

struct CUR_EAS {
	float x = 0.0f;
	float y = 0.0f;
	bool click = false;
	bool RectCheck(D2D1_RECT_F Rect) {
		if (Rect.left < x && x < Rect.right && Rect.top < y && y < Rect.bottom) {
			return true;
		}
		return false;
	}
};

struct EASING {
	int ID = 1;
	float x = 0.0f;
	float y = 0.0f;
	float Size = 52.0f;
	LOCUS Locus;
	bool Hover = false;
	float HoverAnime = 0.0f;
	ComPtr<ID2D1PathGeometry> Geometry;

	int Update(CUR_EAS*Cursor);
	void Paint(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush);
	void UpdateGeometry();
};

void EASING::UpdateGeometry() {
	D2D1_RECT_F Rect = D2D1::RectF(x - Size / 2, y - Size / 2, x + Size / 2, y + Size / 2);
	std::vector<D2D1_POINT_2F> Samples;
	float width = Rect.right - Rect.left;
	float height = Rect.bottom - Rect.top;

	if (width <= 0.0f || height <= 0.0f) return;

	float n = (std::max)(width, 10.0f);
	for (int i = 0; i <= (int)n; i++) {
		float valX = (float)i / n;
		float valY = (float)Locus.LocusToValue(valX, 20);

		float drawX = Rect.left + valX * width;
		float drawY = Rect.bottom - valY * height;

		Samples.push_back(D2D1::Point2(drawX, drawY));
	}

	if (Samples.empty()) return;

	Geometry.Reset();
	HRESULT hr = g_pD2DFactory->CreatePathGeometry(&Geometry);
	if (FAILED(hr)) return;

	ComPtr<ID2D1GeometrySink> sink;
	hr = Geometry->Open(&sink);
	if (SUCCEEDED(hr)) {
		sink->BeginFigure(Samples[0], D2D1_FIGURE_BEGIN_HOLLOW);
		if (Samples.size() > 1) {
			sink->AddLines(Samples.data() + 1, static_cast<UINT32>(Samples.size() - 1));
		}
		sink->EndFigure(D2D1_FIGURE_END_OPEN);
		sink->Close();
	}
}

void Draw_Text_Dialog(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect, std::wstring Text, TextAlign align, float size) {
	if (!pTarget || !pBrush || !g_pTextFormat) return;

	IDWriteTextFormat* pFormatToUse = g_pTextFormat;
	IDWriteTextFormat* pCustomFormat = nullptr;

	if (size > 0.0f && g_pDWriteFactory) {
		UINT32 fontNameLength = g_pTextFormat->GetFontFamilyNameLength() + 1;
		std::vector<wchar_t> fontName(fontNameLength);
		g_pTextFormat->GetFontFamilyName(fontName.data(), fontNameLength);

		UINT32 localeNameLength = g_pTextFormat->GetLocaleNameLength() + 1;
		std::vector<wchar_t> localeName(localeNameLength);
		g_pTextFormat->GetLocaleName(localeName.data(), localeNameLength);

		HRESULT hr = g_pDWriteFactory->CreateTextFormat(
			fontName.data(),
			NULL,
			g_pTextFormat->GetFontWeight(),
			g_pTextFormat->GetFontStyle(),
			g_pTextFormat->GetFontStretch(),
			size,
			localeName.data(),
			&pCustomFormat
		);

		if (SUCCEEDED(hr)) {
			pCustomFormat->SetParagraphAlignment(g_pTextFormat->GetParagraphAlignment());
			pFormatToUse = pCustomFormat;
		}
	}

	switch (align) {
	case TextAlign::Left:
		pFormatToUse->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		break;
	case TextAlign::Center:
		pFormatToUse->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		break;
	case TextAlign::Right:
		pFormatToUse->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
		break;
	}

	const wchar_t* text1 = Text.c_str();
	pTarget->DrawText(
		text1,
		(UINT32)wcslen(text1),
		pFormatToUse,
		Rect,
		pBrush
	);

	if (pCustomFormat) {
		pCustomFormat->Release();
	}
}

void Draw_Svg_Dialog(ID2D1RenderTarget* pTarget, int resourceId, D2D1_RECT_F rect, float size, float angle, D2D1_COLOR_F color) {
	if (!pTarget) return;

	ComPtr<ID2D1DeviceContext5> pDC;
	if (FAILED(pTarget->QueryInterface(IID_PPV_ARGS(&pDC))) || !pDC) return;

	// DLL自身のモジュールハンドルを取得する
	HMODULE hInst = NULL;
	GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCWSTR>(&Draw_Svg_Dialog),
		&hInst
	);

	HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(resourceId), RT_RCDATA);
	if (!hRes) return; // ← GetModuleHandle(NULL) だとここで失敗していました

	HGLOBAL hMem = LoadResource(hInst, hRes);
	if (!hMem) return;

	DWORD resSize = SizeofResource(hInst, hRes);
	void* pData = LockResource(hMem);
	if (!pData) return;

	IStream* pStream = SHCreateMemStream(static_cast<const BYTE*>(pData), resSize);
	if (!pStream) return;

	ComPtr<ID2D1SvgDocument> pSvg;
	HRESULT hr = pDC->CreateSvgDocument(pStream, D2D1::SizeF(100.0f, 100.0f), &pSvg);
	pStream->Release();
	if (FAILED(hr) || !pSvg) return;

	// Root要素の取得と viewBox / 色 / 透明度の設定
	ComPtr<ID2D1SvgElement> pRoot;
	pSvg->GetRoot(&pRoot);
	if (pRoot) {
		// ViewBoxの設定を適用（描画サイズの正常化）
		D2D1_SVG_VIEWBOX viewBox = {};
		if (SUCCEEDED(pRoot->GetAttributeValue(L"viewBox", D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX, &viewBox, sizeof(viewBox)))) {
			if (viewBox.width > 0.0f && viewBox.height > 0.0f) {
				pSvg->SetViewportSize(D2D1::SizeF(viewBox.width, viewBox.height));
			}
		}

		// 色設定
		wchar_t colorStr[64];
		swprintf_s(colorStr, L"rgb(%d, %d, %d)",
			static_cast<int>(color.r * 255.0f),
			static_cast<int>(color.g * 255.0f),
			static_cast<int>(color.b * 255.0f));
		pRoot->SetAttributeValue(L"fill", D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, colorStr);

		// 不透明度設定
		wchar_t opacityStr[32];
		swprintf_s(opacityStr, L"%f", color.a);
		pRoot->SetAttributeValue(L"fill-opacity", D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, opacityStr);
	}

	// 描画サイズと行列変形
	D2D1_SIZE_F originalSize = pSvg->GetViewportSize();
	if (originalSize.width <= 0.0f || originalSize.height <= 0.0f) {
		originalSize = D2D1::SizeF(100.0f, 100.0f);
	}

	float rectWidth = rect.right - rect.left;
	float rectHeight = rect.bottom - rect.top;
	if (rectWidth <= 0.0f || rectHeight <= 0.0f) return;

	float rectCenterX = rect.left + rectWidth / 2.0f;
	float rectCenterY = rect.top + rectHeight / 2.0f;

	float scaleX = rectWidth / originalSize.width;
	float scaleY = rectHeight / originalSize.height;
	float baseScale = (std::min)(scaleX, scaleY);
	float finalScale = baseScale * size;

	D2D1_MATRIX_3X2_F oldTransform;
	pDC->GetTransform(&oldTransform);

	D2D1_MATRIX_3X2_F transform =
		D2D1::Matrix3x2F::Translation(-originalSize.width / 2.0f, -originalSize.height / 2.0f) *
		D2D1::Matrix3x2F::Rotation(angle) *
		D2D1::Matrix3x2F::Scale(finalScale, finalScale) *
		D2D1::Matrix3x2F::Translation(rectCenterX, rectCenterY);

	pDC->SetTransform(transform * oldTransform);
	pDC->DrawSvgDocument(pSvg.Get());
	pDC->SetTransform(oldTransform);
}

struct DialogState {
	int resultValue = 0;
	bool confirmed = false;
	HWND hEdit = NULL;
	ID2D1HwndRenderTarget* pRenderTarget = nullptr;
	ID2D1SolidColorBrush* pBrush = nullptr;
};

void EASING::Paint(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush) {
	if (!pTarget || !pBrush) return;
	D2D1_RECT_F Rect = D2D1::RectF(x - Size / 2, y - Size / 2, x + Size / 2, y + Size / 2);
	pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Text")));
	if ((ID - 2) % 4 == 0 || ID == 1) {
		Draw_Text_Dialog(pTarget, pBrush, D2D1::RectF(Rect.left, Rect.top, Rect.right, Rect.bottom - 30.0f), std::to_wstring(ID), TextAlign::Left, 17);
	}
	else {
		Draw_Text_Dialog(pTarget, pBrush, D2D1::RectF(Rect.left, Rect.top + 30.0f, Rect.right, Rect.bottom), std::to_wstring(ID), TextAlign::Right, 17);
	}

	if (Geometry) {
		pBrush->SetColor(D2D1::ColorF(0x54d1ff));
		pTarget->DrawGeometry(Geometry.Get(), pBrush, 2.0f);
	}

	pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Border")));
	pTarget->DrawRectangle(D2D1::RectF(Rect.left - 1.0f, Rect.top - 1.0f, Rect.right + 1.0f, Rect.bottom + 1.0f), pBrush, 1.0f);
	pBrush->SetColor(D2D1::ColorF(0xffffff, HoverAnime * 0.1f));
	pTarget->FillRectangle(Rect, pBrush);
}

int EASING::Update(CUR_EAS* Cursor) {
	int Return = 0;
	D2D1_RECT_F Rect = D2D1::RectF(x - Size / 2, y - Size / 2, x + Size / 2, y + Size / 2);
	Hover = Cursor->RectCheck(Rect);
	HoverAnime += ((int)Hover - HoverAnime) / 1.8f;
	if (Hover && Cursor->click) {
		return ID;
	}
	if (abs(HoverAnime - (int)Hover) < 0.001) {
		HoverAnime = (float)Hover;
	}
	else {
		Return = -1;
	}
	return Return;
}

bool EasingWindow(HWND hParent, const wchar_t* title, int& outValue, CUR Cursor) {
	static const wchar_t* className = L"EasingNumberInputDialog";
	static bool registered = false;

	static std::vector<EASING> Easing;
	static CUR_EAS cursor;

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
				Easing.clear();
				RECT rc;
				GetClientRect(hwnd, &rc);
				D2D1_RECT_F WinRect = D2D1::RectF(0.0f, 0.0f, (float)rc.right, (float)rc.bottom);

				float PosX[4];
				for (int i = 0; i < 4; i++) {
					PosX[i] = (WinRect.right / 16.0f + WinRect.right / 8.0f * i - WinRect.right / 4.0f) / 1.05f;
				}
				float PosY[6];
				for (int i = 0; i < 6; i++) {
					PosY[i] = WinRect.bottom / 12.0f + (WinRect.bottom / 6.0f) * i;
				}
				LOCUS AddLocus;
				EASING AddEasing;
				AddLocus.Mode = 3;
				AddLocus.Turn = false;
				AddLocus.H1.y = 1.0f;

				AddEasing.ID = 1;
				AddEasing.Locus = AddLocus;
				AddEasing.x = WinRect.right / 4.0f + PosX[0];
				AddEasing.y = PosY[0];
				AddEasing.UpdateGeometry();
				Easing.push_back(AddEasing);

				for (int i = 0; i < 40; i++) {
					AddLocus.H1.y = (float)(i + 2);
					AddEasing.ID = i + 2;
					AddEasing.Locus = AddLocus;
					AddEasing.x = ((i % 8 < 4) ? WinRect.right / 4.0f : WinRect.right / 4.0f * 3.0f) + PosX[i % 4];
					AddEasing.y = PosY[i / 8 + 1];
					AddEasing.UpdateGeometry();
					Easing.push_back(AddEasing);
				}

				SetTimer(hwnd, 2, 1000 / 60, NULL);
				return 0;
			}
			case WM_TIMER: {
				bool Paint = false;
				for (auto& e : Easing) {
					int Result = e.Update(&cursor);
					if (Result != 0) {
						if (Result == -1) {
							Paint = true;
						}
						else {
							if (pState) {
								pState->resultValue = Result;
								pState->confirmed = true;
							}
							DestroyWindow(hwnd);
							break;
						}
					}
				}
				if (Paint) {
					InvalidateRect(hwnd, NULL, FALSE);
				}
				cursor.click = false;
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
							D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)),
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
						pState->pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF(config->get_color_code(config, "Grouping"))));
						for (auto& e : Easing) {
							e.Paint(pState->pRenderTarget, pState->pBrush);
						}

						HRESULT hr = pState->pRenderTarget->EndDraw();
						if (hr == D2DERR_RECREATE_TARGET) {
							if (pState->pBrush) { pState->pBrush->Release(); pState->pBrush = nullptr; }
							if (pState->pRenderTarget) { pState->pRenderTarget->Release(); pState->pRenderTarget = nullptr; }
						}
					}
				}

				EndPaint(hwnd, &ps);
				return 0;
			}
			case WM_MOUSEMOVE: {
				short mx = (short)LOWORD(lp);
				short my = (short)HIWORD(lp);
				cursor.x = (float)mx;
				cursor.y = (float)my;
				break;
			}
			case WM_LBUTTONDOWN: {
				cursor.click = true;
				break;
			}
			case WM_CLOSE:
				DestroyWindow(hwnd);
				return 0;

			case WM_DESTROY: {
				KillTimer(hwnd, 2);
				Easing.clear();
				if (pState) {
					if (pState->pBrush) { pState->pBrush->Release(); pState->pBrush = nullptr; }
					if (pState->pRenderTarget) { pState->pRenderTarget->Release(); pState->pRenderTarget = nullptr; }
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
	int x = (int)(std::max)((float)rcParent.left + (int)Cursor.x - 250, 0.0f);
	int y = (int)(std::max)((float)rcParent.top + (int)Cursor.y - 225 + 50, 0.0f);

	HWND hDlg = CreateWindowExW(
		WS_EX_DLGMODALFRAME,
		className, title,
		WS_POPUP | WS_CAPTION | WS_SYSMENU,
		x, y, 500, 500,
		hParent, NULL, GetModuleHandle(NULL), &state
	);

	if (!hDlg) return false;

	EnableWindow(hParent, FALSE);
	ShowWindow(hDlg, SW_SHOW);
	UpdateWindow(hDlg);

	MSG msg;
	BOOL bRet;
	while (IsWindow(hDlg) && (bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (bRet == -1) {
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	EnableWindow(hParent, TRUE);
	SetForegroundWindow(hParent);

	if (state.confirmed) {
		outValue = state.resultValue;
		return true;
	}
	return false;
}