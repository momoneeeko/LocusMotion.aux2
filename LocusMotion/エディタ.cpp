#include "Direct2D.h"
#include "GUI.h"
#include "Dialog.h"
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

#define PI 3.14159265359f




bool EDIT_LOCUSES::Update(D2D1_RECT_F Rect, CUR* Cursor, D2D1_POINT_2F OllCenter, float Size, const EDIT_LOCUSES* prevLocus, const EDIT_LOCUSES* nextLocus) const {
	bool Return = false;
	Center = D2D1::Point2(((Locus[Mode].S.x + Locus[Mode].F.x) - 1.0f) * Size + OllCenter.x, (-(Locus[Mode].S.y + Locus[Mode].F.y) + 1.0f) * Size + OllCenter.y);
	SizeXG = (Locus[Mode].F.x - Locus[Mode].S.x) * Size;
	SizeYG = (Locus[Mode].F.y - Locus[Mode].S.y) * Size;
	SizeX = (Locus[Mode].F.x - Locus[Mode].S.x) * Size * (float)(1 - (int)Locus[Mode].Turn * 2);
	SizeY = (Locus[Mode].F.y - Locus[Mode].S.y) * Size * (float)(1 - (int)Locus[Mode].Turn * 2);

	D2D1_POINT_2F SPoint = D2D1::Point2(-1.0f * SizeX + Center.x, 1.0f * SizeY + Center.y);
	D2D1_POINT_2F H1Point = D2D1::Point2((Locus[Mode].H1.x * 2.0f - 1.0f) * SizeX + Center.x, (Locus[Mode].H1.y * -2.0f + 1.0f) * SizeY + Center.y);
	D2D1_POINT_2F H2Point = D2D1::Point2((Locus[Mode].H2.x * 2.0f - 1.0f) * SizeX + Center.x, (Locus[Mode].H2.y * -2.0f + 1.0f) * SizeY + Center.y);
	D2D1_POINT_2F FPoint = D2D1::Point2(1.0f * SizeX + Center.x, -1.0f * SizeY + Center.y);

	if ( Mode == 1 ||  Mode == 2) {
		float Rwidth = SizeX / 1.6f;
		H1Point = D2D1::Point2(SPoint.x + cos(Locus[Mode].H1.y * 100 / 180 * PI) * (SizeX / 1.6f), SPoint.y + -sin(Locus[Mode].H1.y * 100 / 180 * PI) * (SizeY / 1.6f));
	}

	if (Cursor->click && (Cursor->RangeCheck(L"LocusEditor"))) {
		auto CheckHit = [](D2D1_POINT_2F pt, POS_UI pos, CUR cur) {
			return (cur.x >= pt.x - pos.w / 2.0f && cur.x <= pt.x + pos.w / 2.0f &&
				cur.y >= pt.y - pos.h / 2.0f && cur.y <= pt.y + pos.h / 2.0f);
			};
		if (Locus[Mode].Turn) {
			if (CheckHit(SPoint, LocusUI[Mode].S, *Cursor)) {
				Cursor->drag = false;
				LocusUI[Mode].F.Clicking = true;
			}
			else if (CheckHit(H1Point, LocusUI[Mode].H1, *Cursor)) {
				Cursor->drag = false;
				LocusUI[Mode].H1.Clicking = true;
			}
			else if (CheckHit(H2Point, LocusUI[Mode].H2, *Cursor)) {
				Cursor->drag = false;
				LocusUI[Mode].H2.Clicking = true;
			}
			else if (CheckHit(FPoint, LocusUI[Mode].F, *Cursor)) {
				Cursor->drag = false;
				LocusUI[Mode].S.Clicking = true;
			}
		}
		else {
			if (CheckHit(SPoint, LocusUI[Mode].S, *Cursor)) {
				Cursor->drag = false;
				LocusUI[Mode].S.Clicking = true;
			}
			else if (CheckHit(H1Point, LocusUI[Mode].H1, *Cursor)) {
				Cursor->drag = false;
				LocusUI[Mode].H1.Clicking = true;
			}
			else if (CheckHit(H2Point, LocusUI[Mode].H2, *Cursor)) {
				Cursor->drag = false;
				LocusUI[Mode].H2.Clicking = true;
			}
			else if (CheckHit(FPoint, LocusUI[Mode].F, *Cursor)) {
				Cursor->drag = false;
				LocusUI[Mode].F.Clicking = true;
			}
		}
	}

	if (!Cursor->clicking) {
		LocusUI[Mode].S.Clicking = false;
		LocusUI[Mode].H1.Clicking = false;
		LocusUI[Mode].H2.Clicking = false;
		LocusUI[Mode].F.Clicking = false;
	}

	if (Cursor->moveing) {
		auto UpdatePos = [&](POS& pos, POS_UI& posUI, int PosTipe, float x, float y) {
			if (posUI.Clicking) {
				float prevS_x = prevLocus ? prevLocus->Locus[Mode].S.x : 0.0f;
				float nextF_x = nextLocus ? nextLocus->Locus[Mode].F.x : 1.0f;

				float minSx = (Tipe == 0 || Tipe == 1) ? 0.0f : prevS_x;
				float maxFx = (Tipe == 0 || Tipe == 3) ? 1.0f : nextF_x;
				if ( Mode == 0) {
					if (PosTipe == 0) {
						if (!(Tipe == 0)) {
							if (Tipe == 2 || Tipe == 3) { pos.x = (std::min)((std::max)(x, minSx), Locus[Mode].F.x); }
							if (Tipe == 2 || Tipe == 3) { pos.y = y; }
						}
					}
					if (PosTipe == 1) {
						pos.x = (std::min)((std::max)(x, 0.0f), 1.0f);
						if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
							pos.y = (std::min)((std::max)(round(y), 0.0f), 1.0f);
						}
						else {
							pos.y = y;
						}
						if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
							Locus[Mode].H2.x = 1.0f - pos.x;
							Locus[Mode].H2.y = 1.0f - pos.y;
						}
						else if (GetAsyncKeyState(VK_MENU) & 0x8000) {
							Locus[Mode].H2.x = (std::min)((std::max)(1.0f - pos.y, 0.0f), 1.0f);
							Locus[Mode].H2.y = 1.0f - pos.x;
						}
					}
					if (PosTipe == 2) {
						pos.x = min(max(x, 0.0f), 1.0f);
						if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
							pos.y = (std::min)((std::max)(round(y), 0.0f), 1.0f);
						}
						else {
							pos.y = y;
						}
						if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
							Locus[Mode].H1.x = 1.0f - pos.x;
							Locus[Mode].H1.y = 1.0f - pos.y;
						}
						if (GetAsyncKeyState(VK_MENU) & 0x8000) {
							Locus[Mode].H1.x = (std::min)((std::max)(1.0f - pos.y, 0.0f), 1.0f);
							Locus[Mode].H1.y = 1.0f - pos.x;
						}
					}
					if (PosTipe == 3) {
						if (!(Tipe == 0)) {
							if (Tipe == 2 || Tipe == 1) { pos.x = (std::min)((std::max)(x, Locus[Mode].S.x), maxFx); }
							if (Tipe == 2 || Tipe == 1) { pos.y = y; }
						}
					}
				}
				else if ( Mode == 1) {
					if (PosTipe == 0) {
						if (!(Tipe == 0)) {
							if (Tipe == 2 || Tipe == 3) { pos.x = (std::min)((std::max)(x, minSx), Locus[Mode].F.x); }
							if (Tipe == 2 || Tipe == 3) { pos.y = y; }
						}
					}
					if (PosTipe == 1) {
						if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
							pos.y = (std::min)((std::max)((round(atan2((std::max)(x, 0.0f), -y) / (PI / 12.0f)) * (PI / 12.0f) / PI * 1.8f - 0.9f), -0.8999f), 0.8999f);
						}
						else {
							pos.y = (std::min)((std::max)((atan2((std::max)(x, 0.0f), -y) / PI * 1.8f - 0.9f), -0.8999f), 0.8999f);
						}
					}
					if (PosTipe == 2) {
						pos.x = (std::min)((std::max)(x, 0.0f), 0.999f);
						pos.y = (std::min)((std::max)(y, 0.0f), 1.0f);
					}
					if (PosTipe == 3) {
						if (!(Tipe == 0)) {
							if (Tipe == 2 || Tipe == 1) { pos.x = (std::min)((std::max)(x, Locus[Mode].S.x), maxFx); }
							if (Tipe == 2 || Tipe == 1) { pos.y = y; }
						}
					}
				}
				else if ( Mode == 2) {
					if (PosTipe == 0) {
						if (!(Tipe == 0)) {
							if (Tipe == 2 || Tipe == 3) { pos.x = (std::min)((std::max)(x, minSx), Locus[Mode].F.x); }
							if (Tipe == 2 || Tipe == 3) { pos.y = y; }
						}
					}
					if (PosTipe == 1) {
						if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
							pos.y = (std::min)((std::max)((round(atan2((std::max)(x, 0.0f), -y) / (PI / 12.0f)) * (PI / 12.0f) / PI * 1.8f - 0.9f), -0.8999f), 0.8999f);
						}
						else {
							pos.y = (std::min)((std::max)((atan2((std::max)(x, 0.0f), -y) / PI * 1.8f - 0.9f), -0.8999f), 0.8999f);
						}
					}
					if (PosTipe == 2) {
						pos.x = (std::min)((std::max)(x, 0.0f), 1.0f);
						pos.y = (std::min)((std::max)(y, 0.0001f), 0.9999f);
					}
					if (PosTipe == 3) {
						if (!(Tipe == 0)) {
							if (Tipe == 2 || Tipe == 1) { pos.x = (std::min)((std::max)(x, Locus[Mode].S.x), maxFx); }
							if (Tipe == 2 || Tipe == 1) { pos.y = y; }
						}
					}
				}
				else if (Mode == 3) {
					if (PosTipe == 0) {
						if (!(Tipe == 0)) {
							if (Tipe == 2 || Tipe == 3) { pos.x = (std::min)((std::max)(x, minSx), Locus[Mode].F.x); }
							if (Tipe == 2 || Tipe == 3) { pos.y = y; }
						}
					}
					if (PosTipe == 3) {
						if (!(Tipe == 0)) {
							if (Tipe == 2 || Tipe == 1) { pos.x = (std::min)((std::max)(x, Locus[Mode].S.x), maxFx); }
							if (Tipe == 2 || Tipe == 1) { pos.y = y; }
						}
					}
				}
				Return = true;
				UpdateLocus = true;
			}
			};

		UpdatePos(Locus[Mode].S, LocusUI[Mode].S, 0, ((Cursor->x - OllCenter.x) / Size + 1.0f) / 2.0f, ((Cursor->y - OllCenter.y) / Size - 1.0f) / -2.0f);
		UpdatePos(Locus[Mode].H1, LocusUI[Mode].H1, 1, ((Cursor->x - Center.x) / SizeX + 1.0f) / 2.0f, ((Cursor->y - Center.y) / SizeY - 1.0f) / -2.0f);
		UpdatePos(Locus[Mode].H2, LocusUI[Mode].H2, 2, ((Cursor->x - Center.x) / SizeX + 1.0f) / 2.0f, ((Cursor->y - Center.y) / SizeY - 1.0f) / -2.0f);
		UpdatePos(Locus[Mode].F, LocusUI[Mode].F, 3, ((Cursor->x - OllCenter.x) / Size + 1.0f) / 2.0f, ((Cursor->y - OllCenter.y) / Size - 1.0f) / -2.0f);

		Locus[0].S = Locus[Mode].S;
		Locus[1].S = Locus[Mode].S;
		Locus[2].S = Locus[Mode].S;
		Locus[3].S = Locus[Mode].S;

		LocusUI[0].S = LocusUI[Mode].S;
		LocusUI[1].S = LocusUI[Mode].S;
		LocusUI[2].S = LocusUI[Mode].S;
		LocusUI[3].S = LocusUI[Mode].S;

		Locus[0].F = Locus[Mode].F;
		Locus[1].F = Locus[Mode].F;
		Locus[2].F = Locus[Mode].F;
		Locus[3].F = Locus[Mode].F;

		LocusUI[0].F = LocusUI[Mode].F;
		LocusUI[1].F = LocusUI[Mode].F;
		LocusUI[2].F = LocusUI[Mode].F;
		LocusUI[3].F = LocusUI[Mode].F;

		Center = D2D1::Point2(((Locus[Mode].S.x + Locus[Mode].F.x) - 1.0f) * Size + OllCenter.x, (-(Locus[Mode].S.y + Locus[Mode].F.y) + 1.0f) * Size + OllCenter.y);
		SizeX = (Locus[Mode].F.x - Locus[Mode].S.x) * Size;
		SizeY = (Locus[Mode].F.y - Locus[Mode].S.y) * Size;
	}

	return Return;
}

