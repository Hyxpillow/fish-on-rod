#pragma once
#include <windows.h>
#include <iostream>

#define MAX_RODS 3
#define MAX_TEXT_LENGTH 256

// 定义渔具结构体
typedef struct {
    int hotkey;
    int rodCode;
    wchar_t rodType[MAX_TEXT_LENGTH];
    wchar_t fishName[MAX_TEXT_LENGTH];
    float fishWeight;
    COLORREF bgColor;
} FishingRod;

// 全局变量
extern HWND hwndMain;
extern HFONT hFont;
extern FishingRod rods[MAX_RODS];


// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void startWinUI();
void InitFishingRods();
void SortRodsByHotkey();
void UpdateUI();
void UpdateRodInfo(int index, int hotkey, int rodCode, const wchar_t* rodType, const wchar_t* fishName, float fishWeight, COLORREF bgColor);