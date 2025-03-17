#include "ui.h"

HWND hwndMain;
HFONT hFont;
FishingRod rods[MAX_RODS];
// 主函数
void startWinUI() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOWDEFAULT;
    // 注册窗口类
    WNDCLASSW  wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"FishingRodUI";  // 移除了L前缀
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassW(&wc);
    
    // 创建主窗口
    hwndMain = CreateWindowW(
        L"FishingRodUI", L"钓鱼信息显示", 
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 200,
        NULL, NULL, hInstance, NULL
    );
    
    // 创建字体
    hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                      CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                      DEFAULT_PITCH | FF_DONTCARE, L"微软雅黑");  // 移除了L前缀
    
    // 初始化渔具数据
    InitFishingRods();
    
    // 显示窗口
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    // 隐藏黑窗口
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    
    // 消息循环
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理资源
    DeleteObject(hFont);
    
}

// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            
            // 保存原始对象
            HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
            
            // 绘制每行信息
            int rowHeight = clientRect.bottom / MAX_RODS;
            
            for (int i = 0; i < MAX_RODS; i++) {
                RECT rowRect = {0, i * rowHeight, clientRect.right, (i + 1) * rowHeight};
                
                // 填充背景颜色
                HBRUSH hBrush = CreateSolidBrush(rods[i].bgColor);
                FillRect(hdc, &rowRect, hBrush);
                DeleteObject(hBrush);
                
                // 在此区域绘制文本
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(255, 255, 255));
                
                wchar_t textBuffer[MAX_TEXT_LENGTH];
                swprintf(textBuffer, MAX_TEXT_LENGTH,
                    L"快捷键: %d | 鱼竿码: %d | 鱼竿类型: %s | 鱼名: %s | 重量: %.2f kg",
                    rods[i].hotkey, rods[i].rodCode, 
                    rods[i].rodType,
                    rods[i].fishName,
                    rods[i].fishWeight);
                
                DrawTextW(hdc, textBuffer, -1, &rowRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
            }
            
            // 恢复原始对象
            SelectObject(hdc, oldFont);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 初始化渔具数据
void InitFishingRods() {
    // 初始化三个渔具示例数据
    UpdateRodInfo(0, 3, 1001, L"碳素竿", L"鲤鱼", 2.5f, RGB(0, 120, 215));
    UpdateRodInfo(1, 1, 1002, L"海竿", L"鲈鱼", 1.8f, RGB(0, 150, 100));
    UpdateRodInfo(2, 2, 1003, L"溪流竿", L"鳟鱼", 3.2f, RGB(120, 0, 150));
    
    // 按快捷键排序
    SortRodsByHotkey();
}

// 更新渔具信息
void UpdateRodInfo(int index, int hotkey, int rodCode, const wchar_t* rodType, const wchar_t* fishName, float fishWeight, COLORREF bgColor) {
    if (index >= 0 && index < MAX_RODS) {
        rods[index].hotkey = hotkey;
        rods[index].rodCode = rodCode;
        wcscpy(rods[index].rodType, rodType);
        wcscpy(rods[index].fishName, fishName);
        rods[index].fishWeight = fishWeight;
        rods[index].bgColor = bgColor;
    }
}

// 按快捷键排序
void SortRodsByHotkey() {
    for (int i = 0; i < MAX_RODS - 1; i++) {
        for (int j = 0; j < MAX_RODS - i - 1; j++) {
            if (rods[j].hotkey > rods[j + 1].hotkey) {
                FishingRod temp = rods[j];
                rods[j] = rods[j + 1];
                rods[j + 1] = temp;
            }
        }
    }
}

// 更新UI函数 - 可从其他线程或定时器调用
void UpdateUI() {
    // 排序
    SortRodsByHotkey();
    // 强制重绘窗口
    InvalidateRect(hwndMain, NULL, TRUE);
}

// 示例: 如何从外部更新数据
// 在实际应用中，你可以创建自定义消息或定时器来调用这些函数
// WM_APP + 1 作为自定义消息，用于接收外部更新

/*
// 示例使用:
// 更新第一个鱼竿的信息
UpdateRodInfo(0, 5, 2001, "碳纤维竿", "草鱼", 4.7f, RGB(200, 50, 50));
// 更新UI
UpdateUI();
*/