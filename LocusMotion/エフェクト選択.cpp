#include "Direct2D.h"
#include "GUI.h"
#include "Dialog.h"
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

#include <random>

std::random_device rd;
std::mt19937 gen(rd());

int randomInt(int min, int max)
{
	std::uniform_int_distribution<int> dist(min, max);
	return dist(gen);
}

#define PI 3.14159265359f

bool P_Effect::Locus_Update(D2D1_RECT_F Rect, CUR Cursor) const {
	bool Return = false;
	if (LocusID < 0 || LocusID >= static_cast<int>(LocusData.CLocus.size())) {
		return false;
	}
	if (SectionWidth.empty() || Value.empty()) return false;

	if (LocusHover.size() != static_cast<size_t>(SectionNum)) {
		LocusHover.resize(SectionNum, false);
	}
	if (LocusHoverAnime.size() != static_cast<size_t>(SectionNum)) {
		LocusHoverAnime.resize(SectionNum, 0.0f);
	}

	std::vector<float> width;
	float add = 1.0f;
	auto it = std::min_element(SectionWidth.begin(), SectionWidth.end());
	if (it != SectionWidth.end() && *it * (Rect.right - Rect.left) < (Rect.bottom - Rect.top)) {
		add = (Rect.bottom - Rect.top) / (*it * (Rect.right - Rect.left));
	}
	XRange = 0.0f;
	for (int i = 0; i < SectionNum; i++) {
		width.push_back(SectionWidth[i] * (Rect.right - Rect.left) * add);
		XRange += SectionWidth[i] * (Rect.right - Rect.left) * add;
	}
	XRange = (std::max)(XRange - (Rect.right - Rect.left), 0.0f);

	if (Cursor.click && Cursor.RectCheck(Rect)) {
		click = true;
	}
	if (Cursor.clicking && !Cursor.click && Cursor.move.x != 0.0f) {
		if (Cursor.RectCheck(Rect) && click) {
			drag = true;
		}
		if (drag) {
			PageXF -= Cursor.move.x;
			PageXF = (std::min)((std::max)(PageXF, 0.0f), XRange);
		}
	}
	if (Cursor.RectCheck(Rect)) {
		if (Cursor.wheel < 0 && GetAsyncKeyState(VK_SHIFT) & 0x8000) {
			PageXF += 50 * abs(Cursor.wheel);
		}
		if (Cursor.wheel > 0 && GetAsyncKeyState(VK_SHIFT) & 0x8000) {
			PageXF -= 50 * abs(Cursor.wheel);
		}
		PageXF = (std::min)((std::max)(PageXF, 0.0f), XRange);
	}
	PageX += (PageXF - PageX) / 3.0f;
	if (abs(PageX - PageXF) < 0.001) {
		PageX = PageXF;
	}

	float diff = maxValue - minValue;
	if (std::abs(diff) < 0.0001f) {
		diff = 1.0f;
	}

	float left = Rect.left - PageX;
	for (int i = 0; i < SectionNum; i++) {
		D2D1_RECT_F LocusRect = D2D1::RectF(left, Rect.top, left + width[i], Rect.bottom - 20.0f);

		LocusHover[i] = Cursor.RectCheck(LocusRect);
		float target = LocusHover[i] ? 1.0f : 0.0f;
		LocusHoverAnime[i] += (target - LocusHoverAnime[i]) / 2.0f;

		if (std::abs(LocusHoverAnime[i] - target) < 0.001f) {
			LocusHoverAnime[i] = target;
		}

		if (Cursor.drop && LocusHover[i]) {
			LocusData.CLocus[LocusID].Locuses[i].LoadLocuses(Editor.ToLocuses());
			Return = true;
		}

		if (click && !drag && !Cursor.clicking && LocusHover[i]) {
			Editor.SetLocuses(LocusData.CLocus[LocusID].Locuses[i]);
			click = false;
			drag = false;
			Return = true;
		}

		if (Cursor.rclick && LocusHover[i]) {
			HMENU hMenu = CreatePopupMenu();
			if (hMenu) {
				AppendMenuW(hMenu, MF_STRING, 104, L"エディタの曲線を適用");
				AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
				AppendMenuW(hMenu, MF_STRING, 101, L"コピー");
				AppendMenuW(hMenu, MF_STRING, 102, L"全てコピー");
				if(Effects.UseCopyLocuses || Effects.UseCopyCLocus){
					AppendMenuW(hMenu, MF_STRING, 103, L"貼り付け");
				}
				else {
					AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 103, L"貼り付け");
				}
				AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
				AppendMenuW(hMenu, MF_STRING, 105, L"モディファイア");

				POINT pt = { (LONG)Cursor.x, (LONG)Cursor.y };
				ClientToScreen(Cursor.hwnd, &pt);

				SetForegroundWindow(Cursor.hwnd);
				int cmd = TrackPopupMenu(
					hMenu,
					TPM_RIGHTBUTTON | TPM_RETURNCMD,
					pt.x, pt.y,
					0, Cursor.hwnd, NULL
				);
				DestroyMenu(hMenu);

				switch (cmd) {
				case 101: {
					Effects.CopyLocuses = LocusData.CLocus[LocusID].Locuses[i];
					Effects.UseCopyLocuses = true;
					Effects.UseCopyCLocus = false;
					break;
				}
				case 102: {
					Effects.CopyCLocus = LocusData.CLocus[LocusID];
					Effects.UseCopyLocuses = false;
					Effects.UseCopyCLocus = true;
					break;
				}
				case 103: {
					if (Effects.UseCopyLocuses) {
						LocusData.CLocus[LocusID].Locuses[i] = Effects.CopyLocuses;
					}
					if (Effects.UseCopyCLocus) {
						auto& targetLocuses = LocusData.CLocus[LocusID].Locuses;
						const auto& copyLocuses = Effects.CopyCLocus.Locuses;

						if (!copyLocuses.empty()) {
							size_t targetSize = targetLocuses.size();
							size_t copySize = copyLocuses.size();

							size_t copyCount = (std::min)(targetSize, copySize);
							for (size_t k = 0; k < copyCount; ++k) {
								targetLocuses[k] = copyLocuses[k];
							}

							if (targetSize > copySize) {
								const auto& lastCopyElement = copyLocuses.back();
								for (size_t k = copySize; k < targetSize; ++k) {
									targetLocuses[k] = lastCopyElement;
								}
							}
						}
					}
					break;
				}
				case 104: {
					LocusData.CLocus[LocusID].Locuses[i].LoadLocuses(Editor.ToLocuses());
					break;
				}
				case 105: {
					// LocusData.CLocus[LocusID].Locuses[i] の参照を直接渡す
					if (ModifierWindow(Cursor.hwnd, L"モディファイア", Cursor, LocusData.CLocus[LocusID].Locuses[i])) {
						Return = true;
					}
					break;
				}
				}
			}
		}
		if (LocusHoverAnime[i] != target) {
			Return = true;
		}
		left += width[i];
	}
	if (!Cursor.clicking) {
		click = false;
	}
	if (drag && !Cursor.clicking) {
		drag = false;
	}
	return Return;
}

