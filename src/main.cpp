#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "date.hpp"
#include "event_store.hpp"
#include "autostart.hpp"

#include <ctime>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cfloat>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

// ---------------- Paleta ----------------
static const ImVec4 C_BG = ImVec4(0.031f, 0.031f, 0.031f, 1.00f);
static const ImVec4 C_PANEL = ImVec4(0.075f, 0.075f, 0.075f, 1.00f);
static const ImVec4 C_PANEL2 = ImVec4(0.118f, 0.118f, 0.118f, 1.00f);
static const ImVec4 C_BORDER = ImVec4(0.165f, 0.165f, 0.165f, 1.00f);
static const ImVec4 C_TEXT = ImVec4(0.920f, 0.920f, 0.920f, 1.00f);
static const ImVec4 C_TEXT_DIM = ImVec4(0.490f, 0.490f, 0.490f, 1.00f);
static const ImVec4 C_ACCENT = ImVec4(0.965f, 0.784f, 0.176f, 1.00f);
static const ImVec4 C_BTN = ImVec4(0.145f, 0.145f, 0.145f, 1.00f);
static const ImVec4 C_RED = ImVec4(0.850f, 0.310f, 0.310f, 1.00f);

static const ImU32 U32_BORDER = IM_COL32(42, 42, 42, 255);
static const ImU32 U32_ACCENT = IM_COL32(246, 200, 45, 255);
static const ImU32 U32_TEXT = IM_COL32(235, 235, 235, 255);
static const ImU32 U32_TEXT_DIM = IM_COL32(125, 125, 125, 255);
static const ImU32 U32_TEXT_DIM2 = IM_COL32(80, 80, 80, 255);
static const ImU32 U32_RED = IM_COL32(220, 80, 80, 255);
static const ImU32 U32_BLACK = IM_COL32(0, 0, 0, 255);

// ---------------- Estado ----------------
static EventStore g_store;
static Date       g_today, g_selected;
static Date       g_view{ 2026, 1, 1 };

static ImFont* g_fontBig = nullptr;

// Notas
static char        g_notesBuf[4096] = "";
static bool        g_notesDirty = false;

// Popup creación rápida
static Date        g_popupDate;
static char        g_popupTime[8] = "";
static char        g_popupText[256] = "";
static bool        g_popupRequested = false;

// Ajustes
static bool        g_showSettings = false;
static bool        g_autostart = false;

static const char* MONTHS[] = {
    "January","February","March","April","May","June",
    "July","August","September","October","November","December"
};
static const char* DAYS_LONG[] = {
    "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

// ---------------- Rutas ----------------
static fs::path getDataDir() {
    fs::path dir;
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    dir = appdata ? fs::path(appdata) / "MinCalendar" : fs::current_path();
#else
    const char* home = std::getenv("HOME");
    dir = home ? fs::path(home) / ".local" / "share" / "minicalendar"
        : fs::current_path();
#endif
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}
static std::string getEventsPath() { return (getDataDir() / "events.db").string(); }
static std::string getNotesPath() { return (getDataDir() / "notes.txt").string(); }

// ---------------- Notas ----------------
static void loadNotes() {
    std::ifstream f(getNotesPath(), std::ios::binary);
    if (!f) return;
    std::stringstream ss; ss << f.rdbuf();
    std::string s = ss.str();
    if (s.size() >= sizeof(g_notesBuf)) s.resize(sizeof(g_notesBuf) - 1);
    std::memcpy(g_notesBuf, s.data(), s.size());
    g_notesBuf[s.size()] = 0;
}
static void saveNotes() {
    std::ofstream f(getNotesPath(), std::ios::binary | std::ios::trunc);
    if (!f) return;
    f.write(g_notesBuf, std::strlen(g_notesBuf));
    g_notesDirty = false;
}

// ---------------- Fechas ----------------
static void initDates() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    g_today = { tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday };
    g_selected = g_today;
    g_view = { g_today.y, g_today.m, 1 };
}

// ---------------- Fuentes / Estilo ----------------
static void loadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    const char* regular = nullptr;
#ifdef _WIN32
    if (fs::exists("C:/Windows/Fonts/segoeui.ttf"))   regular = "C:/Windows/Fonts/segoeui.ttf";
    else if (fs::exists("C:/Windows/Fonts/arial.ttf")) regular = "C:/Windows/Fonts/arial.ttf";
#else
    const char* cands[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        nullptr
    };
    for (int i = 0; cands[i]; ++i)
        if (fs::exists(cands[i])) { regular = cands[i]; break; }
