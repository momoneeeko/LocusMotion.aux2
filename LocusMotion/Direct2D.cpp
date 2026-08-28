#include "Direct2D.h"
#include "resource1.h"

EDIT_HANDLE* edit_handle = nullptr;
LOG_HANDLE* logger = nullptr;
CONFIG_HANDLE* config = nullptr;

ID2D1Factory* g_pD2DFactory = nullptr;
ID2D1HwndRenderTarget* g_pRenderTarget = nullptr;
ID2D1DeviceContext5* g_pDeviceContext = nullptr;
ID2D1SolidColorBrush* g_pBrush = nullptr;

IDWriteFactory* g_pDWriteFactory = nullptr;
IDWriteTextFormat* g_pTextFormat = nullptr;

std::unordered_map<int, ID2D1SvgDocument*> g_pSvgMap;

HRESULT CreateDeviceResources(HWND hwnd) {
	HRESULT hr = S_OK;

	if (!g_pD2DFactory) {
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
	}

	if (SUCCEEDED(hr) && !g_pDWriteFactory) {
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&g_pDWriteFactory)
		);

		if (SUCCEEDED(hr)) {
			LPCWSTR font_name = L"Meiryo";
			float font_size = 16.0f;

			if (config && config->get_font_info) {
				FONT_INFO* font_info = config->get_font_info(config, "Default");

				if (font_info && font_info->name) {
					font_name = font_info->name;
					font_size = font_info->size;
				}
			}

			hr = g_pDWriteFactory->CreateTextFormat(
				font_name,
				NULL,
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				font_size,
				L"ja-JP",
				&g_pTextFormat
			);

			if (SUCCEEDED(hr)) {
				g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			}
		}
	}

	if (SUCCEEDED(hr) && !g_pRenderTarget) {
		RECT rc;
		GetClientRect(hwnd, &rc);

		hr = g_pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)),
			&g_pRenderTarget
		);

		if (SUCCEEDED(hr)) {
			hr = g_pRenderTarget->QueryInterface(__uuidof(ID2D1DeviceContext5), reinterpret_cast<void**>(&g_pDeviceContext));
		}

		if (SUCCEEDED(hr)) {
			hr = g_pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White),
				&g_pBrush
			);
		}

		if (SUCCEEDED(hr)) {
			HMODULE hInst = NULL;
			GetModuleHandleExW(
				GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				reinterpret_cast<LPCWSTR>(&CreateDeviceResources),
				&hInst
			);

			//SVGリソースをロード
			LoadSvgFromResource(hInst, IDR_SVG_OPEN);
			LoadSvgFromResource(hInst, IDR_SVG_BEZIER);
			LoadSvgFromResource(hInst, IDR_SVG_ELASTIC);
			LoadSvgFromResource(hInst, IDR_SVG_BOUNCE);
			LoadSvgFromResource(hInst, IDR_SVG_Easing);
			LoadSvgFromResource(hInst, IDR_SVG_ANGLE);
			LoadSvgFromResource(hInst, IDR_SVG_TURN);
			LoadSvgFromResource(hInst, IDR_SVG_RETURN);
			LoadSvgFromResource(hInst, IDR_SVG_EFFECTHIDE);
		}
	}
	return hr;
}

void InitDirectWrite() {
	if (!g_pDWriteFactory) {
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&g_pDWriteFactory)
		);

		if (g_pDWriteFactory) {
			g_pDWriteFactory->CreateTextFormat(
				L"Meiryo",
				NULL,
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				16.0f,
				L"ja-JP",
				&g_pTextFormat
			);

			if (g_pTextFormat) {
				g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			}
		}
	}
}

void DiscardDeviceResources() {
	for (auto& pair : g_pSvgMap) {
		if (pair.second) {
			pair.second->Release();
		}
	}
	g_pSvgMap.clear();

	if (g_pDeviceContext) { g_pDeviceContext->Release(); g_pDeviceContext = nullptr; }
	if (g_pBrush) { g_pBrush->Release(); g_pBrush = nullptr; }
	if (g_pRenderTarget) { g_pRenderTarget->Release(); g_pRenderTarget = nullptr; }
	if (g_pTextFormat) { g_pTextFormat->Release(); g_pTextFormat = nullptr; }
	if (g_pDWriteFactory) { g_pDWriteFactory->Release(); g_pDWriteFactory = nullptr; }
}