void P_Effect::SetTrackbar(int LocusID) const {
	LocusHover.assign(SectionNum, false);
	LocusHoverAnime.assign(SectionNum, 0.0f);

	OBJECT_HANDLE target_obj = ObjectHandle;
	EFFECT_HANDLE target_effect = EffectHandle;

	if (edit_handle && target_obj && target_effect) {
		struct WriteParam {
			OBJECT_HANDLE obj;
			EFFECT_HANDLE effect;
			int SectionNum;
			int LocusID;
			std::wstring item_name;
			std::string value;
		};

		WriteParam* p_param = new WriteParam{
			target_obj,
			target_effect,
			SectionNum,
			LocusID,
			Name,
			StrValue.empty() ? "0.00" : StrValue
		};

		edit_handle->call_edit_section_param(p_param, [](void* param_ptr, EDIT_SECTION* edit) {
			if (!param_ptr) return;

			WriteParam* p_write = static_cast<WriteParam*>(param_ptr);

			if (edit && p_write->obj && p_write->effect) {
				LPCWSTR eff_name = edit->get_effect_name(p_write->effect);
				if (eff_name) {
					int target_index = -1;
					int count = 0;

					int total_effects = edit->get_effect_list(p_write->obj, nullptr, 0);
					if (total_effects > 0) {
						std::vector<EFFECT_HANDLE> effect_list(total_effects);
						edit->get_effect_list(p_write->obj, effect_list.data(), total_effects);

						for (int i = 0; i < total_effects; ++i) {
							LPCWSTR cur_name = edit->get_effect_name(effect_list[i]);
							if (cur_name && std::wcscmp(cur_name, eff_name) == 0) {
								if (effect_list[i] == p_write->effect) {
									target_index = count;
									break;
								}
								count++;
							}
						}
					}
					std::wstring spec_effect_name = eff_name;
					if (target_index > 0) {
						spec_effect_name += L":" + std::to_wstring(target_index);
					}

					std::string val = p_write->value;
					size_t comma_pos = val.find(',');
					if (comma_pos != std::string::npos) {
						val = val.substr(0, comma_pos);
					}

					std::string add = "";
					int tempnum = p_write->SectionNum;
					for (int i = 0; i < tempnum; i++) {
						add = add + "," + val;
					}

					std::string new_val = p_write->value + add + ",LocusMotion,0|" + std::to_string(p_write->LocusID);

					edit->set_object_item_value(
						p_write->obj,
						spec_effect_name.c_str(),
						p_write->item_name.c_str(),
						new_val.c_str()
					);
				}
			}

			delete p_write;
			});
	}
}