#endif

    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 1;
    cfg.PixelSnapH = false;

    if (regular) {
        ImFont* base = io.Fonts->AddFontFromFileTTF(regular, 15.0f, &cfg);
        g_fontBig = io.Fonts->AddFontFromFileTTF(regular, 52.0f, &cfg);
        io.FontDefault = base;
    }
    else {
        io.Fonts->AddFontDefault();
        g_fontBig = io.Fonts->Fonts[0];
    }
}

static void applyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0;
    s.ChildRounding = 10;
    s.FrameRounding = 6;
    s.PopupRounding = 8;
    s.ScrollbarRounding = 8;
    s.GrabRounding = 6;
    s.WindowPadding = ImVec2(0, 0);
    s.FramePadding = ImVec2(10, 6);
    s.ItemSpacing = ImVec2(8, 8);
    s.ItemInnerSpacing = ImVec2(6, 6);
    s.ScrollbarSize = 8;
    s.WindowBorderSize = 0;
    s.ChildBorderSize = 1;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = C_BG;
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = C_PANEL2;
    c[ImGuiCol_Border] = C_BORDER;
    c[ImGuiCol_FrameBg] = C_PANEL2;
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    c[ImGuiCol_Button] = C_BTN;
    c[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
    c[ImGuiCol_Text] = C_TEXT;
    c[ImGuiCol_TextDisabled] = C_TEXT_DIM;
    c[ImGuiCol_Separator] = C_BORDER;
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    c[ImGuiCol_CheckMark] = C_ACCENT;
    c[ImGuiCol_Header] = C_PANEL2;
    c[ImGuiCol_HeaderHovered] = C_BTN;
    c[ImGuiCol_HeaderActive] = C_BTN;
}

// ---------------- Helpers de UI ----------------
static void cardBegin(const char* id, float h = 0.0f) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, C_PANEL);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
    ImGuiChildFlags f = ImGuiChildFlags_Border;
    if (h == 0.0f) f |= ImGuiChildFlags_AutoResizeY;
    ImGui::BeginChild(id, ImVec2(0, h), f, ImGuiWindowFlags_NoScrollbar);
}
static void cardEnd() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

static bool toggleBtn(const char* label, bool active, float w) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, C_ACCENT);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.06f, 0.06f, 0.06f, 1.0f));
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
    }
    bool c = ImGui::Button(label, ImVec2(w, 0));
    ImGui::PopStyleColor(2);
    return c;
}

static bool navBtn(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
    bool c = ImGui::Button(label, ImVec2(26, 26));
    ImGui::PopStyleColor(2);
    return c;
}