void EDIT_LOCUSES::Draw(D2D1_RECT_F Rect, D2D1_POINT_2F OllCenter, float Size, const EDIT_LOCUSES* prevLocus, const EDIT_LOCUSES* nextLocus) const {
	Center = D2D1::Point2(((Locus[Mode].S.x + Locus[Mode].F.x) - 1.0f) * Size + OllCenter.x, (-(Locus[Mode].S.y + Locus[Mode].F.y) + 1.0f) * Size + OllCenter.y);
	SizeXG = (Locus[Mode].F.x - Locus[Mode].S.x) * Size;
	SizeYG = (Locus[Mode].F.y - Locus[Mode].S.y) * Size;
	SizeX = (Locus[Mode].F.x - Locus[Mode].S.x) * Size * (float)(1 - (int)Locus[Mode].Turn * 2);
	SizeY = (Locus[Mode].F.y - Locus[Mode].S.y) * Size * (float)(1 - (int)Locus[Mode].Turn * 2);

	D2D1_POINT_2F SPoint = D2D1::Point2(-1.0f * SizeX + Center.x, 1.0f * SizeY + Center.y);
	D2D1_POINT_2F H1Point = D2D1::Point2((Locus[Mode].H1.x * 2.0f - 1.0f) * SizeX + Center.x, (Locus[Mode].H1.y * -2.0f + 1.0f) * SizeY + Center.y);
	D2D1_POINT_2F H2Point = D2D1::Point2((Locus[Mode].H2.x * 2.0f - 1.0f) * SizeX + Center.x, (Locus[Mode].H2.y * -2.0f + 1.0f) * SizeY + Center.y);
	D2D1_POINT_2F FPoint = D2D1::Point2(1.0f * SizeX + Center.x, -1.0f * SizeY + Center.y);

	if ( Mode == 1 ||  Mode == 2) {
		float Rwidth = SizeX / 1.6f;
		H1Point = D2D1::Point2(SPoint.x + cos(Locus[Mode].H1.y * 100 / 180 * PI) * (SizeX / 1.6f), SPoint.y + -sin(Locus[Mode].H1.y * 100 / 180 * PI) * (SizeY / 1.6f));
	}
		UpdateGeometry(Size, OllCenter);
		UpdateLocus = false;
	if (Selected && Tipe != 0) {
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Anchor"), 0.05f));
		g_pRenderTarget->FillRectangle(D2D1::RectF(SPoint.x, SPoint.y, FPoint.x, FPoint.y), g_pBrush);
	}

	g_pBrush->SetColor(D2D1::ColorF((Selected && Tipe != 0) ? 0x79abed : 0x54d1ff));
	g_pRenderTarget->DrawGeometry(Geometry.Get(), g_pBrush, 3.0f);

	if ( Mode == 0) {
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Anchor"), 0.6f));
		g_pRenderTarget->DrawLine(SPoint, H1Point, g_pBrush, 2.0f);
		g_pRenderTarget->DrawLine(FPoint, H2Point, g_pBrush, 2.0f);

		D2D1_RECT_F TempRect = D2D1::RectF(SPoint.x - LocusUI[Mode].S.w / 2.0f, SPoint.y - LocusUI[Mode].S.h / 2.0f, SPoint.x + LocusUI[Mode].S.w / 2.0f, SPoint.y + LocusUI[Mode].S.h / 2.0f);
		D2D1_ROUNDED_RECT SRect = D2D1::RoundedRect(TempRect, LocusUI[Mode].S.w / 4.0f / 1.618f, LocusUI[Mode].S.w / 4.0f / 1.618f);
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Anchor")));
		g_pRenderTarget->FillRoundedRectangle(SRect, g_pBrush);

		D2D1_ELLIPSE H1Ellipse = D2D1::Ellipse(H1Point, LocusUI[Mode].H1.w / 2.0f, LocusUI[Mode].H1.h / 2.0f);
		g_pRenderTarget->FillEllipse(H1Ellipse, g_pBrush);

		D2D1_ELLIPSE H2Ellipse = D2D1::Ellipse(H2Point, LocusUI[Mode].H2.w / 2.0f, LocusUI[Mode].H2.h / 2.0f);
		g_pRenderTarget->FillEllipse(H2Ellipse, g_pBrush);

		TempRect = D2D1::RectF(FPoint.x - LocusUI[Mode].F.w / 2.0f, FPoint.y - LocusUI[Mode].F.h / 2.0f, FPoint.x + LocusUI[Mode].F.w / 2.0f, FPoint.y + LocusUI[Mode].F.h / 2.0f);
		D2D1_ROUNDED_RECT FRect = D2D1::RoundedRect(TempRect, LocusUI[Mode].F.w / 4.0f / 1.618f, LocusUI[Mode].F.w / 4.0f / 1.618f);
		g_pRenderTarget->FillRoundedRectangle(FRect, g_pBrush);
	}
	else if ( Mode == 1) {
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Anchor"), 0.6f));
		g_pRenderTarget->DrawLine(SPoint, H1Point, g_pBrush, 2.0f);
		g_pRenderTarget->DrawLine(D2D1::Point2(SPoint.x, H2Point.y), D2D1::Point2(FPoint.x, H2Point.y), g_pBrush, 2.0f);

		D2D1_RECT_F TempRect = D2D1::RectF(SPoint.x - LocusUI[Mode].S.w / 2.0f, SPoint.y - LocusUI[Mode].S.h / 2.0f, SPoint.x + LocusUI[Mode].S.w / 2.0f, SPoint.y + LocusUI[Mode].S.h / 2.0f);
		D2D1_ROUNDED_RECT SRect = D2D1::RoundedRect(TempRect, LocusUI[Mode].S.w / 4.0f / 1.618f, LocusUI[Mode].S.w / 4.0f / 1.618f);
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Anchor")));
		g_pRenderTarget->FillRoundedRectangle(SRect, g_pBrush);

		TempRect = D2D1::RectF(H1Point.x - LocusUI[Mode].H1.w, H1Point.y - LocusUI[Mode].H1.h, H1Point.x + LocusUI[Mode].H1.w, H1Point.y + LocusUI[Mode].H1.h);
		float mukitemp;
		if (Locus[Mode].Turn) {
			if (SizeY < 0.0f) {
				mukitemp = -1.0;
			}
			else {
				mukitemp = 1.0;
			}
		}
		else {
			if (SizeY < 0.0f) {
				mukitemp = -1.0;
			}
			else {
				mukitemp = 1.0;
			}
		}
		Draw_Svg(311, TempRect, 0.8f, atan2(cos(Locus[Mode].H1.y * 100.0f / 180.0f * PI) * SizeX, sin(mukitemp * Locus[Mode].H1.y * 100.0f / 180.0f * PI) * SizeY) / PI * 180.0f * (SizeY / abs(SizeY)) - 90.0f * mukitemp, D2D1::ColorF(config->get_color_code(config, "Anchor")));

		D2D1_ELLIPSE H2Ellipse = D2D1::Ellipse(H2Point, LocusUI[Mode].H2.w / 2.0f, LocusUI[Mode].H2.h / 2.0f);
		g_pRenderTarget->FillEllipse(H2Ellipse, g_pBrush);

		TempRect = D2D1::RectF(FPoint.x - LocusUI[Mode].F.w / 2.0f, FPoint.y - LocusUI[Mode].F.h / 2.0f, FPoint.x + LocusUI[Mode].F.w / 2.0f, FPoint.y + LocusUI[Mode].F.h / 2.0f);
		D2D1_ROUNDED_RECT FRect = D2D1::RoundedRect(TempRect, LocusUI[Mode].F.w / 4.0f / 1.618f, LocusUI[Mode].F.w / 4.0f / 1.618f);
		g_pRenderTarget->FillRoundedRectangle(FRect, g_pBrush);
	}
	else if ( Mode == 2) {
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Anchor"), 0.6f));
		g_pRenderTarget->DrawLine(SPoint, H1Point, g_pBrush, 2.0f);

		D2D1_RECT_F TempRect = D2D1::RectF(SPoint.x - LocusUI[Mode].S.w / 2.0f, SPoint.y - LocusUI[Mode].S.h / 2.0f, SPoint.x + LocusUI[Mode].S.w / 2.0f, SPoint.y + LocusUI[Mode].S.h / 2.0f);
		D2D1_ROUNDED_RECT SRect = D2D1::RoundedRect(TempRect, LocusUI[Mode].S.w / 4.0f / 1.618f, LocusUI[Mode].S.w / 4.0f / 1.618f);
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Anchor")));
		g_pRenderTarget->FillRoundedRectangle(SRect, g_pBrush);
		TempRect = D2D1::RectF(H1Point.x - LocusUI[Mode].H1.w, H1Point.y - LocusUI[Mode].H1.h, H1Point.x + LocusUI[Mode].H1.w, H1Point.y + LocusUI[Mode].H1.h);
		float mukitemp;
		if (Locus[Mode].Turn) {
			if (SizeY < 0.0f) {
				mukitemp = -1.0;
			}
			else {
				mukitemp = 1.0;
			}
		}
		else {
			if (SizeY < 0.0f) {
				mukitemp = -1.0;
			}
			else {
				mukitemp = 1.0;
			}
		}
		Draw_Svg(311, TempRect, 0.8f, atan2(cos(Locus[Mode].H1.y * 100.0f / 180.0f * PI) * SizeX, sin(mukitemp * Locus[Mode].H1.y * 100.0f / 180.0f * PI) * SizeY) / PI * 180.0f * (SizeY / abs(SizeY)) - 90.0f * mukitemp, D2D1::ColorF(config->get_color_code(config, "Anchor")));

		D2D1_ELLIPSE H2Ellipse = D2D1::Ellipse(H2Point, LocusUI[Mode].H2.w / 2.0f, LocusUI[Mode].H2.h / 2.0f);
		g_pRenderTarget->FillEllipse(H2Ellipse, g_pBrush);

		TempRect = D2D1::RectF(FPoint.x - LocusUI[Mode].F.w / 2.0f, FPoint.y - LocusUI[Mode].F.h / 2.0f, FPoint.x + LocusUI[Mode].F.w / 2.0f, FPoint.y + LocusUI[Mode].F.h / 2.0f);
		D2D1_ROUNDED_RECT FRect = D2D1::RoundedRect(TempRect, LocusUI[Mode].F.w / 4.0f / 1.618f, LocusUI[Mode].F.w / 4.0f / 1.618f);
		g_pRenderTarget->FillRoundedRectangle(FRect, g_pBrush);
	}
	else if (Mode == 3) {
		D2D1_RECT_F TempRect = D2D1::RectF(SPoint.x - LocusUI[Mode].S.w / 2.0f, SPoint.y - LocusUI[Mode].S.h / 2.0f, SPoint.x + LocusUI[Mode].S.w / 2.0f, SPoint.y + LocusUI[Mode].S.h / 2.0f);
		D2D1_ROUNDED_RECT SRect = D2D1::RoundedRect(TempRect, LocusUI[Mode].S.w / 4.0f / 1.618f, LocusUI[Mode].S.w / 4.0f / 1.618f);
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Anchor")));
		g_pRenderTarget->FillRoundedRectangle(SRect, g_pBrush);

		TempRect = D2D1::RectF(FPoint.x - LocusUI[Mode].F.w / 2.0f, FPoint.y - LocusUI[Mode].F.h / 2.0f, FPoint.x + LocusUI[Mode].F.w / 2.0f, FPoint.y + LocusUI[Mode].F.h / 2.0f);
		D2D1_ROUNDED_RECT FRect = D2D1::RoundedRect(TempRect, LocusUI[Mode].F.w / 4.0f / 1.618f, LocusUI[Mode].F.w / 4.0f / 1.618f);
		g_pRenderTarget->FillRoundedRectangle(FRect, g_pBrush);
	}
}