void P_Effect::DeleteTrackbar() const {
	OBJECT_HANDLE target_obj = ObjectHandle;
	EFFECT_HANDLE target_effect = EffectHandle;

	if (edit_handle && target_obj && target_effect) {
		struct WriteParam {
			OBJECT_HANDLE obj;
			EFFECT_HANDLE effect;
			std::wstring item_name;
			std::string value;
		};

		WriteParam* p_param = new WriteParam{
			target_obj,
			target_effect,
			Name,
			StrValue
		};

		edit_handle->call_edit_section_param(p_param, [](void* param_ptr, EDIT_SECTION* edit) {
			if (!param_ptr) return;

			WriteParam* p_write = static_cast<WriteParam*>(param_ptr);

			if (edit && p_write->obj && p_write->effect) {
				LPCWSTR eff_name = edit->get_effect_name(p_write->effect);
				if (eff_name) {
					int target_index = -1;
					int count = 0;
					int total_effects = edit->get_effect_list(p_write->obj, nullptr, 0);
					if (total_effects > 0) {
						std::vector<EFFECT_HANDLE> effect_list(total_effects);
						edit->get_effect_list(p_write->obj, effect_list.data(), total_effects);

						for (int i = 0; i < total_effects; ++i) {
							LPCWSTR cur_name = edit->get_effect_name(effect_list[i]);
							if (cur_name && std::wcscmp(cur_name, eff_name) == 0) {
								if (effect_list[i] == p_write->effect) {
									target_index = count;
									break;
								}
								count++;
							}
						}
					}

					std::wstring spec_effect_name = eff_name;
					if (target_index > 0) {
						spec_effect_name += L":" + std::to_wstring(target_index);
					}

					std::string new_val = p_write->value;
					size_t comma_pos = new_val.find(',');
					if (comma_pos != std::string::npos) {
						new_val = new_val.substr(0, comma_pos);
					}

					if (new_val.empty()) {
						new_val = "0.00";
					}

					edit->set_object_item_value(
						p_write->obj,
						spec_effect_name.c_str(),
						p_write->item_name.c_str(),
						new_val.c_str()
					);
				}
			}

			delete p_write;
			});
	}
}

void P_Effect::ChangeIDTrackbar(int NewLocusID) const {
	OBJECT_HANDLE target_obj = ObjectHandle;
	EFFECT_HANDLE target_effect = EffectHandle;

	if (edit_handle && target_obj && target_effect) {
		struct WriteParam {
			OBJECT_HANDLE obj;
			EFFECT_HANDLE effect;
			int new_locus_id;
			std::wstring item_name;
			std::string value;
		};

		WriteParam* p_param = new WriteParam{
			target_obj,
			target_effect,
			NewLocusID,
			Name,
			StrValue
		};

		edit_handle->call_edit_section_param(p_param, [](void* param_ptr, EDIT_SECTION* edit) {
			if (!param_ptr) return;

			WriteParam* p_write = static_cast<WriteParam*>(param_ptr);

			if (edit && p_write->obj && p_write->effect) {
				LPCWSTR eff_name = edit->get_effect_name(p_write->effect);
				if (eff_name) {
					// 同名エフェクトのインデックスを取得
					int target_index = -1;
					int count = 0;
					int total_effects = edit->get_effect_list(p_write->obj, nullptr, 0);
					if (total_effects > 0) {
						std::vector<EFFECT_HANDLE> effect_list(total_effects);
						edit->get_effect_list(p_write->obj, effect_list.data(), total_effects);

						for (int i = 0; i < total_effects; ++i) {
							LPCWSTR cur_name = edit->get_effect_name(effect_list[i]);
							if (cur_name && std::wcscmp(cur_name, eff_name) == 0) {
								if (effect_list[i] == p_write->effect) {
									target_index = count;
									break;
								}
								count++;
							}
						}
					}

					std::wstring spec_effect_name = eff_name;
					if (target_index > 0) {
						spec_effect_name += L":" + std::to_wstring(target_index);
					}

					std::string current_val = p_write->value;

					LPCSTR val_ptr = edit->get_object_item_value(
						p_write->obj,
						spec_effect_name.c_str(),
						p_write->item_name.c_str()
					);
					if (val_ptr) {
						current_val = val_ptr;
					}

					std::string new_val;
					size_t pipe_pos = current_val.find('|');

					if (pipe_pos != std::string::npos) {
						//"|"より前の文字列を残して新しいLocusIDに置き換える
						new_val = current_val.substr(0, pipe_pos + 1) + std::to_string(p_write->new_locus_id);
					}
					else {
						new_val = current_val;
					}
					edit->set_object_item_value(
						p_write->obj,
						spec_effect_name.c_str(),
						p_write->item_name.c_str(),
						new_val.c_str()
					);
				}
			}

			delete p_write;
			});
	}
}

