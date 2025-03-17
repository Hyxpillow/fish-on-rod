#pragma once
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <unordered_map>

#define MAX_RODS 3
#define MAX_TEXT_LENGTH 256

// 定义渔具结构体
typedef struct {
    unsigned int short_cut;
    unsigned int rod_hash;
    float fish_weight;
    std::string fish_name;
    unsigned int rod_type;
    COLORREF bgColor;
} FishingRod;


// 全局变量
extern HWND hwndMain;
extern HFONT hFont;
extern std::unordered_map<u_int, FishingRod> rod_table;


// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void startWinUI();
void InitFishingRods();
void SortRodsByHotkey();
void UpdateUI();
void UpdateRodInfo(int index, int hotkey, int rodCode, const wchar_t* rodType, const wchar_t* fishName, float fishWeight, COLORREF bgColor);