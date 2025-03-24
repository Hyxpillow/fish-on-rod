#include "ui.h"

HWND hGrid; // Grid of window controls
HWND hStatusBar;
HWND hwndMain;
HFONT  hFont;
int initialed;
HBRUSH hb1;
HBRUSH hb2;
HBRUSH current_hb;
// std::unordered_map<u_int, FishingRod> rod_table;
// std::unordered_map<HWND, HBRUSH> cellBackgrounds;

// 主函数
void startWinUI() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOWDEFAULT;
    // 注册窗口类
    WNDCLASSW  wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"Hyxpillow";  // 移除了L前缀
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassW(&wc);
    
    // 创建主窗口
    hwndMain = CreateWindowW(
        L"Hyxpillow", L"LoL School", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX ,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 150,
        NULL, NULL, hInstance, NULL
    );
    SetForegroundWindow(hwndMain);
    
    // 创建字体
    hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                      CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                      DEFAULT_PITCH | FF_DONTCARE, L"宋体"); 
    
    // 显示窗口
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    // 隐藏黑窗口
    FreeConsole();

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
        case WM_CREATE: {
            hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
                WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                hwnd, (HMENU)100, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            // 设置状态栏分为三个部分，第一部分占50%，其余两部分各占25%
            int parts[3];
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            parts[0] = rcClient.right * 7 / 10; // 第一部分占50%
            parts[1] = rcClient.right * 8.5 / 10; // 第二部分结束位置（75%）
            parts[2] = -1;  // 第三部分延伸到窗口右边缘
            
            // 应用分割
            SendMessageW(hStatusBar, SB_SETPARTS, 3, (LPARAM)parts);
            
            // 设置初始文本
            SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"--");
            SendMessageW(hStatusBar, SB_SETTEXTW, 1, (LPARAM)L"--");
            SendMessageW(hStatusBar, SB_SETTEXTW, 2, (LPARAM)L"--");
            
            SendMessageW(hStatusBar, WM_SETFONT, (WPARAM)hFont, TRUE);

            hb2 = CreateSolidBrush(RGB(255, 167, 37));
            hb1 = CreateSolidBrush(RGB(255, 245, 228));
            
            hGrid = CreateWindowExW(
                WS_EX_CLIENTEDGE, // Border style
                L"STATIC", // Using STATIC control for cells
                L"", // Initial text
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                0,
                0, // Y position (each row is 50px high)
                600, // Width (column 1 is 60px, column 2 is 540px)
                100, // Height (each cell is 50px high)
                hwnd,
                (HMENU)MAKEINTRESOURCE(200), // Unique ID for each cell
                ((LPCREATESTRUCT)lParam)->hInstance,
                NULL
            );

            // cellBackgrounds[hGrid[0][col]] = hb1;
            current_hb = hb1;
            
            // Set initial text for each cell
            WCHAR cellText[100];
            swprintf(cellText, 100, L"[空]");
            SetWindowTextW(hGrid, cellText);
            SendMessageW(hGrid, WM_SETFONT, (WPARAM)hFont, TRUE);
            initialed = 1;
            break;
        }
        case WM_SIZE: {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // 重新计算状态栏分割
            int parts[3];
            parts[0] = rcClient.right * 7 / 10; // 第一部分占50%
            parts[1] = rcClient.right * 8.5 / 10; // 第二部分结束位置（75%）
            parts[2] = -1;  // 第三部分延伸到窗口右边缘
            
            // 调整状态栏大小
            SendMessage(hStatusBar, WM_SIZE, 0, 0);
            // 应用新的分割
            SendMessageW(hStatusBar, SB_SETPARTS, 3, (LPARAM)parts);
            
            // 获取状态栏高度
            RECT rcStatus;
            GetWindowRect(hStatusBar, &rcStatus);
            int statusHeight = rcStatus.bottom - rcStatus.top;

            return 0;
        }

        case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            HWND hwndStatic = (HWND)lParam;

            if (hwndStatic == hGrid) {
                SetBkMode(hdcStatic, TRANSPARENT);
                return (LRESULT)current_hb; // 返回对应的 HBRUSH
            }
            break;
        }
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void UI_reset() {
    SetWindowTextW(hGrid, L"--");
    UpdateColor(0);
}


void UpdateStatus(const WCHAR *str, int part) {
    if (part >= 0 && part <= 2) {
        SendMessageW(hStatusBar, SB_SETTEXTW, part, (LPARAM)str);
    }
}

void UpdateStatus(WCHAR *str) {
    UpdateStatus(str, 0);
}

void UpdateText(WCHAR str[]) {
    SetWindowTextW(hGrid, str);
}

void UpdateColor(int color) {
    if (color == 0) {
        current_hb = hb1;
    } else if (color == 1) {
        current_hb = hb2;
    }
    InvalidateRect(hGrid, NULL, TRUE); // 触发重绘
}