bool P_Effect::Update(D2D1_RECT_F Rect, CUR Cursor) const {
	bool Return = false;
	Hover = Cursor.RectCheck(Rect) && Cursor.RangeCheck(L"EffectsList");

	if (Cursor.drop && Hover && Tipe == L"項目") {
		if (Use) {
			LocusData.SetAllLocuses(LocusID, Editor.ToLocuses());
		}
		else {
			int ID = LocusData.NewSetLocuses(SectionNum, Editor.ToLocuses());
			SetTrackbar(ID);
			Use = true;
			LocusID = ID;
		}
		Return = true;
	}

	if (Cursor.RangeCheck(L"EffectsList")) {
		if (Hover && (Cursor.click || Cursor.rclick)) {
			if (Tipe == L"エフェクト" || Use) {
				Open = (Open + 1) % 2;
			}
			else {
				if (Tipe == L"項目" && !Use && !UseElse) {
					HMENU hMenu = CreatePopupMenu();
					if (hMenu) {
						AppendMenuW(hMenu, MF_STRING, 101, L"新規作成");
						AppendMenuW(hMenu, MF_STRING, 102, L"エディタの曲線を適用");
						AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
						if (Effects.CopyID == -1) {
							AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 103, L"貼り付け");
						}
						else {
							AppendMenuW(hMenu, MF_STRING, 103, L"貼り付け");
						}

						POINT pt = { (LONG)Cursor.x, (LONG)Cursor.y };
						ClientToScreen(Cursor.hwnd, &pt);

						SetForegroundWindow(Cursor.hwnd);
						int cmd = TrackPopupMenu(
							hMenu,
							TPM_RIGHTBUTTON | TPM_RETURNCMD,
							pt.x, pt.y,
							0, Cursor.hwnd, NULL
						);
						DestroyMenu(hMenu);

						switch (cmd) {
						case 101:
						{
							int new_id = LocusData.NewCLocus(SectionNum);
							SetTrackbar(new_id);

							Use = true;
							Open = true;
							Return = true;
							break;
						}
						case 102:
						{
							int new_id = LocusData.NewSetLocuses(SectionNum, Editor.ToLocuses());
							SetTrackbar(new_id);

							Use = true;
							Open = true;
							Return = true;
							break;
						}
						case 103:
						{
							SetTrackbar(Effects.CopyID);
							Use = true;
							Return = true;
							break;
						}
						}
					}
				}
			}
			Return = true;
		}

		if (Hover && Cursor.rclick && Tipe == L"項目" && Use) {
			HMENU hMenu = CreatePopupMenu();
			if (hMenu) {
				AppendMenuW(hMenu, MF_STRING, 101, L"コピー");
				if (Effects.CopyID == -1) {
					AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 102, L"貼り付け");
				}
				else {
					AppendMenuW(hMenu, MF_STRING, 102, L"貼り付け");
				}
				AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
				AppendMenuW(hMenu, MF_STRING, 103, L"新規IDを作成");
				AppendMenuW(hMenu, MF_STRING, 104, L"削除");

				POINT pt = { (LONG)Cursor.x, (LONG)Cursor.y };
				ClientToScreen(Cursor.hwnd, &pt);

				SetForegroundWindow(Cursor.hwnd);
				int cmd = TrackPopupMenu(
					hMenu,
					TPM_RIGHTBUTTON | TPM_RETURNCMD,
					pt.x, pt.y,
					0, Cursor.hwnd, NULL
				);
				DestroyMenu(hMenu);

				switch (cmd) {
				case 101:
				{
					Effects.CopyID = LocusID;
					break;
				}
				case 102: 
				{
					if (Effects.CopyID != -1) {
						ChangeIDTrackbar(Effects.CopyID);
					}
					break;
				}
				case 103:
				{
					int old_id = LocusID;
					int new_id = LocusData.NewCLocus(SectionNum);
					LocusData.CLocus[new_id] = LocusData.CLocus[LocusID];
					ChangeIDTrackbar(new_id);
					break;
				}
				case 104:
				{
					DeleteTrackbar();
					Open = false;
					Use = false;
					break;
				}
				}
			}
		}
	}

	HoverAnime += ((float)Hover - HoverAnime) / 2.0f;
	if (abs(HoverAnime - (float)Hover) < 0.001) {
		HoverAnime = (float)Hover;
	}
	OpenAnime += ((float)Open - OpenAnime) / 2.5f;
	if (abs(OpenAnime - (float)Open) < 0.001) {
		OpenAnime = (float)Open;
	}
	if (!(HoverAnime == (float)Hover && OpenAnime == (float)Open)) {
		Return = true;
	}

	if (Use && Open && Tipe == L"項目") {
		D2D1_RECT_F LocusRect = D2D1::RectF(Rect.left + 5.0f, Rect.bottom + 5.0f, Rect.right - 5.0f, Rect.bottom + 120.0f - 5.0f);
		if (Locus_Update(LocusRect, Cursor)) {
			Return = true;
		}
		if (PageX != PageXF) {
			Return = true;
		}
	}

	if (InitialLocusID || ChangeLocus) {
		ChangeIDTrackbar(LocusID);
		InitialLocusID = false;
		ChangeLocus = false;
	}
	return Return;
}

