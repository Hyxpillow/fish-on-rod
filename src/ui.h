#pragma once
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <unordered_map>
#include <commctrl.h>
#include <gdiplus.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

#define MAX_RODS 3
#define MAX_TEXT_LENGTH 256

enum MenuIDs {
    // 展示设置子菜单
    ID_DISPLAY_SINGLE_ROD = 1000,
    ID_DISPLAY_MULTI_ROD,

    // 音效设置子菜单
    ID_SOUND_FISH_HOOKED,
    ID_SOUND_FISH_SPAWN,
    ID_SOUND_MARINE,

    // 颜色设置子菜单
    ID_COLOR_FISH_HOOKED,
    ID_COLOR_FISH_SPAWN,
    ID_COLOR_MARINE,

    // 海钓设置子菜单
    ID_MARINE_BOTTOM_MOTION,
    ID_MARINE_DISTANCE_MARKER,
    
    // 其他菜单项
    ID_PROMICRO_SEND,
    ID_PROMICRO_AUTO,
};

extern int initialed;
extern float meter;

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void startWinUI();
void UI_reset();
void UpdateStatus(WCHAR *str);
void UpdateStatus(const WCHAR *str, int part);

void UpdateTextSingle(WCHAR str[]);
void UpdateColorSingle(int color);
void UpdateTextMulti(int idx, WCHAR str[]);
void UpdateColorMulti(int idx, int color);
void UpdatePngSingle(Gdiplus::Image* data);
void UpdatePngMulti(int idx, Gdiplus::Image* data);
void UpdateRaritySingle(Gdiplus::Image* data);
void UpdateRarityMulti(int idx, Gdiplus::Image* data);
void UpdateTrophySingle(Gdiplus::Image* data);
void UpdateTrophyMulti(int idx, Gdiplus::Image* data);


bool isMenuChecked(int id);
void ShowInputDialog(HINSTANCE hInstance, HWND parent);
void UpdateMenuText(HWND hwnd, WCHAR newText[]);
void AdjustLayout(HWND hwnd);