void Draw_Text(D2D1_RECT_F Rect, std::wstring Text, TextAlign align, float size) {
	if (!g_pRenderTarget || !g_pBrush || !g_pTextFormat) return;

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

	// Align の適用（カスタムフォーマット作成時、または一時的な設定変更）
	DWRITE_TEXT_ALIGNMENT dwriteAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
	switch (align) {
	case TextAlign::Left:   dwriteAlign = DWRITE_TEXT_ALIGNMENT_LEADING; break;
	case TextAlign::Center: dwriteAlign = DWRITE_TEXT_ALIGNMENT_CENTER; break;
	case TextAlign::Right:  dwriteAlign = DWRITE_TEXT_ALIGNMENT_TRAILING; break;
	}

	if (pCustomFormat) {
		pCustomFormat->SetTextAlignment(dwriteAlign);
	}
	else {
		g_pTextFormat->SetTextAlignment(dwriteAlign);
	}

	COLORREF textColor = (config && config->get_color_code) ? config->get_color_code(config, "Text") : 0xFFFFFF;
	g_pBrush->SetColor(D2D1::ColorF(textColor));

	const wchar_t* text1 = Text.c_str();
	g_pRenderTarget->DrawText(
		text1,
		(UINT32)wcslen(text1),
		pFormatToUse,
		Rect,
		g_pBrush
	);

	if (pCustomFormat) {
		pCustomFormat->Release();
	}
}

HRESULT LoadSvgFromResource(HINSTANCE hInstance, int resourceId) {
	if (!g_pDeviceContext) return E_FAIL;

	auto it = g_pSvgMap.find(resourceId);
	if (it != g_pSvgMap.end()) {
		if (it->second) it->second->Release();
		g_pSvgMap.erase(it);
	}

	HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(resourceId), RT_RCDATA);
	if (!hRes) return E_FAIL;

	HGLOBAL hMem = LoadResource(hInstance, hRes);
	if (!hMem) return E_FAIL;

	DWORD size = SizeofResource(hInstance, hRes);
	void* pData = LockResource(hMem);
	if (!pData) return E_FAIL;

	IStream* pStream = SHCreateMemStream(static_cast<const BYTE*>(pData), size);
	if (!pStream) return E_OUTOFMEMORY;

	ID2D1SvgDocument* pSvgDocument = nullptr;
	HRESULT hr = g_pDeviceContext->CreateSvgDocument(
		pStream,
		D2D1::SizeF(100.0f, 100.0f),
		&pSvgDocument
	);

	pStream->Release();

	if (SUCCEEDED(hr) && pSvgDocument) {
		ID2D1SvgElement* pRoot = nullptr;
		pSvgDocument->GetRoot(&pRoot);
		if (pRoot) {
			D2D1_SVG_VIEWBOX viewBox = {};
			if (SUCCEEDED(pRoot->GetAttributeValue(L"viewBox", D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX, &viewBox, sizeof(viewBox)))) {
				if (viewBox.width > 0.0f && viewBox.height > 0.0f) {
					pSvgDocument->SetViewportSize(D2D1::SizeF(viewBox.width, viewBox.height));
				}
			}
			pRoot->Release();
		}

		g_pSvgMap[resourceId] = pSvgDocument;
	}

	return hr;
}

void Draw_Svg(int resourceId, D2D1_RECT_F rect, float size, float angle, D2D1_COLOR_F color) {
	if (!g_pDeviceContext) return;

	auto it = g_pSvgMap.find(resourceId);
	if (it == g_pSvgMap.end() || !it->second) return;

	ID2D1SvgDocument* pSvg = it->second;

	ID2D1SvgElement* pRoot = nullptr;
	pSvg->GetRoot(&pRoot);
	if (pRoot) {
		wchar_t colorStr[64];
		swprintf_s(
			colorStr,
			L"rgb(%d, %d, %d)",
			static_cast<int>(color.r * 255.0f),
			static_cast<int>(color.g * 255.0f),
			static_cast<int>(color.b * 255.0f)
		);

		pRoot->SetAttributeValue(L"fill", D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, colorStr);
		wchar_t opacityStr[32];
		swprintf_s(opacityStr, L"%f", color.a);
		pRoot->SetAttributeValue(L"fill-opacity", D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, opacityStr);

		pRoot->Release();
	}

	D2D1_SIZE_F originalSize = pSvg->GetViewportSize();
	if (originalSize.width <= 0.0f || originalSize.height <= 0.0f) {
		originalSize = D2D1::SizeF(100.0f, 100.0f);
	}

	float rectWidth = rect.right - rect.left;
	float rectHeight = rect.bottom - rect.top;
	float rectCenterX = rect.left + rectWidth / 2.0f;
	float rectCenterY = rect.top + rectHeight / 2.0f;

	float scaleX = rectWidth / originalSize.width;
	float scaleY = rectHeight / originalSize.height;
	float baseScale = (std::min)(scaleX, scaleY);
	float finalScale = baseScale * size;

	D2D1_MATRIX_3X2_F oldTransform;
	g_pDeviceContext->GetTransform(&oldTransform);

	D2D1_MATRIX_3X2_F transform =
		D2D1::Matrix3x2F::Translation(-originalSize.width / 2.0f, -originalSize.height / 2.0f) *
		D2D1::Matrix3x2F::Rotation(angle) *
		D2D1::Matrix3x2F::Scale(finalScale, finalScale) *
		D2D1::Matrix3x2F::Translation(rectCenterX, rectCenterY);

	g_pDeviceContext->SetTransform(transform * oldTransform);
	g_pDeviceContext->DrawSvgDocument(pSvg);
	g_pDeviceContext->SetTransform(oldTransform);
}