void P_Effect::Locus_Draw(D2D1_RECT_F Rect, CUR* Cursor) const {
	if (LocusID < 0 || LocusID >= static_cast<int>(LocusData.CLocus.size())) {
		Draw_Text(Rect, L"指定されたIDは定義されていません", TextAlign::Center);
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Border")));
		g_pRenderTarget->DrawRectangle(Rect, g_pBrush, 1.0f);
		return;
	}
	if (SectionWidth.size() < static_cast<size_t>(SectionNum) ||
		Value.size() < static_cast<size_t>(SectionNum + 1)) {
		return;
	}

	if (Geometry.size() != static_cast<size_t>(SectionNum)) {
		Geometry.resize(SectionNum);
	}

	if (SectionWidth.empty() || Value.empty()) return;

	std::vector<float> width;
	float add = 1.0f;
	auto it = std::min_element(SectionWidth.begin(), SectionWidth.end());
	if (it != SectionWidth.end() && *it * (Rect.right - Rect.left) < (Rect.bottom - Rect.top)) {
		add = (Rect.bottom - Rect.top) / (*it * (Rect.right - Rect.left));
	}

	for (int i = 0; i < SectionNum; i++) {
		width.push_back(SectionWidth[i] * (Rect.right - Rect.left) * add);
	}

	float diff = maxValue - minValue;
	if (std::abs(diff) < 0.0001f) {
		diff = 1.0f;
	}

	float left = Rect.left - PageX;
	for (int i = 0; i < SectionNum; i++) {
		D2D1_RECT_F DrawRect = D2D1::RectF(left, Rect.top, left + width[i], Rect.bottom - 20.0f);

		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Background")));
		g_pRenderTarget->FillRectangle(DrawRect, g_pBrush);
		g_pBrush->SetColor(D2D1::ColorF( 0x7274db, (LocusData.CLocus[LocusID].Locuses[i].Modifier.size() != 0) ? 0.1f : 0.0f));
		g_pRenderTarget->FillRectangle(DrawRect, g_pBrush);
		g_pBrush->SetColor(D2D1::ColorF(0xffffff, LocusHoverAnime[i]*0.1f));
		g_pRenderTarget->FillRectangle(DrawRect, g_pBrush);

		float normS = (Value[i] - minValue) / diff;
		float normE = (Value[i + 1] - minValue) / diff;

		float sy_pos = DrawRect.bottom - normS * (DrawRect.bottom - DrawRect.top);
		float ey_pos = DrawRect.bottom - normE * (DrawRect.bottom - DrawRect.top);

		if (i < static_cast<int>(LocusData.CLocus[LocusID].Locuses.size())) {
			UpdateGeometry(LocusData.CLocus[LocusID].Locuses[i], DrawRect, i, normS, normE);

			if (Geometry[i]) {
				g_pRenderTarget->PushAxisAlignedClip(
					DrawRect,
					D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
				);

				g_pBrush->SetColor(D2D1::ColorF(0x54d1ff));
				g_pRenderTarget->DrawGeometry(Geometry[i].Get(), g_pBrush, 2.5f);

				g_pRenderTarget->PopAxisAlignedClip();
			}
		}

		//枠線
		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Border")));
		g_pRenderTarget->DrawRectangle(DrawRect, g_pBrush, 1.0f);

		if (Cursor->drag) {
			g_pBrush->SetColor(D2D1::ColorF(0x85deff, LocusHoverAnime[i]));
			g_pRenderTarget->DrawRectangle(DrawRect, g_pBrush, 2.5f);
		}

		g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Border")));
		D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2(DrawRect.left, sy_pos), 3.5f, 3.5f);
		g_pRenderTarget->FillEllipse(ellipse, g_pBrush);

		if (i == SectionNum - 1) {
			D2D1_ELLIPSE end_ellipse = D2D1::Ellipse(D2D1::Point2(DrawRect.right, ey_pos), 4.0f, 4.0f);
			g_pRenderTarget->FillEllipse(end_ellipse, g_pBrush);
		}

		left += width[i];
	}
}

void P_Effect::Draw(D2D1_RECT_F Rect, CUR* Cursor) const {
	D2D1_ROUNDED_RECT roundedRect2 = D2D1::RoundedRect(Rect, 5, 5);

	D2D1::ColorF colorNormal(config->get_color_code(config, "ButtonBody"));
	D2D1::ColorF colorHover(config->get_color_code(config, "ButtonBodyHover"));

	float r = colorNormal.r * (1.0f - HoverAnime) + colorHover.r * HoverAnime;
	float g = colorNormal.g * (1.0f - HoverAnime) + colorHover.g * HoverAnime;
	float b = colorNormal.b * (1.0f - HoverAnime) + colorHover.b * HoverAnime;

	g_pBrush->SetColor(D2D1::ColorF(r, g, b));
	g_pRenderTarget->FillRoundedRectangle(roundedRect2, g_pBrush);
	if (Cursor->drag && Tipe == L"項目") {
		g_pBrush->SetColor(D2D1::ColorF(0x85deff, HoverAnime));
		g_pRenderTarget->DrawRoundedRectangle(roundedRect2, g_pBrush, 1.5f);
	}
	if (UseElse) {
		g_pBrush->SetColor(D2D1::ColorF(0xff0000, 0.5f));
		g_pRenderTarget->DrawRoundedRectangle(roundedRect2, g_pBrush, 1.0f);
	}

	g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Text")));
	Draw_Text(D2D1::RectF(Rect.left + 30, Rect.top, Rect.right, Rect.bottom), Name);
	if (!(Tipe == L"項目" && !Use)) {
		Draw_Svg(201, D2D1::RectF(Rect.left, Rect.top, Rect.left + 30, Rect.bottom), 0.4f, -90.0f * (1 - OpenAnime), D2D1::ColorF(config->get_color_code(config, "Text")));
		if (Open && Tipe == L"項目") {
			Locus_Draw(D2D1::RectF(Rect.left + 5.0f, Rect.bottom + 5.0f, Rect.right - 5.0f, Rect.bottom + 120.0f - 5.0f), Cursor);
		}
		if (Use) {
			Draw_Text(D2D1::RectF(Rect.left, Rect.top, Rect.right - 10.0f, Rect.bottom), L"ID : " + std::to_wstring(LocusID), TextAlign::Right);
		}
	}
}