// ---------------- Sidebar ----------------
static void drawSidebar(float width) {
    ImGui::BeginChild("##sidebar", ImVec2(width, 0), false,
        ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));

    // Cabecera
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
    ImGui::TextUnformatted("General");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 12);

    ImGui::PushStyleColor(ImGuiCol_Button, C_ACCENT);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.06f, 0.06f, 0.06f, 1.0f));
    if (ImGui::Button("+ Create", ImVec2(0, 26))) {
        g_popupDate = g_selected;
        g_popupTime[0] = 0;
        g_popupText[0] = 0;
        g_popupRequested = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine(0, 6);
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
    if (ImGui::Button("Today", ImVec2(0, 26))) {
        g_view = { g_today.y, g_today.m, 1 };
        g_selected = g_today;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (navBtn("<##sPrev")) { if (--g_view.m == 0) { g_view.m = 12; g_view.y--; } }
    ImGui::SameLine(0, 2);
    if (navBtn(">##sNext")) { if (++g_view.m == 13) { g_view.m = 1; g_view.y++; } }

    ImGui::Dummy({ 0, 8 });

    // ---- Tarjeta reloj ----
    cardBegin("##clock", 130.0f);
    {
        std::time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        char timeBuf[8];
        std::snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", tm->tm_hour, tm->tm_min);

        ImGui::PushFont(g_fontBig);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
        ImGui::TextUnformatted(timeBuf);
        ImGui::PopStyleColor();
        ImGui::PopFont();

        char dateBuf[64];
        std::snprintf(dateBuf, sizeof(dateBuf), "%s, %s %d",
            DAYS_LONG[tm->tm_wday], MONTHS[tm->tm_mon], tm->tm_mday);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
        ImGui::TextUnformatted(dateBuf);
        ImGui::PopStyleColor();

        ImGui::Dummy({ 0, 4 });
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(
            p, ImVec2(p.x + 2, p.y + 16), U32_ACCENT);
        ImGui::Dummy({ 8, 0 });
        ImGui::SameLine(0, 6);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
        ImGui::TextUnformatted("Free right now");
        ImGui::PopStyleColor();
    }
    cardEnd();
    ImGui::Dummy({ 0, 10 });

    // ---- Tarjeta Today ----
    cardBegin("##today", 130.0f);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
        ImGui::TextUnformatted("Today");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        int count = 0;
        if (const auto* v = g_store.find(g_today)) count = (int)v->size();
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
        ImGui::Text("· %d", count);
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetWindowWidth() - 34);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
        if (ImGui::Button("+##tadd", ImVec2(22, 22))) {
            g_popupDate = g_today;
            g_popupTime[0] = 0;
            g_popupText[0] = 0;
            g_popupRequested = true;
        }
        ImGui::PopStyleColor(2);

        ImGui::Dummy({ 0, 4 });

        const auto* evs = g_store.find(g_today);
        if (!evs || evs->empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
            ImGui::TextUnformatted("Nothing scheduled");
            ImGui::PopStyleColor();
        }
        else {
            for (size_t i = 0; i < evs->size(); ++i) {
                const auto& e = (*evs)[i];
                ImGui::TextColored(C_ACCENT, "•");
                ImGui::SameLine(0, 6);
                ImGui::Text("%s  %s", e.time.c_str(), e.text.c_str());
            }
        }
    }
    cardEnd();
    ImGui::Dummy({ 0, 10 });

    // ---- Subjects ----
    cardBegin("##subjects", 130.0f);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
        ImGui::TextUnformatted("Subjects");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
        ImGui::Text("· %04d/%d", g_today.y, (g_today.m - 1) / 6 + 1);
        ImGui::PopStyleColor();

        ImGui::Dummy({ 0, 6 });

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(p.x, p.y + 4), ImVec2(p.x + 10, p.y + 14), U32_RED, 2.0f);
        ImGui::Dummy({ 14, 14 });
        ImGui::SameLine(0, 6);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
        ImGui::TextUnformatted("General");
        ImGui::PopStyleColor();
    }
    cardEnd();
    ImGui::Dummy({ 0, 10 });

    // ---- Notes (funcional) ----
    float notesH = 200.0f;
    cardBegin("##notes", notesH + 60.0f);
    {
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
        ImGui::TextUnformatted("Notes");
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetWindowWidth() - 34);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
        if (ImGui::Button("+##nadd", ImVec2(22, 22))) {
            size_t l = std::strlen(g_notesBuf);
            if (l + 2 < sizeof(g_notesBuf)) {
                if (l > 0 && g_notesBuf[l - 1] != '\n') { g_notesBuf[l++] = '\n'; }
                g_notesBuf[l++] = '-';
                g_notesBuf[l++] = ' ';
                g_notesBuf[l] = 0;
                g_notesDirty = true;
            }
        }
        ImGui::PopStyleColor(2);

        ImGui::Dummy({ 0, 4 });

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        if (ImGui::InputTextMultiline("##notesedit", g_notesBuf, sizeof(g_notesBuf),
            ImVec2(-FLT_MIN, notesH),
            ImGuiInputTextFlags_AllowTabInput))
            g_notesDirty = true;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        if (ImGui::IsItemDeactivatedAfterEdit() && g_notesDirty)
            saveNotes();
    }
    cardEnd();

    ImGui::PopStyleVar();
    ImGui::EndChild();
}

