#pragma once
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <unordered_map>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#define MAX_RODS 3
#define MAX_TEXT_LENGTH 256

// 定义渔具结构体
// typedef struct {
//     unsigned int short_cut;
//     float fish_weight;
//     const char* fish_name;
    
//     WCHAR state[256]; 
//     unsigned int rod_type; // 1:浮子 2:路亚 3:水底 4:海钓
//     unsigned int color; // 0:白色 1:黄色
// } FishingRod;


// 全局变量
extern HWND hwndMain;
extern HFONT hFont;
extern int initialed;
// extern std::unordered_map<u_int, FishingRod> rod_table;


// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void startWinUI();
void UI_reset();
void UpdateStatus(WCHAR *str);
void UpdateStatus(const WCHAR *str, int part);
void UpdateText(WCHAR str[]);
void UpdateColor(int color);