void P_Effects::GetObjectEffects() {
	std::vector<P_Effect> result_list;

	if (!edit_handle) return;

	std::unordered_map<EFFECT_HANDLE, P_Effect> old_handle_map;
	std::unordered_map<std::wstring, P_Effect> old_key_map;

	int current_effect_index = -1;
	std::wstring current_parent = L"";

	for (const auto& item : Effect) {
		if (item.Tipe == L"エフェクト") {
			current_effect_index++;
			current_parent = item.Name;

			if (item.EffectHandle) {
				old_handle_map[item.EffectHandle] = item;
			}
			old_key_map[item.GetKey(current_effect_index)] = item;
		}
		else {
			std::wstring item_key = std::to_wstring(reinterpret_cast<uintptr_t>(item.EffectHandle)) + L"::" + item.Name;
			old_key_map[item_key] = item;
			old_key_map[item.GetKey(current_effect_index, current_parent)] = item;
		}
	}

	struct ReadSectionParam {
		std::vector<P_Effect>* p_out_list;
		const std::unordered_map<EFFECT_HANDLE, P_Effect>* p_handle_map;
		const std::unordered_map<std::wstring, P_Effect>* p_key_map;
	} read_param = { &result_list, &old_handle_map, &old_key_map };

	edit_handle->call_read_section_param(&read_param, [](void* param, EDIT_SECTION* edit) {
		auto p_param = static_cast<ReadSectionParam*>(param);
		auto p_out_list = p_param->p_out_list;
		const auto& old_handle_map = *p_param->p_handle_map;
		const auto& old_key_map = *p_param->p_key_map;

		OBJECT_HANDLE object = edit->get_focus_object();
		if (!object) return;

		int effect_count = edit->get_effect_list(object, nullptr, 0);
		if (effect_count <= 0) return;

		std::vector<EFFECT_HANDLE> effects(effect_count);
		edit->get_effect_list(object, effects.data(), effect_count);

		for (int i = 0; i < effect_count; ++i) {
			EFFECT_HANDLE effect_h = effects[i];
			if (!effect_h) continue;

			LPCWSTR effect_name = edit->get_effect_name(effect_h);
			if (!effect_name) continue;

			P_Effect effect_item;
			effect_item.Name = effect_name;
			effect_item.Tipe = L"エフェクト";
			effect_item.EffectHandle = effect_h;
			effect_item.Open = true;
			effect_item.Use = false;

			auto it_h = old_handle_map.find(effect_h);
			if (it_h != old_handle_map.end()) {
				effect_item.Open = it_h->second.Open;
				effect_item.OpenAnime = it_h->second.OpenAnime;
			}
			else {
				auto it_k = old_key_map.find(effect_item.GetKey(i));
				if (it_k != old_key_map.end()) {
					effect_item.Open = it_k->second.Open;
					effect_item.OpenAnime = it_k->second.OpenAnime;
				}
			}

			p_out_list->push_back(effect_item);

			struct EnumParam {
				std::vector<P_Effect>* p_list;
				std::wstring parent_name;
				EFFECT_HANDLE parent_handle;
				OBJECT_HANDLE obj_handle;
				int effect_index;
				const std::unordered_map<std::wstring, P_Effect>* p_key_map;
				EDIT_SECTION* edit;
			} enum_param = { p_out_list, effect_name, effect_h, object, i, &old_key_map, edit };

			edit_handle->enum_effect_item(effect_name, &enum_param, [](void* p_data, LPCWSTR item_name, int item_type) {
				auto p_enum = static_cast<EnumParam*>(p_data);

				if (item_name && (item_type == EDIT_HANDLE::EFFECT_ITEM_TYPE_INTEGER ||
					item_type == EDIT_HANDLE::EFFECT_ITEM_TYPE_NUMBER)) {

					P_Effect child_item;
					child_item.Name = item_name;
					child_item.Tipe = L"項目";
					child_item.EffectHandle = p_enum->parent_handle;
					child_item.ObjectHandle = p_enum->obj_handle;
					child_item.Open = false;
					child_item.Use = false;
					child_item.UseElse = false;
					child_item.SectionNum = p_enum->edit->get_object_section_num(p_enum->obj_handle);
					child_item.LocusHover.assign(child_item.SectionNum, false);
					child_item.LocusHoverAnime.assign(child_item.SectionNum, 0.0f);
					OBJECT_LAYER_FRAME frame = p_enum->edit->get_object_layer_frame(p_enum->obj_handle);

					float total_length = static_cast<float>(frame.end - frame.start + 1);
					child_item.SectionWidth.clear();
					child_item.FrameS.clear(); child_item.FrameF.clear();

					if (child_item.SectionNum <= 1 || total_length <= 0.0f) {
						child_item.SectionWidth.push_back(1.0f);
						child_item.FrameS.push_back(static_cast<float>(frame.start));
						child_item.FrameF.push_back(static_cast<float>(frame.end));
					}
					else {
						std::vector<int> section_frames;
						section_frames.push_back(frame.start);
						for (int i = 1; i < child_item.SectionNum; ++i) {
							section_frames.push_back(p_enum->edit->get_object_section_frame(p_enum->obj_handle, i));
						}
						section_frames.push_back(frame.end + 1);

						for (int i = 0; i < child_item.SectionNum; ++i) {
							int sec_start = section_frames[i];
							int sec_end = section_frames[i + 1];
							float sec_length = static_cast<float>(sec_end - sec_start);

							child_item.SectionWidth.push_back(sec_length / total_length);
							child_item.FrameS.push_back(static_cast<float>(sec_start));
							child_item.FrameF.push_back(static_cast<float>(sec_end - 1));
						}
					}

					LPCSTR val_ptr = p_enum->edit->get_object_item_value(p_enum->obj_handle, p_enum->parent_name.c_str(), item_name);
					child_item.StrValue = val_ptr ? val_ptr : "0.00";

					child_item.Value.clear();
					{
						std::stringstream ss(child_item.StrValue);
						std::string token;
						float last_val = 0.0f;

						while (std::getline(ss, token, ',')) {
							if (!token.empty()) {
								try {
									last_val = std::stof(token);
									child_item.Value.push_back(last_val);
								}
								catch (...) {
									child_item.Value.push_back(last_val);
								}
							}
						}

						size_t target_count = static_cast<size_t>(child_item.SectionNum + 1);
						while (child_item.Value.size() < target_count) {
							child_item.Value.push_back(last_val);
						}

						if (child_item.Value.size() > target_count) {
							child_item.Value.resize(target_count);
						}
						auto it = std::min_element(child_item.Value.begin(), child_item.Value.end());
						child_item.minValue = *it;
						it = std::max_element(child_item.Value.begin(), child_item.Value.end());
						child_item.maxValue = *it;
						if (child_item.minValue == child_item.maxValue) {
							auto it = std::find(child_item.Value.begin(), child_item.Value.end(), child_item.minValue);
							if (it != child_item.Value.end()) {
								*it = child_item.minValue - 0.01f;
								child_item.minValue -= 0.01f;
							}
						}
					}

					TRACK_INFO track_info = {};
					if (p_enum->edit->get_effect_track_info(p_enum->parent_handle, item_name, &track_info, sizeof(TRACK_INFO))) {
						if (track_info.mode != nullptr) {
							if (std::wcscmp(track_info.mode, L"LocusMotion") == 0) {
								child_item.Use = true;
								if (track_info.param && track_info.param_num > 0) {
									if (track_info.param[0] != -1) {
										child_item.LocusID = static_cast<int>(track_info.param[0]);

									}
									else {
										child_item.LocusID = LocusData.NewSetLocuses(child_item.SectionNum, Editor.ToLocuses());
										track_info.param[0] = child_item.LocusID;
										child_item.InitialLocusID = true;
									}
								}
							}
							else {
								child_item.UseElse = true;
							}
						}
					}

					if (child_item.Use &&
						child_item.LocusID >= 0 &&
						child_item.LocusID < static_cast<int>(LocusData.CLocus.size()))
					{
						auto& currentLocusList = LocusData.CLocus[child_item.LocusID].Locuses;

						if (static_cast<size_t>(child_item.SectionNum) > currentLocusList.size()) {
							LOCUSES NewLocuses;
							LOCUS NewLocus;
							NewLocus.Mode = 0;
							NewLocus.Turn = false;
							NewLocus.S = { 0.0f, 0.0f };
							NewLocus.H1 = { 0.3f, 0.3f };
							NewLocus.H2 = { 0.7f, 0.7f };
							NewLocus.F = { 1.0f, 1.0f };
							NewLocuses.Locus.push_back(NewLocus);

							size_t diff_count = static_cast<size_t>(child_item.SectionNum) - currentLocusList.size();
							for (size_t i = 0; i < diff_count; i++) {
								currentLocusList.push_back(NewLocuses);
							}
						}
					}

					std::wstring handle_item_key = std::to_wstring(reinterpret_cast<uintptr_t>(p_enum->parent_handle)) + L"::" + item_name;
					auto it_hk = p_enum->p_key_map->find(handle_item_key);

					if (it_hk != p_enum->p_key_map->end()) {
						child_item.Open = it_hk->second.Open;
						child_item.OpenAnime = it_hk->second.OpenAnime;
					}
					else {
						std::wstring child_key = child_item.GetKey(p_enum->effect_index, p_enum->parent_name);
						auto it_k = p_enum->p_key_map->find(child_key);
						if (it_k != p_enum->p_key_map->end()) {
							child_item.Open = it_k->second.Open;
							child_item.OpenAnime = it_k->second.OpenAnime;
						}
					}

					p_enum->p_list->push_back(child_item);
				}
				});
		}
		});

	Effect = result_list;
}