void EDITOR::GridDraw(D2D1_RECT_F Rect, float Size, D2D1_POINT_2F Center, int Grid) {
	if (Size <= 0.0f || Grid <= 0) return;
	float width = Size / (float)Grid;

	D2D1_POINT_2F origin = D2D1::Point2(Center.x - Size / 2.0f, Center.y - Size / 2.0f);
	float startIdxX = (Rect.left - origin.x) / width;
	float endIdxX = (Rect.right - origin.x) / width;
	int minIx = static_cast<int>(std::floor(startIdxX));
	int maxIx = static_cast<int>(std::ceil(endIdxX));

	for (int ix = minIx; ix <= maxIx; ++ix) {
		float gridx = origin.x + ix * width;
		bool isMainLine = (Grid == 1) || (std::abs(ix) % Grid == 0);
		float strokeWidth = isMainLine ? 1.5f : 0.75f;
		float alpha = isMainLine ? 0.2f : 0.1f;
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Text"), alpha));
		g_pRenderTarget->DrawLine(D2D1::Point2(gridx, Rect.top), D2D1::Point2(gridx, Rect.bottom), g_pBrush, strokeWidth);
	}

	float startIdxY = (Rect.top - origin.y) / width;
	float endIdxY = (Rect.bottom - origin.y) / width;

	int minIy = static_cast<int>(std::floor(startIdxY));
	int maxIy = static_cast<int>(std::ceil(endIdxY));

	for (int iy = minIy; iy <= maxIy; ++iy) {
		float gridy = origin.y + iy * width;

		bool isMainLine = (Grid == 1) || (std::abs(iy) % Grid == 0);
		float strokeWidth = isMainLine ? 1.5f : 0.75f;
		float alpha = isMainLine ? 0.2f : 0.1f;

		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Text"), alpha));
		g_pRenderTarget->DrawLine(D2D1::Point2(Rect.left, gridy), D2D1::Point2(Rect.right, gridy), g_pBrush, strokeWidth);
	}

	g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Background"), 0.5f));
	g_pRenderTarget->FillRectangle(D2D1::RectF(Rect.left, Rect.top, Center.x - Size / 2, Rect.bottom), g_pBrush);
	g_pRenderTarget->FillRectangle(D2D1::RectF(Center.x + Size / 2, Rect.top, Rect.right, Rect.bottom), g_pBrush);
	g_pBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.3f));
	g_pRenderTarget->FillRectangle(D2D1::RectF(Rect.left, Rect.top, Center.x - Size / 2, Rect.bottom), g_pBrush);
	g_pRenderTarget->FillRectangle(D2D1::RectF(Center.x + Size / 2, Rect.top, Rect.right, Rect.bottom), g_pBrush);
}

