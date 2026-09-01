#pragma once
#include <windows.h>
#include "GUI.h"

void Draw_Text_Dialog(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect, std::wstring Text, TextAlign align = TextAlign::Center, float size = 0.0f);
void Draw_Svg_Dialog(ID2D1RenderTarget* pTarget, int resourceId, D2D1_RECT_F rect, float size, float angle, D2D1_COLOR_F color);
// 数値入力ダイアログを表示する関数の宣言
bool EasingWindow(HWND hParent, const wchar_t* title, int& outValue, CUR Cursor);

bool ModifierWindow(HWND hParent, const wchar_t* title, CUR Cursor, LOCUSES& targetLocuses);