bool P_Effects::Update(CUR Cursor) {
	D2D1_RECT_F Rect = Cursor.range.Range[L"EffectsList"];
	bool Return = false;
	YRange = 0.0;
	bool EffectOpen = false;
	for (const auto& e : Effect) {
		if (e.Tipe == L"エフェクト") {
			YRange += 30.0;
			if (e.Open) {
				YRange += 8.0;
				EffectOpen = true;
			}
			else
				EffectOpen = false;
		}
		else if(EffectOpen) {
			YRange += 30.0;
			if (e.Open)
				YRange += 100.0f;
		}
	}
	YRange = (std::max)(YRange - (Rect.bottom - Rect.top), 0.0f);

	if (Cursor.RangeCheck(L"EffectsList")) {
		if (Cursor.wheel > 0 && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
			PageYF -= 60.0f;
		}
		if (Cursor.wheel < 0 && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
			PageYF += 60.0f;
		}
	}
	HideButtonHover = Cursor.RectCheck(Cursor.range.Range[L"EffectsListHide"]);

	if (HideButtonHover && Cursor.click) {
		Hide = !Hide;
		Return = true;
	}

	PageYF = (std::min)((std::max)(PageYF, 0.0f), YRange);
	PageY += (PageYF - PageY) / 3.0f;
	if (abs(PageY - PageYF) < 0.001) {
		PageY = PageYF;
	}

	HideButtonHoverAnime += ((float)HideButtonHover - HideButtonHoverAnime) / 3.0f;
	if (abs((float)HideButtonHover - HideButtonHoverAnime) < 0.001) {
		HideButtonHoverAnime = (float)HideButtonHover;
	}

	if (Hide) {
		HideAnime += ((float)Hide - HideAnime) / 2.3f;
	}
	else {
		HideAnime += ((float)Hide - HideAnime) / 2.8f;
	}
	if (abs(HideAnime - (float)Hide) < 0.001) {
		HideAnime = (float)Hide;
	}

	float y = 3 - PageY;
	bool open = false;
	for (const auto& e : Effect) {
		if (e.Tipe == L"エフェクト") {
			D2D1_RECT_F PEffectRect = D2D1::RectF(Rect.left + 1, Rect.top + y + 1, Rect.right - 5, Rect.top + y + 30 - 1);
			if (e.Update(PEffectRect, Cursor)) {
				Return = true;
			}
			open = e.Open;
			y += 30.0f;
		}
		else if ((e.Tipe == L"項目") && (open == true)) {
			D2D1_RECT_F PEffectRect = D2D1::RectF(Rect.left + 15 + 1, Rect.top + y + 1, Rect.right - 5, Rect.top + y + 30 - 1);
			if (e.Update(PEffectRect, Cursor)) {
				Return = true;
			}
			y += 30.0f + 100.0f * e.Use * e.Open;
		}
	}
	if (PageY != PageYF || HideButtonHoverAnime != (float)HideButtonHover || HideAnime !=(float)Hide) {
		Return = true;
	}
	AnimeMoving = Return;
	return Return;
}

