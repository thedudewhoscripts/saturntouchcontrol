#include "saturn_touch.h"
#include "libs/imgui/imgui.h"
#include <cmath>

extern "C" {
#include <ultra64.h>
}

static bool sTouchHudVisible = true;
static u16 sTouchButtons = 0;
static s8 sStickX = 0;
static s8 sStickY = 0;
static bool sStickDragging = false;

static void touch_init(void) {
    sTouchButtons = 0;
    sStickX = 0;
    sStickY = 0;
}

static void touch_read(OSContPad *pad) {
    pad->button |= sTouchButtons;
    if (sStickX != 0) pad->stick_x = sStickX;
    if (sStickY != 0) pad->stick_y = sStickY;
}

static u32 touch_rawkey(void) {
    return VK_INVALID;
}

extern "C" struct ControllerAPI controller_saturn_touch = {
    0x3000,
    touch_init,
    touch_read,
    touch_rawkey,
    NULL,
    NULL,
    NULL,
    NULL
};

static void DrawTouchCircleButton(ImDrawList *dl, const char *id, const char *label, ImVec2 center, float radius, ImU32 baseCol, u16 n64Mask) {
    ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
    ImGui::InvisibleButton(id, ImVec2(radius * 2.0f, radius * 2.0f));
    bool held = ImGui::IsItemActive();
    bool hovered = ImGui::IsItemHovered();

    if (held) {
        sTouchButtons |= n64Mask;
    }

    ImU32 fill = held ? IM_COL32(255, 255, 255, 170) : (hovered ? IM_COL32(255, 255, 255, 95) : baseCol);
    dl->AddCircleFilled(center, radius, fill, 32);
    dl->AddCircle(center, radius, IM_COL32(255, 255, 255, 190), 32, 2.2f);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f), IM_COL32(255, 255, 255, 240), label);
}

extern "C" void saturn_touch_render_imgui(void) {
    ImGuiIO &io = ImGui::GetIO();
    float w = io.DisplaySize.x;
    float h = io.DisplaySize.y;
    if (w <= 0.0f || h <= 0.0f) return;

    sTouchButtons = 0;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##SaturnTouchOverlay", nullptr, flags);
    ImDrawList *dl = ImGui::GetWindowDrawList();

    // Top-right toggle pill
    ImGui::SetCursorScreenPos(ImVec2(w - 115.0f, 32.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.15f, 0.65f));
    if (ImGui::Button(sTouchHudVisible ? "Hide Touch" : "Show Touch", ImVec2(100.0f, 26.0f))) {
        sTouchHudVisible = !sTouchHudVisible;
    }
    ImGui::PopStyleColor();

    if (!sTouchHudVisible) {
        sStickX = 0;
        sStickY = 0;
        ImGui::End();
        return;
    }

    // --- 1. Bottom-Left Virtual Analog Stick (coopdx style) ---
    float stickRadius = 72.0f;
    float knobRadius = 30.0f;
    ImVec2 stickCenter(125.0f, h - 125.0f);

    ImGui::SetCursorScreenPos(ImVec2(stickCenter.x - stickRadius * 1.3f, stickCenter.y - stickRadius * 1.3f));
    ImGui::InvisibleButton("##VirtualStick", ImVec2(stickRadius * 2.6f, stickRadius * 2.6f));

    if (ImGui::IsItemActive()) {
        sStickDragging = true;
    } else if (!ImGui::IsMouseDown(0)) {
        sStickDragging = false;
    }

    ImVec2 knobPos = stickCenter;
    if (sStickDragging) {
        float dx = io.MousePos.x - stickCenter.x;
        float dy = io.MousePos.y - stickCenter.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > stickRadius) {
            dx = (dx / dist) * stickRadius;
            dy = (dy / dist) * stickRadius;
        }
        knobPos = ImVec2(stickCenter.x + dx, stickCenter.y + dy);
        sStickX = (s8)((dx / stickRadius) * 80.0f);
        sStickY = (s8)((-dy / stickRadius) * 80.0f); // Invert Y for N64 up=+Y
    } else {
        sStickX = 0;
        sStickY = 0;
    }

    dl->AddCircleFilled(stickCenter, stickRadius, IM_COL32(255, 255, 255, 45), 40);
    dl->AddCircle(stickCenter, stickRadius, IM_COL32(255, 255, 255, 150), 40, 2.5f);
    dl->AddCircleFilled(knobPos, knobRadius, IM_COL32(230, 235, 245, 175), 32);
    dl->AddCircle(knobPos, knobRadius, IM_COL32(255, 255, 255, 230), 32, 2.0f);

    // --- 2. Bottom-Right coopdx Action Buttons (A, B, Z, R, L, Start, C-Pad) ---
    float rBig = 34.0f;
    float rSmall = 24.0f;

    // A (Blue) & B (Green)
    DrawTouchCircleButton(dl, "##BtnA", "A", ImVec2(w - 85.0f, h - 95.0f), rBig, IM_COL32(45, 125, 235, 110), A_BUTTON);
    DrawTouchCircleButton(dl, "##BtnB", "B", ImVec2(w - 165.0f, h - 75.0f), rBig, IM_COL32(45, 195, 95, 110), B_BUTTON);

    // Z (Purple/Gray) & R (Trigger) & L
    DrawTouchCircleButton(dl, "##BtnZ", "Z", ImVec2(w - 165.0f, h - 155.0f), 28.0f, IM_COL32(140, 140, 165, 105), Z_TRIG);
    DrawTouchCircleButton(dl, "##BtnR", "R", ImVec2(w - 75.0f, h - 180.0f), 26.0f, IM_COL32(180, 180, 195, 105), R_TRIG);
    DrawTouchCircleButton(dl, "##BtnL", "L", ImVec2(75.0f, h - 235.0f), 24.0f, IM_COL32(180, 180, 195, 95), L_TRIG);

    // Start (Red, top-center)
    DrawTouchCircleButton(dl, "##BtnStart", "START", ImVec2(w * 0.5f, h - 42.0f), 26.0f, IM_COL32(220, 60, 60, 115), START_BUTTON);

    // C-Buttons (Yellow diamond cluster above A/B)
    ImVec2 cCenter(w - 125.0f, h - 255.0f);
    float cOffset = 34.0f;
    ImU32 cCol = IM_COL32(235, 190, 35, 110);
    DrawTouchCircleButton(dl, "##BtnCU", "C^", ImVec2(cCenter.x, cCenter.y - cOffset), rSmall, cCol, U_CBUTTONS);
    DrawTouchCircleButton(dl, "##BtnCD", "Cv", ImVec2(cCenter.x, cCenter.y + cOffset), rSmall, cCol, D_CBUTTONS);
    DrawTouchCircleButton(dl, "##BtnCL", "C<", ImVec2(cCenter.x - cOffset, cCenter.y), rSmall, cCol, L_CBUTTONS);
    DrawTouchCircleButton(dl, "##BtnCR", "C>", ImVec2(cCenter.x + cOffset, cCenter.y), rSmall, cCol, R_CBUTTONS);

    ImGui::End();
}
