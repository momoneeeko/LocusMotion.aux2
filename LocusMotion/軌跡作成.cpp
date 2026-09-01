#include "Direct2D.h"
#include "GUI.h"
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#define PI 3.14159265359

double Easing(double t, int Mode) {
    switch (Mode) {
    case 0:
        return -cos(PI / 2.0 * t) + 1.0;
    case 1:
        return pow(t, 2.0);
    case 2:
        return pow(t, 3.0);
    case 3:
        return pow(t, 4.0);
    case 4:
        return pow(t, 5.0);
    case 5:
        return pow(2, 10 * (t - 1.0));
    case 6:
        return 1.0 - sqrt(1.0 - pow(t, 2.0));
    case 7:
        return -pow(2, 10.0 * (t - 1.0)) * sin((t * 10.0 - 10.75) * (2.0 * PI / 3.0));
    case 8:
    {
        double s = 1.70158;
        return pow(t, 2.0) * ((s + 1.0) * t - s);
    }
    case 9:
    {
        double w = 7.5625;
        if (1.0 - t < 4.0 / 11.0) {
            return 1.0 - (w * pow(1.0 - t, 2.0));
        }
        else if (1.0 - t < 8.0 / 11.0) {
            return 1.0 - (w * pow(1.0 - t - 6.0 / 11.0, 2.0) + 3.0 / 4.0);
        }
        else if (1.0 - t < 10.0 / 11.0) {
            return 1.0 - (w * pow(1.0 - t - 9.0 / 11.0, 2.0) + 15.0 / 16.0);
        }
        else {
            return 1.0 - (w * pow(1.0 - t - 21.0 / 22.0, 2.0) + 63.0 / 64.0);
        }
    }
    }
}