void P_Effects::Draw(CUR Cursor) {
	D2D1_RECT_F Rect = Cursor.range.Range[L"EffectsList"];

	g_pRenderTarget->PushAxisAlignedClip(
		Rect,
		D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
	);

	g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Grouping")));
	g_pRenderTarget->FillRectangle(Rect, g_pBrush);

	float RectYRange = (Rect.bottom - Rect.top);
	float ScrollRatio;
	if (YRange != 0.0) {
		ScrollRatio = PageY / YRange;
	}
	else {
		ScrollRatio = 0;
	}
	float ScrollRange = RectYRange / (YRange + RectYRange) * RectYRange - 10.0f;
	float ScrollY = Rect.top + ScrollRange / 2.0f + 5.0f + ScrollRatio * (RectYRange - ScrollRange - 10.0f);
	D2D1_RECT_F ScrollRect = D2D1::RectF(Rect.right - 6.0f, ScrollY - ScrollRange / 2.0f, Rect.right - 2.0f, ScrollY + ScrollRange / 2.0f);

	g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Border"), 0.5f));
	D2D1_ROUNDED_RECT ScrollRoundedRect = D2D1::RoundedRect(ScrollRect, 2.0f, 2.0f);
	g_pRenderTarget->FillRoundedRectangle(ScrollRoundedRect, g_pBrush);

	float y = 3 - PageY;
	bool open = false;
	bool item = false;
	for (const auto& e : Effect) {
		if (e.Tipe == L"エフェクト") {
			if (item) {
				y += 8.0f;
				item = false;
			}
			D2D1_RECT_F PEffectRect = D2D1::RectF(Rect.left + 1, Rect.top + y + 1, Rect.right - 10, Rect.top + y + 30 - 1);
				e.Draw(PEffectRect, &Cursor);
			open = e.Open;
			y += 30.0f;
		}
		else if ((e.Tipe == L"項目") && (open == true)) {
			item = true;
			D2D1_RECT_F PEffectRect = D2D1::RectF(Rect.left + 15 + 1, Rect.top + y + 1, Rect.right - 10, Rect.top + y + 30 - 1);
				e.Draw(PEffectRect, &Cursor);
			y += 30.0f + 100.0f * e.Use * e.Open;
		}
	}
	g_pRenderTarget->PopAxisAlignedClip();

	D2D1_RECT_F HideButtonRect = Cursor.range.Range[L"EffectsListHide"];
	g_pBrush->SetColor(D2D1::ColorF(config->get_color_code(config, "Background")));
	g_pRenderTarget->FillRectangle(HideButtonRect, g_pBrush);
	g_pBrush->SetColor(D2D1::ColorF(0xffffff, HideButtonHoverAnime * 0.1f));
	g_pRenderTarget->FillRectangle(HideButtonRect, g_pBrush);
	if (HideButtonRect.bottom - HideButtonRect.top < HideButtonRect.right - HideButtonRect.left) {
		Draw_Svg(331, HideButtonRect, 0.5f, 180.0f * !Hide, D2D1::ColorF(config->get_color_code(config, "Text")));
	}
	else {
		Draw_Svg(331, HideButtonRect, 0.5f, 180.0f * !Hide - 90.0f, D2D1::ColorF(config->get_color_code(config, "Text")));
	}
}