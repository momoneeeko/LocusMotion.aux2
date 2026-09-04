#pragma once

#include <d2d1.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#include <string>
#include <vector>
#include <map>

#include "plugin2.h"

extern EDIT_HANDLE* edit_handle;

struct POS {
	float x = 0.0f;
	float y = 0.0f;
};

struct CUR_MOD {
	HWND hwnd = NULL;
	float x = 0.0f;
	float y = 0.0f;
	float wheel = 0.0f;
	bool click = false;
	bool RectCheck(D2D1_RECT_F Rect) const {
		return (Rect.left < x && x < Rect.right && Rect.top < y && y < Rect.bottom);
	}
};

struct MODIFIER {
	int Mode = 0;
	std::vector<double> Param;

	std::vector<bool> ParamHover;
	std::vector<float> ParamHoverAnime;
	std::vector<bool> ParamUse;

	MODIFIER() : MODIFIER(1) {}

	MODIFIER(int Mode) {
		this->Mode = Mode;
		switch (Mode) {
		case 1: {	//コマ落ち
			Param.push_back(5.0);	// 間隔
			Param.push_back(2.0);	// 0=秒, 1=ミリ秒, 2=フレーム, 3=FPS 4=BPM 5=BPM取得
			Param.push_back(0.0);	// 開始位置
			Param.push_back(0.0);	// 0=秒, 1=ミリ秒, 2=フレーム 3=BPM参照
			for (int i = 0; i < 4; i++) {
				ParamHover.push_back(false);
				ParamHoverAnime.push_back(0.0f);
				ParamUse.push_back(false);
			}
			break;
		}
		case 2: {	//離散化
			Param.push_back(10.0);	// 離散化個数
			Param.push_back(0.0);	// 0=切り捨て, 1=四捨五入, 2=切り上げ
			for (int i = 0; i < 2; i++) {
				ParamHover.push_back(false);
				ParamHoverAnime.push_back(0.0f);
				ParamUse.push_back(false);
			}
			break;
		}
		case 3: {	//速度化
			Param.push_back(1.0);	// 増分
			for (int i = 0; i < 1; i++) {
				ParamHover.push_back(false);
				ParamHoverAnime.push_back(0.0f);
				ParamUse.push_back(false);
			}
			break;
		}
		case 4: {	//ループ
			Param.push_back(5.0);	// 回数
			for (int i = 0; i < 1; i++) {
				ParamHover.push_back(false);
				ParamHoverAnime.push_back(0.0f);
				ParamUse.push_back(false);
			}
			break;
		}
		}
	}

	double ChengeUnit(int Moto, int Ato, double Value) {
		double Milli = Value; // 初期値を設定
		switch (Moto) {
		case 0: Milli = Value * 1000.0; break;
		case 1: Milli = Value; break;
		case 2: {
			EDIT_INFO info{};
			edit_handle->get_edit_info(&info, sizeof(EDIT_INFO));
			if (info.scale != 0) {
				double framerate = static_cast<double>(info.rate) / info.scale;
				Milli = (framerate != 0.0) ? (Value / framerate * 1000.0) : 0.0;
			}
			break;
		}
		case 3: Milli = (Value != 0.0) ? (1000.0 / Value) : 0.0; break;
		case 4: Milli = (Value != 0.0) ? (60000.0 / Value) : 0.0; break;
		case 5: Milli = 1000.0; break;
		}

		switch (Ato) {
		case 0: return Milli / 1000.0;
		case 1: return Milli;
		case 2: {
			EDIT_INFO info{};
			edit_handle->get_edit_info(&info, sizeof(EDIT_INFO));
			if (info.scale != 0) {
				double framerate = static_cast<double>(info.rate) / info.scale;
				return framerate * (Milli / 1000.0);
			}
			return 0.0;
		}
		case 3: return (Milli != 0.0) ? (1000.0 / Milli) : 0.0;
		case 4: return (Milli != 0.0) ? (60000.0 / Milli) : 0.0;
		case 5: return 1.0;
		}

		return Milli; // 必ず値を返す
	}
	bool UpdataParam(D2D1_RECT_F Rect, CUR_MOD* Cursor);
	void PaintParam(ID2D1RenderTarget* pTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F Rect);
};

