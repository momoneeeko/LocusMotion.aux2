#pragma once
#include <windows.h>
#include "GUI.h"

void Draw_Text_Dialog(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect, std::wstring Text, TextAlign align = TextAlign::Center, float size = 0.0f);
// 数値入力ダイアログを表示する関数の宣言
bool EasingWindow(HWND hParent, const wchar_t* title, int& outValue, CUR Cursor);

bool ModifierWindow(HWND hParent, const wchar_t* title, int& outValue, CUR Cursor);