bool EDITOR::Update(CUR* Cursor, bool LocusUpdate) {
	bool Return = LocusUpdate;
	if (Cursor->click && Cursor->RangeCheck(L"LocusEditor")) {
		Cursor->drag = true;
	}
	if (!Cursor->clicking) {
		Cursor->drag = false;
	}

	if (Cursor->rclick && Cursor->RangeCheck(L"LocusEditor")) {
		Cursor->rclick = false;
		HMENU hMenu = CreatePopupMenu();
		if (hMenu) {
			AppendMenuW(hMenu, MF_STRING, 101, L"反転");
			AppendMenuW(hMenu, MF_STRING, 102, L"全て反転");
			AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
			AppendMenuW(hMenu, MF_STRING, 103, L"コピー");
			if(UseCopy){
				AppendMenuW(hMenu, MF_STRING, 104, L"貼り付け");
			}
			else {
				AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 104, L"貼り付け");
			}
			AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
			AppendMenuW(hMenu, MF_STRING, 105, L"イージングを選択");
			AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
			if (Locus.size() == 1) {
				AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 106, L"削除");
			}
			else {
				AppendMenuW(hMenu, MF_STRING, 106, L"削除");
			}

			POINT pt = { (LONG)Cursor->x, (LONG)Cursor->y };
			ClientToScreen(Cursor->hwnd, &pt);

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
				Locus[SelectLocus].Locus[Locus[SelectLocus].Mode].Turn = !Locus[SelectLocus].Locus[Locus[SelectLocus].Mode].Turn;
				Return = true;
				break;
			case 102:
			{
				std::reverse(Locus.begin(), Locus.end());
				SelectLocus = (int)Locus.size() - 1 - SelectLocus;
				for (int i = 0; i < Locus.size(); i++) {
					POS NewS = { 1.0f - Locus[i].Locus[Locus[i].Mode].F.x, 1.0f - Locus[i].Locus[Locus[i].Mode].F.y };
					POS NewF = { 1.0f - Locus[i].Locus[Locus[i].Mode].S.x, 1.0f - Locus[i].Locus[Locus[i].Mode].S.y };
					Locus[i].Locus[Locus[i].Mode].S = NewS;
					Locus[i].Locus[Locus[i].Mode].F = NewF;
					if (Locus[i].Tipe == 1) {
						Locus[i].Tipe = 3;
					}
					else if (Locus[i].Tipe == 3) {
						Locus[i].Tipe = 1;
					}
					Locus[i].Locus[Locus[i].Mode].Turn = !Locus[i].Locus[Locus[i].Mode].Turn;
				}
				Return = true;
				break;
			}
			case 103:
			{
				UseCopy = true;
				CopyLocus = Locus[SelectLocus].Locus[Locus[SelectLocus].Mode];
				break;
			}
			case 104:
			{
				if (UseCopy) {
					Locus[SelectLocus].Locus[CopyLocus.Mode].H1 = CopyLocus.H1;
					Locus[SelectLocus].Locus[CopyLocus.Mode].H2 = CopyLocus.H2;
					Locus[SelectLocus].Locus[CopyLocus.Mode].Turn = CopyLocus.Turn;
					Locus[SelectLocus].Mode = CopyLocus.Mode;
					Locus[SelectLocus].UseMode3 = (CopyLocus.Mode == 3) ? true : false;
					Return = true;
				}
				break;
			}
			case 105:
			{
				int inputNum = 0;
				if (EasingWindow(Cursor->hwnd, L"イージング番号入力", inputNum, *Cursor)) {
					Locus[SelectLocus].Mode = 3;
					Locus[SelectLocus].Locus[3].Turn = false;
					Locus[SelectLocus].Locus[Locus[SelectLocus].Mode].H1.y = static_cast<float>(inputNum);
					Locus[SelectLocus].UseMode3 = true;
					Return = true;
				}
				break;
			}
			case 106:
			{
				if (Locus.size() != 1) {
					if (SelectLocus == Locus.size() - 1) {
						Locus[SelectLocus - 1].Locus[0].F = Locus[SelectLocus].Locus[0].F;
						Locus[SelectLocus - 1].Locus[1].F = Locus[SelectLocus].Locus[1].F;
						Locus[SelectLocus - 1].Locus[2].F = Locus[SelectLocus].Locus[2].F;
						Locus[SelectLocus - 1].Locus[3].F = Locus[SelectLocus].Locus[3].F;
						if (Locus.size() == 2) {
							Locus[SelectLocus - 1].Tipe = 0;
						}
						else {
							Locus[SelectLocus - 1].Tipe = 3;
						}
						Locus.erase(Locus.begin() + SelectLocus);
						SelectLocus -= 1;
						Locus[SelectLocus].Selected = true;
					}
					else {
						Locus[SelectLocus + 1].Locus[0].S = Locus[SelectLocus].Locus[0].S;
						Locus[SelectLocus + 1].Locus[1].S = Locus[SelectLocus].Locus[1].S;
						Locus[SelectLocus + 1].Locus[2].S = Locus[SelectLocus].Locus[2].S;
						Locus[SelectLocus + 1].Locus[3].S = Locus[SelectLocus].Locus[3].S;
						if (SelectLocus == 0) {
							Locus[SelectLocus + 1].Tipe = 1;
						}
						if (Locus.size() == 2) {
							Locus[SelectLocus + 1].Tipe = 0;
						}
						Locus.erase(Locus.begin() + SelectLocus);
						Locus[SelectLocus].Selected = true;
					}
					Return = true;
				}
				else {
					MessageBeep(MB_ICONINFORMATION);
				}
				break;
			}
			}
			Cursor->x = 0.0f;
			Cursor->y = 0.0f;
		}
	}

	bool UpdateSize = LocusUpdate;
	D2D1_RECT_F Rect = Cursor->range.Range[L"Locus"];
	D2D1_RECT_F Lrect = D2D1::RectF(Rect.left + 30.0f, Rect.top + 30.0f + 40.0f, Rect.right - 30.0f, Rect.bottom - 30.0f);

	float Size = (std::max)((std::min)(std::abs(Lrect.right - Lrect.left), std::abs(Lrect.bottom - Lrect.top)) / 2.0f, 0.01f);
	Size = (std::max)(Size, 10.0f);
	if (Cursor->wheel > 0.0f && Cursor->RangeCheck(L"Locus")) {
		if (sizeF < 10.0f) {
			sizeF *= 1.1f * Cursor->wheel;
		}
	}
	else if (Cursor->wheel < 0.0f && Cursor->RangeCheck(L"Locus")) {
		if (sizeF > 0.05f) {
			sizeF /= 1.1f * -Cursor->wheel;
		}
	}
	size += (sizeF - size) / 3.0f;
	if (abs(size - sizeF) < 0.001) {
		size = sizeF;
	}
	else {
		UpdateSize = true;
	}
	Size *= size;

	if (Cursor->mclick && Cursor->RangeCheck(L"LocusEditor")) {
		Mclicking = true;
	}
	if (!Cursor->mclicking) {
		Mclicking = false;
	}
	if (Mclicking) {
		if (Size > 0.0f) {
			xF = (std::min)((std::max)(xF + Cursor->move.x / Size, -1.0f), 1.0f);
			yF += Cursor->move.y / Size;
		}
	}
	x += (xF - x) / 2.0f;
	if (abs(x - xF) < 0.001) {
		x = xF;
	}
	else {
		UpdateSize = true;
	}
	y += (yF - y) / 2.0f;
	if (abs(y - yF) < 0.001) {
		y = yF;
	}
	else {
		UpdateSize = true;
	}
	D2D1_POINT_2F Center = D2D1::Point2(
		(Lrect.left + Lrect.right) / 2.0f + (x * Size),
		(Lrect.top + Lrect.bottom) / 2.0f + (y * Size)
	);
	if (Cursor->RangeCheck(L"LocusEditor")) {
		int splitIndex = -1;
		EDIT_LOCUSES newLocus1, newLocus2;

		for (size_t i = 0; i < Locus.size(); ++i) {
			float splitX = ((Cursor->x - Center.x) / Size + 1.0f) / 2.0f;
			float splitY = ((Cursor->y - Center.y) / Size - 1.0f) / -2.0f;

			if (splitX > Locus[i].Locus[Locus[i].Mode].S.x && splitX < Locus[i].Locus[Locus[i].Mode].F.x) {
				if (Cursor->dclick) {
					splitIndex = static_cast<int>(i);

					newLocus1 = Locus[i];
					newLocus2 = Locus[i];

					newLocus1.Selected = false;

					newLocus1.Locus[Locus[i].Mode].F.x = splitX;
					newLocus1.Locus[Locus[i].Mode].F.y = splitY;

					newLocus2.Locus[Locus[i].Mode].S.x = splitX;
					newLocus2.Locus[Locus[i].Mode].S.y = splitY;

					newLocus1.Locus[Locus[i].Mode].H2.x = (std::min)(newLocus1.Locus[Locus[i].Mode].H2.x, 1.0f);
					newLocus2.Locus[Locus[i].Mode].H1.x = (std::max)(newLocus2.Locus[Locus[i].Mode].H1.x, 0.0f);
					newLocus1.UpdateLocus = true;
					newLocus2.UpdateLocus = true;

					if (Locus[i].Tipe == 0) {
						newLocus1.Tipe = 1;
						newLocus2.Tipe = 3;
					}
					else if (Locus[i].Tipe == 1) {
						newLocus1.Tipe = 1;
						newLocus2.Tipe = 2;
					}
					else if (Locus[i].Tipe == 2) {
						newLocus1.Tipe = 2;
						newLocus2.Tipe = 2;
					}
					else if (Locus[i].Tipe == 3) {
						newLocus1.Tipe = 2;
						newLocus2.Tipe = 3;
					}
					Return = true;
					break;
				}
				if (Cursor->click || Cursor->rclick) {
					if (ClickingLocus == -1) {
						Locus[SelectLocus].Selected = false;
						SelectLocus = (int)i;
						Locus[i].Selected = true;
						Return = true;
					}
					ClickingLocus = (int)i;
				}
			}
		}


		if (!Cursor->clicking) {
			ClickingLocus = -1;
		}

		if (splitIndex != -1) {
			Locus[splitIndex] = newLocus1;
			Locus.insert(Locus.begin() + splitIndex + 1, newLocus2);
			SelectLocus = splitIndex + 1;
		}
	}

	for (size_t i = 0; i < Locus.size(); ++i) {
		const EDIT_LOCUSES* prevLocus = (i > 0) ? &Locus[i - 1] : nullptr;
		const EDIT_LOCUSES* nextLocus = (i + 1 < Locus.size()) ? &Locus[i + 1] : nullptr;
		if (UpdateSize) {
			Locus[i].UpdateLocus = true;
		}
		if (Locus[i].Update(Lrect, Cursor, Center, Size, prevLocus, nextLocus)) {
			Return = true;
		}
	}

	for (size_t i = 1; i < Locus.size(); ++i) {
		if (Locus[i].LocusUI[Locus[i].Mode].S.Clicking) {
			Locus[i - 1].Locus[Locus[i].Mode].F.x = Locus[i].Locus[Locus[i].Mode].S.x;
			Locus[i - 1].Locus[Locus[i].Mode].F.y = Locus[i].Locus[Locus[i].Mode].S.y;
		}
		else if (Locus[i - 1].LocusUI[Locus[i].Mode].F.Clicking) {
			Locus[i].Locus[Locus[i].Mode].S.x = Locus[i - 1].Locus[Locus[i].Mode].F.x;
			Locus[i].Locus[Locus[i].Mode].S.y = Locus[i - 1].Locus[Locus[i].Mode].F.y;
		}
	}

	D2D1_RECT_F ModeRect = D2D1::RectF(Rect.left + 10.0f, Rect.top + 6.0f, Rect.right - 10.0f - 44.0f, Rect.top + 40.0f);
	ModeMenuHover = Cursor->RectCheck(ModeRect);
	if (Cursor->click && ModeMenuHover) {
		ModeMenuClicking = true;
	}
	if (!Cursor->clicking && ModeMenuClicking) {
		ModeMenuClicking = false;
		if (Locus[SelectLocus].Mode != 3) {
			Locus[SelectLocus].UseMode3 = false;
			Return = true;
		}
	}
	if (ModeMenuClicking) {
		int ModeTemp = (int)floor((Cursor->x - ModeRect.left) / ((ModeRect.right - ModeRect.left) / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)));
		Locus[SelectLocus]. Mode = (std::min)((std::max)(ModeTemp, 0), (Locus[SelectLocus].UseMode3 ? 3 : 2));
		Locus[SelectLocus].UpdateLocus = true;
	}

	ModeMenuAnime += ((float)Locus[SelectLocus]. Mode - ModeMenuAnime) / 2.3f;
	if (abs(ModeMenuAnime - (float)Locus[SelectLocus].Mode) < 0.001) {
		ModeMenuAnime = (float)Locus[SelectLocus].Mode;
	}

	D2D1_RECT_F TurnRect = D2D1::RectF(Rect.right - 10.0f - 34.0f, Rect.top + 6.0f, Rect.right - 10.0f, Rect.top + 40.0f);
	TurnHover = Cursor->RectCheck(TurnRect);
	if (TurnHover) {
		if (Cursor->click) {
			Locus[SelectLocus].Locus[Locus[SelectLocus].Mode].Turn = (bool)(((int)Locus[SelectLocus].Locus[Locus[SelectLocus].Mode].Turn + 1) % 2);
			Locus[SelectLocus].UpdateLocus = true;
			Return = true;
		}
		TurnHoverAnime += (1.0f - TurnHoverAnime) / 1.8f;
		if (abs(TurnHoverAnime - 1.0f) < 0.001) {
			TurnHoverAnime = 1.0f;
		}
	}
	else {
		TurnHoverAnime += (0.0f - TurnHoverAnime) / 2.3f;
		if (abs(TurnHoverAnime - 0.0f) < 0.001) {
			TurnHoverAnime = 0.0f;
		}
	}

	D2D1_RECT_F ReturnSizeRect = D2D1::RectF(Rect.right - 10.0f - 34.0f, Rect.bottom - 40.0f, Rect.right - 10.0f, Rect.bottom - 6.0f);
	if (sizeF != 1.0f || xF != 0.0f || yF != 0.0f) {
		ReturnSizeHide = false;
	}
	else {
		ReturnSizeHide = true;
	}
	ReturnSizeHover = Cursor->RectCheck(ReturnSizeRect);
	if (ReturnSizeHover) {
		if (Cursor->click && !ReturnSizeHide) {
			sizeF = 1.0f;
			xF = 0.0f;
			yF = 0.0f;
			ReturnSizeHide = true;
			Cursor->drag = false;
		}
		ReturnSizeHoverAnime += (1.0f - ReturnSizeHoverAnime) / 1.8f;
		if (abs(ReturnSizeHoverAnime - 1.0f) < 0.001) {
			ReturnSizeHoverAnime = 1.0f;
		}
	}
	else {
		ReturnSizeHoverAnime += (0.0f - ReturnSizeHoverAnime) / 2.3f;
		if (abs(ReturnSizeHoverAnime - 0.0f) < 0.001) {
			ReturnSizeHoverAnime = 0.0f;
		}
	}
	if (ReturnSizeHide) {
		ReturnSizeHideAnime += (0.0f - ReturnSizeHideAnime) / 2.5f;
		if (abs(ReturnSizeHideAnime - 0.0f) < 0.001) {
			ReturnSizeHideAnime = 0.0f;
		}
	}
	else {
		ReturnSizeHideAnime += (1.0f - ReturnSizeHideAnime) / 2.5f;
		if (abs(ReturnSizeHideAnime - 1.0f) < 0.01) {
			ReturnSizeHideAnime = 1.0f;
		}
	}
	if ((x == xF) && (y == yF) && (size == sizeF) && ((ModeMenuAnime == 0.0f) || (ModeMenuAnime == 1.0f) || (ModeMenuAnime == 2.0f)) && ((TurnHoverAnime == 0.0f) || (TurnHoverAnime == 1.0f)) && ((ReturnSizeHoverAnime == 0.0f) || (ReturnSizeHoverAnime == 1.0f)) && ((ReturnSizeHideAnime == 0.0f) || (ReturnSizeHideAnime == 1.0f))) {
		AnimeMoving = false;
	}
	else {
		AnimeMoving = true;
		Return = true;
	}
	return Return;
}