struct LOCUS {			//Locusの最小単位
	int Mode = 0;
	bool Turn = false;
	POS S = { 0.0f, 0.0f };			//絶対
	POS H1 = { 0.0f, 0.0f };		//相対
	POS H2 = { 0.0f, 0.0f };		//相対
	POS F = { 0.0f, 0.0f };			//絶対

	double LocusToValue(double Time, int n);
};

struct LOCUSES {		//Locusを集めたもの。
	std::vector<LOCUS> Locus;
	std::vector<MODIFIER> Modifier;

	double LocusesToValue(double Time, int n);
	double PlayModifier(double x, double Time, double framerate, int n);

	void LoadLocuses(LOCUSES Locuses) {
		Locus.clear();
		for (int i = 0; i < Locuses.Locus.size(); i++) {
			Locus.push_back(Locuses.Locus[i]);
		}
	}
};

struct CLOCUS {			//LocusesをAviUtl側の中間点の数分用意したもの
	std::vector<LOCUSES> Locuses;
	int LocusID = 0;
};

struct LOCUSDATA {		//CLocusのデータベース。
	std::vector<CLOCUS> CLocus;

	int NewCLocus(int SectionNum);
	int NewSetLocuses(int SectionNum, LOCUSES Locuses);
	int NewSetCLocus(int SectionNum, CLOCUS cLocus);
	void SetLocuses(int LocusID, int Section, LOCUSES Locuses);
	void SetAllLocuses(int LocusID, LOCUSES Locuses);

};

extern LOCUSDATA LocusData;

struct RANGE {
	std::map<std::wstring,D2D1_RECT_F> Range;
	int Type = 0;		//0=縦長, 1=横長

	void Update(D2D1_RECT_F Window, float DisplayRatio) {
		float Width = (Window.right - Window.left);
		float Height = (Window.bottom - Window.top);
		if (Width < Height) {
			Type = 0;
			Range[L"EffectsList"] = D2D1::RectF(0.0f, (Height - 30.0f) * DisplayRatio + 5.0f, Width, Height - 30.0f);
			Range[L"EffectsListHide"] = D2D1::RectF(Range[L"EffectsList"].left, Range[L"EffectsList"].bottom, Range[L"EffectsList"].right, Range[L"EffectsList"].bottom + 30.0f);
			Range[L"EffectsListRatio"] = D2D1::RectF(Range[L"EffectsList"].left, Range[L"EffectsList"].top - 5.0f, Range[L"EffectsList"].right, Range[L"EffectsList"].top);
			Range[L"Locus"] = D2D1::RectF(0.0f, 0.0f, Width, (Height - 30.0f) * DisplayRatio);
		}
		else {
			Type = 1;
			Range[L"EffectsList"] = D2D1::RectF((Width - 30.0f) * DisplayRatio + 5.0f, 0.0f, Width - 30.0f, Height);
			Range[L"EffectsListHide"] = D2D1::RectF(Range[L"EffectsList"].right, Range[L"EffectsList"].top, Range[L"EffectsList"].right + 30.0f, Range[L"EffectsList"].bottom);
			Range[L"EffectsListRatio"] = D2D1::RectF(Range[L"EffectsList"].left - 5.0f, Range[L"EffectsList"].top, Range[L"EffectsList"].left, Range[L"EffectsList"].bottom);
			Range[L"Locus"] = D2D1::RectF(0.0f, 0.0f, (Width - 30.0f) * DisplayRatio, Height);
		}
		Range[L"LocusEditor"] = D2D1::RectF(Range[L"Locus"].left, Range[L"Locus"].top + 40.0f, Range[L"Locus"].right, Range[L"Locus"].bottom);
		Range[L"LocusMenu"] = D2D1::RectF(Range[L"Locus"].left + 10.0f, Range[L"Locus"].top + 6.0f, Range[L"Locus"].right - 10.0f - 44.0f, Range[L"Locus"].top + 40.0f);
	}
};