double LOCUS::LocusToValue(double Time, int n) {
    if (Turn) Time = 1.0f - Time;

    double out = 0.0;
    if (Mode == 0) {			//ベジェ曲線
        double t0 = 0;
        double t1 = 1;
        double t = 0;
        double u;
        for (int i = 0; i < n; i++) {
            t = (t0 + t1) / 2;
            u = 1 - t;
            if (Time > (3 * u * u * t * H1.x) + (3 * u * t * t * H2.x) + t * t * t * 1) {
                t0 = t;
            }
            else {
                t1 = t;
            }
        }
        u = 1 - t;
        out = (3 * u * u * t * H1.y) + (3 * u * t * t * H2.y) + (t * t * t);
    }
    else if (Mode == 1) {		//エラスティック
        double f = 1 / H2.x - 1.0;     
        double damping = pow(100.0, (H2.y));          //減衰係数
        double power = (1 - exp((1 - Time) * damping)) / (exp(damping) - 1);
        out = ((cos(Time * f * 2.0 * PI) + (tan(H1.y * 100.0 / 180.0 * PI) / -2.0 / (f * 2.0) * sin(Time * f * 2.0 * PI))) * power + 1.0);
    }
    else if (Mode == 2) {       //バウンス
        double newH2_y = 1.0 - H2.y;
        double v0 = tan(H1.y * 100.0 / 180.0 * PI);    //初速度
        if (v0 > H2.y / H2.x) {
            v0 = H2.y / H2.x;
        }
        double FPt = 0.0;

        if (std::abs(v0) < 1e-6) {
            FPt = H2.x / (1.0 + std::sqrt(newH2_y));
        }
        else {
            double A = v0;
            double B = newH2_y - 1.0 - 2.0 * v0 * H2.x;
            double C = 2.0 * H2.x + v0 * H2.x * H2.x;
            double D = -H2.x * H2.x;

            double p = (3.0 * A * C - B * B) / (3.0 * A * A);
            double q = (2.0 * B * B * B - 9.0 * A * B * C + 27.0 * A * A * D) / (27.0 * A * A * A);
            double disc = (q * q / 4.0) + (p * p * p / 27.0); // 判別式

            if (disc > 0.0) {
                double u = std::cbrt(-q / 2.0 + std::sqrt(disc));
                double v = std::cbrt(-q / 2.0 - std::sqrt(disc));
                FPt = u + v - (B / (3.0f * A));
            }
            else {
                double r_val = std::sqrt(-(p * p * p) / 27.0);
                double phi = std::acos((std::max)(-1.0, (std::min)(1.0, -q / (2.0 * r_val))));
                double root1 = 2.0 * std::cbrt(r_val) * std::cos(phi / 3.0) - (B / (3.0 * A));
                double root2 = 2.0 * std::cbrt(r_val) * std::cos((phi + 2.0 * PI) / 3.0) - (B / (3.0 * A));
                double root3 = 2.0 * std::cbrt(r_val) * std::cos((phi + 4.0 * PI) / 3.0) - (B / (3.0 * A));

                if (root1 > 0.0 && root1 <= H2.x) FPt = root1;
                else if (root2 > 0.0 && root2 <= H2.x) FPt = root2;
                else FPt = root3;
            }
        }

        double a = (2.0 - 2.0 * v0 * FPt) / (FPt * FPt);    //加速度

        double top_x = v0 / a * -1.0;  //最初の放物線の頂点x
        double top_y = top_x * (v0 + a * top_x / 2.0);     //最初の放物線の頂点y

        double datax_0 = FPt + (2.0 * std::sqrt(2.0 / a) * std::sqrt(newH2_y));
        double tmp1 = newH2_y / (1.0 - top_y);
        double tmp2 = 2.0 * std::sqrt(2.0f * (1.0 - top_y) / a);


        if (FPt > Time) {
            if (1.0f > FPt) {
                out = v0 * Time + a * Time * Time / 2.0;
            }
            else {
                out = 1.0;
            }
        }
        else if (datax_0 > Time) {
            if (1.0 > datax_0) {
                out = a * std::pow(Time - H2.x, 2.0) / 2.0 + H2.y;
            }
            else {
                out = 1.0;
            }
        }
        else {
            double r = std::sqrt(tmp1);

            if (r < 1e-5) {
                out = 1.0;
            }
            else {
                if (r >= 0.999) r = 0.999;
                double C_val = tmp2 / (1.0 - top_x);

                double V = 1.0 - (Time - datax_0) * (1.0 - r) / (C_val * r * r);

                if (V <= 0.0) {
                    out = 1.0;
                }
                else {
                    double n_float = std::log(V) / std::log(r);
                    int n = (int)std::floor(n_float) + 1;

                    if (n > 300) {
                        out = 1.0;
                    }
                    else {
                        int i_val = n + 1;

                        double datax_prev = (n == 1) ? datax_0 : datax_0 + C_val * r * r * (1.0 - std::pow(r, n - 1)) / (1.0 - r);
                        double datax_curr = datax_0 + C_val * r * r * (1.0 - std::pow(r, n)) / (1.0 - r);
                        double datay_curr = (1.0 - top_y) * std::pow(tmp1, i_val);

                        if (1.0 > datax_curr) {
                            double tmp1_val = Time - (datax_prev + datax_curr) / 2.0;
                            out = a * tmp1_val * tmp1_val / 2.0 + (1.0 - datay_curr);
                        }
                        else {
                            out = 1.0;
                        }
                    }
                }
            }
        }

        if (out >= 1.0) {
            out = 1.0;
        }
    }
    else if (Mode == 3) {
        int type = (int)H1.y;
        if (type == 1) {
            out = Time;
        }
        else {
            int EaseMode = (type - 2) / 4;
            int InOut = (type - 2) % 4;
            if (InOut == 0) {
                out = Easing(Time, EaseMode);
            }
            else if (InOut == 1) {
                out = 1.0 - Easing(1.0 - Time, EaseMode);
            }
            else if (InOut == 2) {
                if (Time < 0.5) {
                    out = Easing(Time * 2.0, EaseMode) / 2.0;
                }
                else {
                    out = (1.0 - Easing(1.0 - ((Time - 0.5) * 2.0), EaseMode)) / 2.0 + 0.5;
                }
            }
            else if (InOut == 3) {
                if (Time < 0.5) {
                    out = (1.0 - Easing(1.0 - (Time * 2.0), EaseMode)) / 2.0;
                }
                else {
                    out = Easing((Time - 0.5) * 2.0, EaseMode) / 2.0 + 0.5;
                }
            }
        }
    }
    if (Turn)out = 1.0 - out;
    return out;
}

//追加したいモディファイア
// ☆コマ落ち
// ☆離散化
// ・直線化 (一定間隔で直線にする)
// ・ループ (0~1を(回数)繰り返す)
// ・反転ループ (0~1,1~0を(回数)繰り返す)
// ・段々ループ
// ・速度を取得 (曲線の傾きを取得)
// ・波を合成 (正弦波やランダムなどを合成)

double LOCUSES::LocusesToValue(double Time, int n) {
    double out = 0.0f;
    for (size_t i = 0; i < Locus.size(); i++) {
        if (Locus[i].S.x <= Time && Time <= Locus[i].F.x) {
            float duration = Locus[i].F.x - Locus[i].S.x;
            if (duration > 0.0001f) {
                double localTime = (Time - Locus[i].S.x) / duration;
                out = Locus[i].LocusToValue(localTime, n) * (Locus[i].F.y - Locus[i].S.y) + Locus[i].S.y;
            }
            else {
                out = Locus[i].LocusToValue(0.0f, n);
            }
        }
    }
    return out;
}