// ---------------- Popup de evento ----------------
static void drawEventPopup() {
    if (ImGui::BeginPopupModal("Nuevo evento", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Fecha: %s", g_popupDate.iso().c_str());
        ImGui::Dummy({ 0, 4 });

        ImGui::SetNextItemWidth(70);
        ImGui::InputTextWithHint("##ptime", "HH:MM", g_popupTime, sizeof(g_popupTime));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(280);
        ImGui::InputTextWithHint("##ptext", "Descripción", g_popupText, sizeof(g_popupText));

        ImGui::Dummy({ 0, 6 });

        bool canSave = g_popupText[0] != 0;

        if (!canSave) ImGui::BeginDisabled();
        if (ImGui::Button("Guardar", ImVec2(120, 30))) {
            std::string t = g_popupTime[0] ? g_popupTime : "--:--";
            g_store.forDate(g_popupDate).push_back({ t, g_popupText });
            g_store.save(getEventsPath());
            g_popupTime[0] = 0;
            g_popupText[0] = 0;
            ImGui::CloseCurrentPopup();
        }
        if (!canSave) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(120, 30))) {
            g_popupTime[0] = 0;
            g_popupText[0] = 0;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// ---------------- Cuadrícula del mes ----------------
static void drawCalendarGrid() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    float header_h = 34.0f;
    float col_w = avail.x / 7.0f;
    float cell_h = (avail.y - header_h) / 6.0f;

    dl->AddRectFilled(p0, ImVec2(p0.x + avail.x, p0.y + header_h),
        IM_COL32(18, 18, 18, 255));

    const char* names[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
    for (int i = 0; i < 7; ++i) {
        ImVec2 ts = ImGui::CalcTextSize(names[i]);
        float x = p0.x + i * col_w + (col_w - ts.x) * 0.5f;
        float y = p0.y + (header_h - ts.y) * 0.5f;
        dl->AddText(ImVec2(x, y), U32_TEXT, names[i]);
    }

    for (int r = 1; r <= 6; ++r) {
        float y = p0.y + header_h + r * cell_h;
        dl->AddLine(ImVec2(p0.x, y), ImVec2(p0.x + avail.x, y), U32_BORDER);
    }
    for (int c = 1; c < 7; ++c) {
        float x = p0.x + c * col_w;
        dl->AddLine(ImVec2(x, p0.y + header_h),
            ImVec2(x, p0.y + avail.y), U32_BORDER);
    }
    dl->AddRect(p0, ImVec2(p0.x + avail.x, p0.y + avail.y), U32_BORDER);

    int first = Date::weekday(g_view.y, g_view.m, 1);
    int total = Date::daysInMonth(g_view.y, g_view.m);
    Date prev = g_view; if (--prev.m == 0) { prev.m = 12; prev.y--; }
    int  prevTotal = Date::daysInMonth(prev.y, prev.m);
    Date nxt = g_view; if (++nxt.m == 13) { nxt.m = 1; nxt.y++; }

    int d = 1, nd = 1;
    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 7; ++c) {
            int idx = r * 7 + c;
            float cx = p0.x + c * col_w;
            float cy = p0.y + header_h + r * cell_h;

            Date dt; bool cur;
            if (idx < first) {
                dt = { prev.y, prev.m, prevTotal - first + idx + 1 }; cur = false;
            }
            else if (d > total) {
                dt = { nxt.y, nxt.m, nd++ }; cur = false;
            }
            else {
                dt = { g_view.y, g_view.m, d++ }; cur = true;
            }

            char b[8]; std::snprintf(b, sizeof(b), "%d", dt.d);
            ImVec2 ts = ImGui::CalcTextSize(b);

            bool isToday = (dt == g_today);
            bool isSel = (dt == g_selected);

            if (isToday) {
                float rad = 14.0f;
                ImVec2 center = ImVec2(cx + 18 + rad - 4, cy + 18 + rad - 4);
                dl->AddCircleFilled(center, rad, U32_ACCENT, 32);
                dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f),
                    U32_BLACK, b);
            }
            else if (!cur) {
                dl->AddText(ImVec2(cx + 14, cy + 10), U32_TEXT_DIM2, b);
            }
            else {
                ImU32 col = isSel ? U32_ACCENT : U32_TEXT;
                dl->AddText(ImVec2(cx + 14, cy + 10), col, b);
                if (isSel) {
                    dl->AddRect(ImVec2(cx + 1, cy + 1),
                        ImVec2(cx + col_w - 1, cy + cell_h - 1),
                        U32_ACCENT, 6.0f, 0, 1.5f);
                }
            }

            if (cur) {
                const auto* evs = g_store.find(dt);
                if (evs && !evs->empty()) {
                    float ey = cy + 40.0f;
                    for (size_t i = 0; i < evs->size() && i < 4; ++i) {
                        const auto& e = (*evs)[i];
                        dl->AddRectFilled(ImVec2(cx + 10, ey + 3),
                            ImVec2(cx + 16, ey + 9), U32_RED, 1.0f);
                        char tb[128];
                        if (e.time == "--:--")
                            std::snprintf(tb, sizeof(tb), "%s", e.text.c_str());
                        else
                            std::snprintf(tb, sizeof(tb), "%s %s",
                                e.time.c_str(), e.text.c_str());
                        dl->AddText(ImVec2(cx + 22, ey), U32_TEXT_DIM, tb);
                        ey += 16.0f;
                    }
                }
            }
        }
    }

    // Invisible button captura hover y clicks sobre toda la cuadrícula
    ImGui::InvisibleButton("##grid", avail);

    if (ImGui::IsItemHovered()
        && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        ImVec2 m = ImGui::GetMousePos();
        float mx = m.x - p0.x;
        float my = m.y - p0.y - header_h;

        if (mx >= 0 && my >= 0) {
            int c = (int)(mx / col_w);
            int r = (int)(my / cell_h);

            if (c >= 0 && c < 7 && r >= 0 && r < 6) {
                int idx = r * 7 + c;
                if (idx >= first && idx < first + total) {
                    Date target = { g_view.y, g_view.m, idx - first + 1 };
                    g_selected = target;

                    // MouseClickedCount: 1 = primer click, 2 = doble click
                    int clicks = ImGui::GetIO().MouseClickedCount[ImGuiMouseButton_Left];
                    if (clicks >= 2) {
                        g_popupDate = target;
                        g_popupTime[0] = 0;
                        g_popupText[0] = 0;
                        g_popupRequested = true;
                    }
                }
            }
        }
    }
}