struct CUR {
	float x = 0.0f;
	float y = 0.0f;
	float wheel = 0;
	bool click = false;
	bool dclick = false;
	bool clicking = false;
	bool rclick = false;
	bool rclicking = false;
	bool rdclick = false;
	bool mclick = false;
	bool mclicking = false;
	bool mdclick = false;
	D2D1_POINT_2F move = D2D1::Point2F(0.0f, 0.0f);
	bool moveing = false;
	bool action = false;
	RANGE range;
	HWND hwnd = nullptr;
	bool drag = false;
	bool drop = false;
	bool RectCheck(D2D1_RECT_F Rect) {
		if (Rect.left < x && x < Rect.right && Rect.top < y && y < Rect.bottom) {
			return true;
		}
		return false;
	}
	bool RangeCheck(std::wstring Name){
		if(range.Range[Name].left < x && x < range.Range[Name].right && range.Range[Name].top < y && y < range.Range[Name].bottom) {
			return true;
		}
		return false;
	}
};

struct POS_UI {
	float w = 0.0f;
	float h = 0.0f;
	bool Hover = false;
	bool Clicking = false;
};

struct LOCUS_UI {
	POS_UI S;			//絶対
	POS_UI H1;		//相対
	POS_UI H2;		//相対
	POS_UI F;			//絶対
};

class EDIT_LOCUSES {
public:
	int Tipe = 0;			//0=単体, 1=最初, 2=中間, 3=最後
	mutable int Mode = 0;			//0=ベジェ, 1=エラスティック, 2,バウンス
	mutable LOCUS Locus[4];
	mutable LOCUS_UI LocusUI[4];
	mutable bool Selected = false;
	mutable ComPtr<ID2D1PathGeometry> Geometry;
	mutable bool UseMode3 = false;
	// 描画用
	mutable D2D1_POINT_2F Center = {0,0};
	mutable float SizeX = 0;
	mutable float SizeY = 0;
	mutable float SizeXG = 0;
	mutable float SizeYG = 0;
	D2D1_RECT_F LRect = D2D1::RectF(0.0f,0.0f,0.0f,0.0f);		//このLocusのSとFの範囲(px絶対座標)
	mutable bool UpdateLocus = true;

	//曲線を更新する
	void UpdateGeometry(float size, D2D1_POINT_2F OllCenter) const;
	bool Update(D2D1_RECT_F Rect, CUR* Cursor, D2D1_POINT_2F OllCenter, float Size, const EDIT_LOCUSES* prevLocus, const EDIT_LOCUSES* nextLocus) const;
	void Draw(D2D1_RECT_F Rect, D2D1_POINT_2F OllCenter, float Size, const EDIT_LOCUSES* prevLocus, const EDIT_LOCUSES* nextLocus) const;
};

class EDITOR {
public:
	std::vector<EDIT_LOCUSES> Locus;
	float size = 1.0f;
	float sizeF = 1.0f;
	float x = 0.0f;
	float xF = 0.0f;
	float y = 0.0f;
	float yF = 0.0f;
	int ClickingLocus = -1;
	int SelectLocus = 0;
	bool Mclicking = false;

	bool UseCopy = false;
	LOCUS CopyLocus;
	//UI関連
	bool AnimeMoving = false;
	bool ModeMenuHover = false;
	bool ModeMenuClicking = false;
	float ModeMenuAnime = 1.0f;
	bool TurnHover = false;
	float TurnHoverAnime = 1.0f;
	bool ReturnSizeHide = true;
	float ReturnSizeHideAnime = 0.0f;
	bool ReturnSizeHover = false;
	float ReturnSizeHoverAnime = 0.0f;