double LOCUSES::PlayModifier(double x, double Time, double framerate, int n) {
    double t = x;
    double out = 0.0;
    double Trans = 0.1;
    //前半モディファイア
    for (int i = 0; i < Modifier.size(); i++) {
        switch (Modifier[i].Mode) {
        case 1: {   //コマ落ち
            double v = Modifier[i].ChengeUnit((int)Modifier[i].Param[1], 0, Modifier[i].Param[0]); //間隔
            double p = Modifier[i].ChengeUnit((int)Modifier[i].Param[3], 0, Modifier[i].Param[2]);
            t = floor(x * (Time / v) - p) / (Time / v) + p;
            break;
        }
        }
    }

    // Timeから数値を計算
    out = LocusesToValue(t, n);

    //後半モディファイア
    for (int i = 0; i < Modifier.size(); i++) {
        switch (Modifier[i].Mode) {
        case 2: {   //離散化
            if (Modifier[i].Param[1] == 0.0) {
                out = floor(out * Modifier[i].Param[0]) / Modifier[i].Param[0];
            }
            else if (Modifier[i].Param[1] == 1.0) {
                out = round(out * Modifier[i].Param[0]) / Modifier[i].Param[0];
            }
            else {
                out = ceil(out * Modifier[i].Param[0]) / Modifier[i].Param[0];
            }
            break;
        }
        case 3: {   //速度化
            if (t - Modifier[i].Param[0] / 100.0 / 2.0 <= 0.0) {
                out = (LocusesToValue(t + Modifier[i].Param[0] / 100.0 / 2.0, n) - LocusesToValue(t, n)) / (Modifier[i].Param[0] / 100.0 / 2.0);
            }
            else if (t + Modifier[i].Param[0] / 100 / 2.0 >= 1.0) {
                out = (LocusesToValue(t, n) - LocusesToValue(t - Modifier[i].Param[0] / 100.0 / 2.0, n)) / (Modifier[i].Param[0] / 100.0 / 2.0);
            }
            else {
                out = (LocusesToValue(t + Modifier[i].Param[0] / 100.0 / 2.0, n) - LocusesToValue(t - Modifier[i].Param[0] / 100.0 / 2.0, n)) / (Modifier[i].Param[0] / 100.0);
            }
            break;
        }
        }
    }
    return out;
}

int LOCUSDATA::NewCLocus(int SectionNum) {
    CLOCUS NewCLocus;
    LOCUSES NewLocuses;
    LOCUS NewLocus;
    NewLocus.Mode = 0;
    NewLocus.Turn = false;
    NewLocus.S = { 0.0f, 0.0f };
    NewLocus.H1 = { 0.3f, 0.3f };
    NewLocus.H2 = { 0.7f, 0.7f };
    NewLocus.F = { 1.0f, 1.0f };
    NewLocuses.Locus.push_back(NewLocus);

    for (int i = 0; i < SectionNum; i++) {
        NewCLocus.Locuses.push_back(NewLocuses);
    }

    int new_id = static_cast<int>(CLocus.size());
    NewCLocus.LocusID = new_id;
    CLocus.push_back(NewCLocus);
    return new_id;
}

inline bool operator==(const POS& a, const POS& b) {
    return std::abs(a.x - b.x) < 0.00001f && std::abs(a.y - b.y) < 0.00001f;
}

inline bool operator==(const LOCUS& a, const LOCUS& b) {
    return a.Mode == b.Mode &&
        a.Turn == b.Turn &&
        a.S == b.S &&
        a.H1 == b.H1 &&
        a.H2 == b.H2 &&
        a.F == b.F;
}

inline bool operator==(const LOCUSES& a, const LOCUSES& b) {
    if (a.Locus.size() != b.Locus.size()) return false;
    for (size_t i = 0; i < a.Locus.size(); ++i) {
        if (!(a.Locus[i] == b.Locus[i])) return false;
    }
    return true;
}

inline bool operator==(const CLOCUS& a, const CLOCUS& b) {
    if (a.Locuses.size() != b.Locuses.size()) return false;
    for (size_t i = 0; i < a.Locuses.size(); ++i) {
        if (!(a.Locuses[i] == b.Locuses[i])) return false;
    }
    return true;
}