// ---------------- Barra superior ----------------
static void drawTopBar() {
    if (navBtn("<##mPrev")) { if (--g_view.m == 0) { g_view.m = 12; g_view.y--; } }
    ImGui::SameLine(0, 2);
    if (navBtn(">##mNext")) { if (++g_view.m == 13) { g_view.m = 1; g_view.y++; } }

    ImGui::SameLine(0, 14);
    char title[64];
    std::snprintf(title, sizeof(title), "%s %d", MONTHS[g_view.m - 1], g_view.y);
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();

    float rightW = 2 * 74.0f + 60.0f;
    ImGui::SameLine(ImGui::GetWindowWidth() - rightW);

    toggleBtn("Month", true, 74);
    ImGui::SameLine(0, 4);
    toggleBtn("Agenda", false, 74);
    ImGui::SameLine(0, 12);

    // Engranaje → ajustes
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
    if (ImGui::Button("G##gear", ImVec2(28, 28)))
        g_showSettings = true;
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Ajustes");
}

// ---------------- Popup de ajustes ----------------
static void drawSettingsPopup() {
    if (g_showSettings) {
        ImGui::OpenPopup("Ajustes");
        g_showSettings = false;
    }

    if (ImGui::BeginPopupModal("Ajustes", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Preferencias");
        ImGui::Separator();
        ImGui::Dummy({ 0, 4 });

#ifdef _WIN32
        if (ImGui::Checkbox("Iniciar MinCalendar con el sistema", &g_autostart)) {
            if (!autostart::setEnabled(g_autostart))
                g_autostart = autostart::isEnabled();
        }
#else
        ImGui::TextDisabled("(auto-inicio disponible solo en Windows)");
#endif

        ImGui::Dummy({ 0, 12 });

        if (ImGui::Button("Cerrar", ImVec2(120, 30)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

// ---------------- Main ----------------
static void drawMain() {
    ImGui::BeginChild("##main", ImVec2(0, 0), false,
        ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));
    ImGui::BeginChild("##maintop", ImVec2(0, 40), false,
        ImGuiWindowFlags_NoScrollbar);
    drawTopBar();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 8));
    ImGui::BeginChild("##maingrid", ImVec2(0, 0), false,
        ImGuiWindowFlags_NoScrollbar);
    drawCalendarGrid();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::EndChild();
}

int main() {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* win = glfwCreateWindow(1360, 800, "MinCalendar", nullptr, nullptr);
    if (!win) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Tolerancia de doble click un poco más generosa
    io.MouseDoubleClickTime = 0.40f; // 400 ms
    io.MouseDoubleClickMaxDist = 10.0f; // 10 px

    loadFonts();
    applyStyle();

    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    initDates();
    g_store.load(getEventsPath());
    loadNotes();
#ifdef _WIN32
    g_autostart = autostart::isEnabled();
#endif

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGui::Begin("##root", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoDecoration);

        drawSidebar(320.0f);
        ImGui::SameLine(0, 0);
        drawMain();

        // El popup se abre siempre al nivel raíz para que su ID coincida
        // con el de BeginPopupModal.
        if (g_popupRequested) {
            ImGui::OpenPopup("Nuevo evento");
            g_popupRequested = false;
        }

        drawEventPopup();
        drawSettingsPopup();

        ImGui::End();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(C_BG.x, C_BG.y, C_BG.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    g_store.save(getEventsPath());
    if (g_notesDirty) saveNotes();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}