	void GridDraw(D2D1_RECT_F Rect, float Size, D2D1_POINT_2F Center, int Grid);
	bool Update(CUR *Cursor, bool LocusUpdate);
	void Draw(CUR *Cursor);
	LOCUSES ToLocuses();
	void SetLocuses(LOCUSES locuses);
};

class P_Effect {
public:
	OBJECT_HANDLE ObjectHandle = nullptr;
	EFFECT_HANDLE EffectHandle = nullptr;
	std::string StrValue = "";

	std::wstring Name;         // エフェクト・項目名
	std::wstring Tipe;         // エフェクトか項目か
	mutable bool Hover = false;
	mutable float HoverAnime = 0.0f;
	mutable bool Open = false;  // 表示非表示
	mutable float OpenAnime = 0.0f;
	mutable bool Use = false;          // 軌跡が設定済みか
	mutable bool UseElse = false;		//別のトラックバー移動モードが設定されている。

	mutable int LocusID = 0;
	mutable bool InitialLocusID = false;	//IDが初期値の-1の時にtrue
	mutable bool ChangeLocus = false;		//"エディタの曲線を適用"チェックボックスが1の時にtrue
	mutable int SectionNum = 1;
	mutable std::vector<float> Value;
	mutable float maxValue = 0.0f, minValue = 0.0f;

	mutable std::vector<float> FrameS;
	mutable std::vector<float> FrameF;
	mutable std::vector<float> SectionWidth;

	mutable std::vector<ComPtr<ID2D1PathGeometry>> Geometry;
	mutable std::vector<bool> LocusHover;
	mutable std::vector<float> LocusHoverAnime;

	mutable float PageX = 0.0;
	mutable float PageXF = 0.0;
	mutable float XRange = 0.0;
	mutable bool click = false;
	mutable bool drag = false;

	// フォールバック用の識別キー（インデックス + 親名 + 自身の種別 + 名前）
	std::wstring GetKey(int effectIndex, const std::wstring& parentName = L"") const {
		return std::to_wstring(effectIndex) + L"::" + parentName + L"::" + Tipe + L"::" + Name;
	}

	void UpdateGeometry(LOCUSES Locus, D2D1_RECT_F Rect, int Section, float normS, float normE) const;
	bool Update(D2D1_RECT_F rect, CUR *Cursor) const;
	bool Locus_Update(D2D1_RECT_F Rect, CUR *Cursor) const;
	void Draw(D2D1_RECT_F Rect, CUR* Cursor) const;
	void Locus_Draw(D2D1_RECT_F Rect, CUR* Cursor) const;
	void SetTrackbar(int LocusID) const;
	void DeleteTrackbar() const;
	void ChangeIDTrackbar(int NewLocusID) const;
};

class P_Effects {
public:
	OBJECT_HANDLE ObjectHandle;
	std::vector<P_Effect> Effect;
	std::vector<CLOCUS> Locus;
	float PageY = 0.0f;
	float PageYF = 0.0f;
	bool AnimeMoving = false;

	int CopyID = -1;
	CLOCUS CopyCLocus;
	bool UseCopyCLocus = false;
	LOCUSES CopyLocuses;
	bool UseCopyLocuses = false;

	float YRange = 0;

	bool Hide = false;
	float HideAnime = 0.0f;
	bool HideButtonHover = false;
	float HideButtonHoverAnime = 0.0f;

	float DisplayRatio = 0.5f;		//エディタとエフェクト一覧の割合
	bool RatioHover = false;
	float RatioHoverAnime = 0.0f;
	bool RatioClicking = false;

	void GetObjectEffects();
	bool Update(CUR *Cursor);
	void Draw(CUR *Cursor);
};

extern EDITOR Editor;
extern P_Effects Effects;
extern CUR cursor;
extern LOCUSDATA LocusData;
