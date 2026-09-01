#include "Direct2D.h"
#include "GUI.h"
#include "resource1.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#include <commctrl.h>

#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <cstdint>
#include <algorithm>

#define SampleWindowName L"LocusMotion"

HCURSOR hDragCursor = NULL;

//---------------------------------------------------------------------
//	汎用プラグイン構造体定義
//---------------------------------------------------------------------
COMMON_PLUGIN_TABLE common_plugin_table = {
	L"LocusMotion",								// プラグインの名前
	L"LocusMotion β1.0 もも猫(@momo_neeeko)",		// プラグインの情報
};

//---------------------------------------------------------------------
//	必要とする本体バージョン番号取得関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) DWORD RequiredVersion() {
	return 2003300;
}

//---------------------------------------------------------------------
//	ログ出力機能初期化関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) void InitializeLogger(LOG_HANDLE* handle) {
	logger = handle;
}

//---------------------------------------------------------------------
//	設定関連初期化関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) void InitializeConfig(CONFIG_HANDLE* handle) {
	config = handle;
}

//---------------------------------------------------------------------
//	プラグインDLL初期化関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version) {
	return true;
}

//---------------------------------------------------------------------
//	プラグインDLL解放関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) void UninitializePlugin() {
	DiscardDeviceResources();
}

//---------------------------------------------------------------------
//	汎用プラグイン構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable(void) {
	return &common_plugin_table;
}

//---------------------------------------------------------------------
//	ウィンドウプロシージャ関連変数
//---------------------------------------------------------------------
float LocusMotionVersion = 0.0f;

UINT WinWidth = 0;
UINT WinHeight = 0;
D2D1_POINT_2F BeforCursor = D2D1::Point2(0.0f, 0.0f);
CUR cursor;
EDITOR Editor;
P_Effects Effects;
LOCUSDATA LocusData;
HWND g_hPluginWnd = NULL;
bool WindowResize = true;

