#pragma once

#include "GUI.h"
#include <windows.h>
#include <d2d1.h>
#include <d2d1_3.h>
#include <dwrite.h>
#include <shlwapi.h>
#include <commctrl.h>

#include <string>
#include <vector>
#include <unordered_map>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "shlwapi.lib")

#include "plugin2.h"
#include "logger2.h" 
#include "config2.h" 
#include "module2.h"

extern EDIT_HANDLE* edit_handle;
extern LOG_HANDLE* logger;
extern CONFIG_HANDLE* config;

extern ID2D1Factory* g_pD2DFactory;
extern ID2D1HwndRenderTarget* g_pRenderTarget;
extern ID2D1DeviceContext5* g_pDeviceContext;
extern ID2D1SolidColorBrush* g_pBrush;

extern IDWriteFactory* g_pDWriteFactory;
extern IDWriteTextFormat* g_pTextFormat;

extern std::unordered_map<int, ID2D1SvgDocument*> g_pSvgMap;

HRESULT CreateDeviceResources(HWND hwnd);
void InitDirectWrite();
void DiscardDeviceResources();

HRESULT LoadSvgFromResource(HINSTANCE hInstance, int resourceId);

void Draw_Svg(int resourceId, D2D1_RECT_F rect, float size, float angle, D2D1_COLOR_F color);

enum class TextAlign {
    Left,
    Center,
    Right
};
void Draw_Text(D2D1_RECT_F Rect, std::wstring Text, TextAlign align = TextAlign::Left, float size = 0.0f);