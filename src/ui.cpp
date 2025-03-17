#include "ui.h"

HWND hwndMain;
HFONT hFont;
std::unordered_map<u_int, FishingRod> rod_table;
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
            
            FishingRod displaying_rods[3] = {0};
            for (auto it = rod_table.begin(); it != rod_table.end(); i++) {
                if (it->second.short_cut == 1) {
                    displaying_rods[0] = it->second;
                } else if (it->second.short_cut == 2) {
                    displaying_rods[1] = it->second;
                } else if (it->second.short_cut == 3) {
                    displaying_rods[2] = it->second;
                } 
            }
            
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
                    L"快捷键: %d | 鱼竿码: %08X | 鱼竿类型: %d",
                    displaying_rods[i].short_cut, 
                    displaying_rods[i].rod_hash, 
                    displaying_rods[i].rod_type);
                
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

// 更新UI函数 - 可从其他线程或定时器调用
void UpdateUI() {
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