void OnEvent(void* param) {
	HWND hwnd = (HWND)param;
	Effects.GetObjectEffects();
	WindowResize = true;
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
	case WM_CREATE: {
		RECT rect;
		GetWindowRect(hwnd, &rect);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		if (width < 300 || height < 300) {
			SetWindowPos(hwnd, NULL, 0, 0, 500, 500, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}

		EDIT_LOCUSES defaultLocus;
		defaultLocus.Tipe = 0;
		defaultLocus.Locus[0].Mode = 0;
		defaultLocus.Locus[1].Mode = 1;
		defaultLocus.Locus[2].Mode = 2;
		defaultLocus.Locus[3].Mode = 3;

		defaultLocus.Locus[0].Turn = false;
		defaultLocus.Locus[1].Turn = false;
		defaultLocus.Locus[2].Turn = false;
		defaultLocus.Locus[3].Turn = false;

		defaultLocus.Locus[0].S = { 0.0f, 0.0f };
		defaultLocus.LocusUI[0].S = { 10.0f, 10.0f };
		defaultLocus.Locus[1].S = { 0.0f, 0.0f };
		defaultLocus.LocusUI[1].S = { 10.0f, 10.0f };
		defaultLocus.Locus[2].S = { 0.0f, 0.0f };
		defaultLocus.LocusUI[2].S = { 10.0f, 10.0f };
		defaultLocus.Locus[3].S = { 0.0f, 0.0f };
		defaultLocus.LocusUI[3].S = { 10.0f, 10.0f };

		defaultLocus.Locus[0].H1 = { 0.3f, 0.3f };
		defaultLocus.LocusUI[0].H1 = { 10.0f, 10.0f };
		defaultLocus.Locus[1].H1 = { 0.0f, 0.0f };
		defaultLocus.LocusUI[1].H1 = { 10.0f, 10.0f };
		defaultLocus.Locus[2].H1 = { 0.0f, 0.0f };
		defaultLocus.LocusUI[2].H1 = { 10.0f, 10.0f };
		defaultLocus.Locus[3].H1 = { 0.0f, 1.0f };
		defaultLocus.LocusUI[3].H1 = { 10.0f, 10.0f };

		defaultLocus.Locus[0].H2 = { 0.7f, 0.7f };
		defaultLocus.LocusUI[0].H2 = { 10.0f, 10.0f };
		defaultLocus.Locus[1].H2 = { 0.2f, 0.4f };
		defaultLocus.LocusUI[1].H2 = { 10.0f, 10.0f };
		defaultLocus.Locus[2].H2 = { 0.36939f, 0.5f };
		defaultLocus.LocusUI[2].H2 = { 10.0f, 10.0f };
		defaultLocus.Locus[3].H2 = { 0.0f, 0.0f };
		defaultLocus.LocusUI[3].H2 = { 10.0f, 10.0f };

		defaultLocus.Locus[0].F = { 1.0f, 1.0f };
		defaultLocus.LocusUI[0].F = { 10.0f, 10.0f };
		defaultLocus.Locus[1].F = { 1.0f, 1.0f };
		defaultLocus.LocusUI[1].F = { 10.0f, 10.0f };
		defaultLocus.Locus[2].F = { 1.0f, 1.0f };
		defaultLocus.LocusUI[2].F = { 10.0f, 10.0f };
		defaultLocus.Locus[3].F = { 1.0f, 1.0f };
		defaultLocus.LocusUI[3].F = { 10.0f, 10.0f };

		Editor.Locus.push_back(defaultLocus);
		Editor.SelectLocus = 0;

		SetTimer(hwnd, 1, 1000 / 60, NULL);

		cursor.action = true;
		cursor.hwnd = hwnd;
		break;
	}
	case WM_SIZE: {
		WinWidth = LOWORD(lparam);
		WinHeight = HIWORD(lparam);
		WindowResize = true;
		if (g_pRenderTarget) {
			g_pRenderTarget->Resize(D2D1::SizeU(WinWidth, WinHeight));
		}
		RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
		return 0;
	}
	case WM_PAINT: {
		PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		if (SUCCEEDED(CreateDeviceResources(hwnd))) {
			g_pRenderTarget->BeginDraw();

			COLORREF bgColor = (config && config->get_color_code) ? config->get_color_code(config, "Background") : 0x202020;
			g_pRenderTarget->Clear(D2D1::ColorF(bgColor));

			Editor.Draw(cursor);
			Effects.Draw(cursor);

			HRESULT hr = g_pRenderTarget->EndDraw();
			if (hr == D2DERR_RECREATE_TARGET) {
				DiscardDeviceResources();
			}
		}
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_TIMER: {
		cursor.range.Update(D2D1::RectF(0.0f, 0.0f, (float)WinWidth, (float)WinHeight), Effects.HideAnime);
		cursor.move = D2D1::Point2(cursor.x - BeforCursor.x, cursor.y - BeforCursor.y);
		BeforCursor = D2D1::Point2(cursor.x, cursor.y);
		if (cursor.action || Editor.AnimeMoving || Effects.AnimeMoving || WindowResize) {
			bool LocusRedraw = Editor.Update(&cursor, WindowResize);
			bool EffectRedraw = Effects.Update(cursor);
			if (LocusRedraw || EffectRedraw) {
				InvalidateRect(hwnd, NULL, FALSE);
			}
		}
		WindowResize = false;
		cursor.action = false;

		cursor.click = false;
		cursor.dclick = false;
		cursor.mclick = false;
		cursor.mdclick = false;
		cursor.rclick = false;
		cursor.rdclick = false;
		cursor.moveing = false;
		cursor.drop = false;
		cursor.wheel = 0.0f;
		if (!cursor.clicking && cursor.drag) {
			cursor.drag = false;
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
		if (cursor.drag) {
			SetCursor(hDragCursor);
		}
		break;
	}
	case WM_MOUSEMOVE: {
		cursor.x = (float)(short)LOWORD(lparam);
		cursor.y = (float)(short)HIWORD(lparam);
		cursor.moveing = true;
		cursor.action = true;
		break;
	}
	case WM_LBUTTONDOWN: {
		cursor.click = true;
		cursor.clicking = true;
		cursor.action = true;
		SetCapture(hwnd);
		break;
	}
	case WM_LBUTTONUP: {
		cursor.clicking = false;
		cursor.action = true;
		if (cursor.drag) {
			cursor.drag = false;
			cursor.drop = true;
		}
		if (GetCapture() == hwnd) {
			ReleaseCapture();
		}
		break;
	}
	case WM_LBUTTONDBLCLK: {
		cursor.dclick = true;
		cursor.click = true;
		cursor.clicking = true;
		cursor.action = true;
		SetCapture(hwnd);
		break;
	}
	case WM_MBUTTONDOWN: {
		cursor.mclick = true;
		cursor.mclicking = true;
		cursor.action = true;
		SetCapture(hwnd);
		break;
	}
	case WM_MBUTTONUP: {
		cursor.mclicking = false;
		cursor.action = true;
		if (GetCapture() == hwnd) {
			ReleaseCapture();
		}
		break;
	}
	case WM_RBUTTONDOWN: {
		cursor.rclick = true;
		cursor.rclicking = true;
		cursor.action = true;
		SetCapture(hwnd);
		break;
	}
	case WM_RBUTTONUP: {
		cursor.rclicking = false;
		cursor.action = true;
		if (GetCapture() == hwnd) {
			ReleaseCapture();
		}
		break;
	}
	case WM_MOUSEWHEEL: {
		cursor.action = true;
		short delta = GET_WHEEL_DELTA_WPARAM(wparam);
		cursor.wheel = (std::min)((std::max)(cursor.wheel + (float)(delta / 120), -1.5f), 1.5f);
		break;
	}
	case WM_GETMINMAXINFO: {
		LPMINMAXINFO mmi = (LPMINMAXINFO)lparam;
		mmi->ptMinTrackSize.x = 500;
		mmi->ptMinTrackSize.y = 500;
		return 0;
	}
	case WM_DESTROY: {
		KillTimer(hwnd, 1);
		return 0;
	}
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}

//---------------------------------------------------------------------
//	GetLocus関数 (スクリプトモジュール)
//---------------------------------------------------------------------
void GetLocus(SCRIPT_MODULE_PARAM* param) {
	int LocusID = param->get_param_int(0);
	int Section = (std::max)(param->get_param_int(1),0);
	double x = param->get_param_double(2);
	double time = param->get_param_array_double(3, Section + 1) - param->get_param_array_double(3, Section);
	double framerate = param->get_param_double(4);

	if (LocusID < 0 || LocusID >= (int)LocusData.CLocus.size()) {
		param->push_result_double(-1.0);
		return;
	}

	if (Section < 0 || Section >= (int)LocusData.CLocus[LocusID].Locuses.size()) {
		param->push_result_double(-1.0);
		return;
	}

	double Out = LocusData.CLocus[LocusID].Locuses[Section].PlayModifier(x, time, framerate, 20);
	param->push_result_double(Out);
}

SCRIPT_MODULE_FUNCTION functions[] = {
	{ L"GetLocus", GetLocus },
	{ nullptr }
};

SCRIPT_MODULE_TABLE script_module_table = {
	L"LocusMotion ScriptModule By もも猫",
	functions
};

//---------------------------------------------------------------------
//  プロジェクト保存/読み込み関数
//---------------------------------------------------------------------
void OnProjectSave(PROJECT_FILE* project) {
	if (!project) return;

	if (!LocusData.CLocus.empty()) {
		std::ostringstream oss;

		//先頭にバージョンを出力
		oss << LocusMotionVersion << " ";

		//CLocus の要素数を書き出し
		oss << LocusData.CLocus.size();

		for (const auto& clocus : LocusData.CLocus) {
			oss << " " << clocus.LocusID << " " << clocus.Locuses.size();
			for (const auto& locuses_item : clocus.Locuses) {
				//LOCUSの保存
				oss << " " << locuses_item.Locus.size();
				for (const auto& locus : locuses_item.Locus) {
					oss << " " << locus.Mode
						<< " " << (locus.Turn ? 1 : 0)
						<< " " << locus.S.x << " " << locus.S.y
						<< " " << locus.H1.x << " " << locus.H1.y
						<< " " << locus.H2.x << " " << locus.H2.y
						<< " " << locus.F.x << " " << locus.F.y;
				}

				//MODIFIERの保存
				oss << " " << locuses_item.Modifier.size();
				for (const auto& modifier : locuses_item.Modifier) {
					oss << " " << modifier.Mode << " " << modifier.Param.size();
					for (const auto& p : modifier.Param) {
						oss << " " << p;
					}
				}
			}
		}

		project->set_param_string("LocusMotionData", oss.str().c_str());
	}
}

void OnProjectLoad(PROJECT_FILE* project) {
	if (!project) return;

	LPCSTR rawData = project->get_param_string("LocusMotionData");
	if (!rawData) return;

	std::string strData(rawData);
	if (strData.empty()) return;

	std::istringstream iss(strData);

	//保存データのバージョンを読み込み
	float loadedVersion = -1.0f;
	if (!(iss >> loadedVersion)) return;

	//バージョン 0.0 用の読み込み処理
	if (loadedVersion == 0.0f) {
		size_t clocusCount = 0;
		if (!(iss >> clocusCount) || clocusCount == 0) return;

		LocusData.CLocus.clear();

		for (size_t i = 0; i < clocusCount; ++i) {
			CLOCUS clocus;
			size_t locusesCount = 0;
			if (!(iss >> clocus.LocusID >> locusesCount)) break;

			for (size_t j = 0; j < locusesCount; ++j) {
				LOCUSES locuses_item;

				//LOCUSの読み込み
				size_t locusCount = 0;
				if (!(iss >> locusCount)) break;

				for (size_t k = 0; k < locusCount; ++k) {
					LOCUS locus;
					int turn = 0;
					if (!(iss >> locus.Mode >> turn
						>> locus.S.x >> locus.S.y
						>> locus.H1.x >> locus.H1.y
						>> locus.H2.x >> locus.H2.y
						>> locus.F.x >> locus.F.y)) {
						break;
					}
					locus.Turn = (turn != 0);
					locuses_item.Locus.push_back(locus);
				}

				//MODIFIERの読み込み
				size_t modifierCount = 0;
				if (iss >> modifierCount) {
					for (size_t m = 0; m < modifierCount; ++m) {
						int mode = 0;
						size_t paramCount = 0;
						if (!(iss >> mode >> paramCount)) break;

						// 正しいModeでコンストラクタを呼び出し、ParamHoverやParamHoverAnimeを確保させる
						MODIFIER modifier(mode);
						modifier.Param.clear(); // デフォルト値を破棄して保存データで上書き

						for (size_t p = 0; p < paramCount; ++p) {
							double val = 0.0;
							if (!(iss >> val)) break;
							modifier.Param.push_back(val);
						}
						locuses_item.Modifier.push_back(modifier);
					}
				}

				clocus.Locuses.push_back(locuses_item);
			}
			LocusData.CLocus.push_back(clocus);
		}
	}
	//アップデートする時はここに追加
}

//---------------------------------------------------------------------
//  右クリックメニュー「エディタの曲線を適用」コールバック関数
//---------------------------------------------------------------------
void ProcApplyEditorLocusItemMenu(EDIT_SECTION* edit, OBJECT_HANDLE object, LPCWSTR effect, LPCWSTR item) {
	if (!edit || !object || !effect || !item) return;

	if (g_hPluginWnd && IsWindow(g_hPluginWnd)) {
		if (!IsWindowVisible(g_hPluginWnd)) {
			HWND hHostWnd = edit_handle ? edit_handle->get_host_app_window() : nullptr;
			if (hHostWnd) {
				HMENU hMainMenu = GetMenu(hHostWnd);
				if (hMainMenu) {
					HMENU hViewMenu = NULL;
					int count = GetMenuItemCount(hMainMenu);
					for (int i = 0; i < count; ++i) {
						wchar_t text[128] = {};
						GetMenuStringW(hMainMenu, i, text, 128, MF_BYPOSITION);
						if (std::wcsstr(text, L"表示")) {
							hViewMenu = GetSubMenu(hMainMenu, i);
							break;
						}
					}

					if (hViewMenu) {
						int viewCount = GetMenuItemCount(hViewMenu);
						for (int i = 0; i < viewCount; ++i) {
							wchar_t itemText[128] = {};
							GetMenuStringW(hViewMenu, i, itemText, 128, MF_BYPOSITION);
							if (std::wcsstr(itemText, L"LocusMotion")) {
								UINT cmdID = GetMenuItemID(hViewMenu, i);
								if (cmdID != (UINT)-1 && cmdID != 0) {
									SendMessage(hHostWnd, WM_COMMAND, MAKEWPARAM(cmdID, 0), 0);
								}
								break;
							}
						}
					}
				}
			}
		}
		else {
			SetForegroundWindow(g_hPluginWnd);
		}
	}

	TRACK_INFO track_info = {};
	if (!edit->get_object_track_info(object, effect, item, &track_info, sizeof(TRACK_INFO))) {
		return;
	}

	int target_locus_id = -1;
	if (track_info.mode && std::wcscmp(track_info.mode, L"LocusMotion") == 0) {
		if (track_info.param && track_info.param_num > 0) {
			target_locus_id = static_cast<int>(track_info.param[0]);
		}
	}

	int section_num = edit->get_object_section_num(object);

	if (target_locus_id < 0 || target_locus_id >= static_cast<int>(LocusData.CLocus.size())) {
		target_locus_id = LocusData.NewSetLocuses(section_num, Editor.ToLocuses());
	}
	else {
		LocusData.SetAllLocuses(target_locus_id, Editor.ToLocuses());
	}

	LPCSTR val_ptr = edit->get_object_item_value(object, effect, item);
	std::string current_val = val_ptr ? val_ptr : "0.00";

	std::stringstream ss(current_val);
	std::string token;
	std::vector<std::string> numeric_tokens;

	while (std::getline(ss, token, ',')) {
		if (token.empty()) continue;

		bool is_number = true;
		try {
			size_t idx = 0;
			std::stod(token, &idx);
			if (idx != token.size()) {
				is_number = false;
			}
		}
		catch (...) {
			is_number = false;
		}

		if (is_number) {
			numeric_tokens.push_back(token);
		}
		else {
			break;
		}
	}

	std::string values_part = "";

	if (!numeric_tokens.empty()) {
		for (size_t i = 0; i < numeric_tokens.size(); ++i) {
			if (i > 0) values_part += ",";
			values_part += numeric_tokens[i];
		}

		size_t required_count = static_cast<size_t>(section_num + 1);
		std::string last_val = numeric_tokens.back();
		while (numeric_tokens.size() < required_count) {
			values_part += "," + last_val;
			numeric_tokens.push_back(last_val);
		}
	}
	else {
		values_part = "0.00";
		for (int i = 0; i < section_num; ++i) {
			values_part += ",0.00";
		}
	}

	std::string new_val = values_part + ",LocusMotion,0|" + std::to_string(target_locus_id);
	edit->set_object_item_value(object, effect, item, new_val.c_str());
}

//---------------------------------------------------------------------
//	プラグイン登録関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host) {
	HMODULE hModule = NULL;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCWSTR)&RegisterPlugin,
		&hModule
	);

	hDragCursor = LoadCursor(hModule, MAKEINTRESOURCE(ID_CURSOR_DRAG));

	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_DBLCLKS;
	wcex.lpszClassName = SampleWindowName;
	wcex.lpfnWndProc = wnd_proc;
	wcex.hInstance = GetModuleHandle(0);
	wcex.hbrBackground = nullptr;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	if (!RegisterClassEx(&wcex)) {
		return;
	}

	auto hwnd = CreateWindowEx(
		0,
		SampleWindowName,
		SampleWindowName,
		WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr,
		nullptr,
		GetModuleHandle(0),
		nullptr
	);
	if (!hwnd) {
		return;
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	g_hPluginWnd = hwnd;
	host->register_window_client(SampleWindowName, hwnd);

	host->register_event_listener(EVENT_TYPE::CHANGE_FOCUS_OBJECT, hwnd, OnEvent);
	host->register_event_listener(EVENT_TYPE::UPDATE_OBJECT, hwnd, OnEvent);

	edit_handle = host->create_edit_handle();

	host->register_script_module_name(&script_module_table, L"LocusMotion");

	host->register_project_save_handler(OnProjectSave);
	host->register_project_load_handler(OnProjectLoad);

	host->register_object_item_menu(
		L"エディタの曲線を適用",
		false,
		ProcApplyEditorLocusItemMenu
	);
}

//---------------------------------------------------------------------
//	スクリプトモジュール構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) SCRIPT_MODULE_TABLE* GetScriptModuleTable(void) {
	return &script_module_table;
}