int LOCUSDATA::NewSetLocuses(int SectionNum, LOCUSES Locuses) {
    CLOCUS target;
    target.Locuses.assign(SectionNum, Locuses);
    target.LocusID = static_cast<int>(CLocus.size());
    CLocus.push_back(target);
    return target.LocusID;
}

int LOCUSDATA::NewSetCLocus(int SectionNum, CLOCUS cLocus) {
    CLOCUS target = cLocus;
    target.LocusID = static_cast<int>(CLocus.size());
    CLocus.push_back(target);
    return target.LocusID;
}

void LOCUSDATA::SetLocuses(int LocusID, int Section, LOCUSES Locuses) {
    CLocus[LocusID].Locuses[Section] = Locuses;
}

void LOCUSDATA::SetAllLocuses(int LocusID, LOCUSES Locuses) {
    if (LocusID < 0 || LocusID >= static_cast<int>(CLocus.size())) {
        return;
    }
    for (size_t i = 0; i < CLocus[LocusID].Locuses.size(); i++) {
        CLocus[LocusID].Locuses[i].LoadLocuses(Locuses);
    }
}

void P_Effect::UpdateGeometry(LOCUSES Locus, D2D1_RECT_F Rect, int Section, float normS, float normE) const {
    if (Section < 0 || Section >= static_cast<int>(Geometry.size())) return;

    std::vector<D2D1_POINT_2F> Samples;
    float width = Rect.right - Rect.left;
    float height = Rect.bottom - Rect.top;

    if (width <= 0.0f || height <= 0.0f) return;

    float n = (std::max)(width, 10.0f);
    for (int i = 0; i <= (int)n; i++) {
        float x = (float)i / n;
        EDIT_INFO info{};
        edit_handle->get_edit_info(&info, sizeof(EDIT_INFO));
        double framerate = static_cast<double>(info.rate) / info.scale;
        float locusVal = (float)Locus.PlayModifier(x, (FrameF[0] - FrameS[0]) / framerate, framerate, 20);
        float currentNorm = normS + (normE - normS) * locusVal;
        float drawX = Rect.left + x * width;
        float drawY = Rect.bottom - currentNorm * height;

        Samples.push_back(D2D1::Point2(drawX, drawY));
    }

    if (Samples.empty()) return;

    Geometry[Section].Reset();
    HRESULT hr = g_pD2DFactory->CreatePathGeometry(&Geometry[Section]);
    if (FAILED(hr)) return;

    ComPtr<ID2D1GeometrySink> sink;
    hr = Geometry[Section]->Open(&sink);
    if (SUCCEEDED(hr)) {
        sink->BeginFigure(Samples[0], D2D1_FIGURE_BEGIN_HOLLOW);
        if (Samples.size() > 1) {
            sink->AddLines(Samples.data() + 1, static_cast<UINT32>(Samples.size() - 1));
        }
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        sink->Close();
    }
}

void EDIT_LOCUSES::UpdateGeometry(float size, D2D1_POINT_2F OllCenter) const {
    if (size <= 0) return;

    std::vector<D2D1_POINT_2F>Samples;
    float n = (std::min)((std::max)((abs(Locus[Mode].F.x - Locus[Mode].S.x) * size), 10.0f), 1000.0f) * 3.0f;
    for (int i = 0; i <= (int)n; i++) {
        float x = i / n;
        float y = (float)Locus[Mode].LocusToValue(x, 20);
        Samples.push_back(D2D1::Point2((x * 2.0f - 1.0f) * SizeXG + Center.x, (y * -2.0f + 1.0f) * SizeYG + Center.y));
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

LOCUSES EDITOR::ToLocuses() {
    LOCUSES OutLocuses;
    for (size_t i = 0; i < Locus.size(); i++) {
        OutLocuses.Locus.push_back(Locus[i].Locus[Locus[i].Mode]);
    }
    return OutLocuses;
}

void EDITOR::SetLocuses(LOCUSES Locuses) {
    Locus.clear();
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
    for (int i = 0;i < Locuses.Locus.size(); i++) {
        EDIT_LOCUSES newLocus = defaultLocus;
        newLocus.Locus[Locuses.Locus[i].Mode] = Locuses.Locus[i];
        if (Locuses.Locus.size() == 1) {
            newLocus.Tipe = 0;
        }
        else {
            if (i == 0) {
                newLocus.Tipe = 1;
            }
            else if (i == Locuses.Locus.size() - 1) {
                newLocus.Tipe = 3;
            }
            else {
                newLocus.Tipe = 2;
            }
        }
        newLocus.Mode = Locuses.Locus[i].Mode;
        Locus.push_back(newLocus);
    }
    SelectLocus = 0;
}