void EDITOR::Draw(CUR *Cursor) {
	D2D1_RECT_F Rect = Cursor->range.Range[L"Locus"];
	D2D1_RECT_F Lrect = D2D1::RectF(Rect.left + 30.0f, Rect.top + 30.0f + 40.0f, Rect.right - 30.0f, Rect.bottom - 30.0f);

	float Size = (std::min)(std::abs(Lrect.right - Lrect.left), std::abs(Lrect.bottom - Lrect.top)) / 2.0f;
	Size = (std::max)(Size, 10.0f);
	Size *= size;
	D2D1_POINT_2F Center = D2D1::Point2(
		(Lrect.left + Lrect.right) / 2.0f + (x * Size),
		(Lrect.top + Lrect.bottom) / 2.0f + (y * Size)
	);

	GridDraw(Rect, Size * 2.0f, Center, 5);

	for (size_t i = 0; i < Locus.size(); ++i) {
		const EDIT_LOCUSES* prevLocus = (i > 0) ? &Locus[i - 1] : nullptr;
		const EDIT_LOCUSES* nextLocus = (i + 1 < Locus.size()) ? &Locus[i + 1] : nullptr;
		Locus[i].Draw(Lrect, Center, Size, prevLocus, nextLocus);
	}

	D2D1_RECT_F ModeRect = D2D1::RectF(Rect.left + 10.0f, Rect.top + 6.0f, Rect.right - 10.0f - 44.0f, Rect.top + 40.0f);
	g_pBrush->SetColor(D2D1::ColorF(0.3f, 0.3f, 0.3f, 0.5f));
	g_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(ModeRect, 5.0f, 5.0f), g_pBrush);
	g_pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.5f));
	g_pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(ModeRect, 5.0f, 5.0f), g_pBrush, 0.25f);
	g_pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.2f));
	D2D1_RECT_F TempRect = D2D1::RectF(
		ModeRect.left + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) - 0.5f,
		ModeRect.top + (ModeRect.bottom - ModeRect.top) * (1.0f / 10.0f) + 1.0f,
		ModeRect.left + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) + 0.5f,
		ModeRect.bottom - (ModeRect.bottom - ModeRect.top) * (1.0f / 10.0f) - 1.0f
	);
	g_pRenderTarget->FillRectangle(TempRect, g_pBrush);
	TempRect = D2D1::RectF(
		ModeRect.left + (ModeRect.right - ModeRect.left) * (2.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) - 0.5f,
		ModeRect.top + (ModeRect.bottom - ModeRect.top) * (1.0f / 10.0f) + 1.0f,
		ModeRect.left + (ModeRect.right - ModeRect.left) * (2.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) + 0.5f,
		ModeRect.bottom - (ModeRect.bottom - ModeRect.top) * (1.0f / 10.0f) - 1.0f
	);
	g_pRenderTarget->FillRectangle(TempRect, g_pBrush);
	if (Locus[SelectLocus].UseMode3) {
		TempRect = D2D1::RectF(
			ModeRect.left + (ModeRect.right - ModeRect.left) * (3.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) - 0.5f,
			ModeRect.top + (ModeRect.bottom - ModeRect.top) * (1.0f / 10.0f) + 1.0f,
			ModeRect.left + (ModeRect.right - ModeRect.left) * (3.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) + 0.5f,
			ModeRect.bottom - (ModeRect.bottom - ModeRect.top) * (1.0f / 10.0f) - 1.0f
		);
		g_pRenderTarget->FillRectangle(TempRect, g_pBrush);
	}
	g_pBrush->SetColor(D2D1::ColorF(0xffffff, 0.3f));
	TempRect = D2D1::RectF(ModeRect.left + 3.5f, ModeRect.top + 2.5f, ModeRect.left + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) - 3.5f, ModeRect.bottom - 2.5f);
	TempRect = D2D1::RectF(TempRect.left + (ModeRect.right - ModeRect.left) * ((1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) * ModeMenuAnime), TempRect.top, TempRect.right + (ModeRect.right - ModeRect.left) * ((1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) * ModeMenuAnime), TempRect.bottom);
	g_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(TempRect, 5.0f, 5.0f), g_pBrush);
	g_pBrush->SetColor(D2D1::ColorF(0xffffff, 0.75f));
	g_pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(TempRect, 5.0f, 5.0f), g_pBrush, 0.5f);
	TempRect = D2D1::RectF(ModeRect.left + 3.5f, ModeRect.top + 2.5f, ModeRect.left + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)) - 3.5f, ModeRect.bottom - 2.5f);
	Draw_Svg(301, TempRect, 0.618f, 0.0f, D2D1::ColorF(config->get_color_code(config, "Text")));
	TempRect = D2D1::RectF(TempRect.left + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)), TempRect.top, TempRect.right + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)), TempRect.bottom);
	Draw_Svg(302, TempRect, 0.618f, 0.0f, D2D1::ColorF(config->get_color_code(config, "Text")));
	TempRect = D2D1::RectF(TempRect.left + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)), TempRect.top, TempRect.right + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)), TempRect.bottom);
	Draw_Svg(303, TempRect, 0.618f, 0.0f, D2D1::ColorF(config->get_color_code(config, "Text")));
	if (Locus[SelectLocus].UseMode3) {
		TempRect = D2D1::RectF(TempRect.left + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)), TempRect.top, TempRect.right + (ModeRect.right - ModeRect.left) * (1.0f / (Locus[SelectLocus].UseMode3 ? 4.0f : 3.0f)), TempRect.bottom);
		Draw_Svg(304, TempRect, 0.618f, 0.0f, D2D1::ColorF(config->get_color_code(config, "Text")));
	}

	D2D1_RECT_F TurnRect = D2D1::RectF(Rect.right - 10.0f - 34.0f, Rect.top + 6.0f, Rect.right - 10.0f, Rect.top + 40.0f);
	float TempCol = 0.3f * (1.0f - TurnHoverAnime) + 0.5f * TurnHoverAnime;
	g_pBrush->SetColor(D2D1::ColorF(TempCol, TempCol, TempCol, 0.5f));
	g_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(TurnRect, 5.0f, 5.0f), g_pBrush);
	g_pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.5f));
	g_pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(TurnRect, 5.0f, 5.0f), g_pBrush, 0.25f);
	Draw_Svg(321, TurnRect, 0.5f, 0.0f, D2D1::ColorF(config->get_color_code(config, "Text")));

	D2D1_RECT_F ReturnSizeRect = D2D1::RectF(Rect.right - 10.0f - 34.0f, Rect.bottom - 40.0f, Rect.right - 10.0f, Rect.bottom - 6.0f);

	TempCol = 0.3f * (1.0f - ReturnSizeHoverAnime) + 0.5f * ReturnSizeHoverAnime;
	g_pBrush->SetColor(D2D1::ColorF(TempCol, TempCol, TempCol, ReturnSizeHideAnime * 0.5f));
	g_pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(ReturnSizeRect, 5.0f, 5.0f), g_pBrush);
	g_pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, ReturnSizeHideAnime * 0.5f));
	g_pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(ReturnSizeRect, 5.0f, 5.0f), g_pBrush, 0.25f);
	Draw_Svg(322, ReturnSizeRect, 0.618f, 0.0f, D2D1::ColorF(config->get_color_code(config, "Text"), ReturnSizeHideAnime));
}