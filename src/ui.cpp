#include "ui.h"

HWND hGrid[3][2]; // Grid of window controls
HWND hStatusBar;
HWND hwndMain;
HFONT  hFont;
int initialed;
HBRUSH hb1;
HBRUSH hb2;
std::unordered_map<u_int, FishingRod> rod_table;
std::unordered_map<HWND, HBRUSH> cellBackgrounds;

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
        L"FishingRodUI", L"妙妙小工具", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX ,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 200,
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
            SendMessageW(hStatusBar, WM_SETFONT, (WPARAM)hFont, TRUE);

            hb2 = CreateSolidBrush(RGB(255, 167, 37));
            hb1 = CreateSolidBrush(RGB(255, 245, 228));
            
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 2; col++) {
                    hGrid[row][col] = CreateWindowExW(
                        WS_EX_CLIENTEDGE, // Border style
                        L"STATIC", // Using STATIC control for cells
                        L"", // Initial text
                        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                        col == 0 ? 0 : 100, // X position (column 1 starts at 0, column 2 at 60)
                        row * 50, // Y position (each row is 50px high)
                        col == 0 ? 100 : 500, // Width (column 1 is 60px, column 2 is 540px)
                        50, // Height (each cell is 50px high)
                        hwnd,
                        (HMENU)MAKEINTRESOURCE(200 + row * 2 + col), // Unique ID for each cell
                        ((LPCREATESTRUCT)lParam)->hInstance,
                        NULL
                    );

                    cellBackgrounds[hGrid[row][col]] = hb1;
                    
                    // Set initial text for each cell
                    WCHAR cellText[100];
                    if (col == 0)
                        swprintf(cellText, 100, L"鱼竿%d", row + 1);
                    else
                        swprintf(cellText, 100, L"[空]");
                    SetWindowTextW(hGrid[row][col], cellText);
                    SendMessageW(hGrid[row][col], WM_SETFONT, (WPARAM)hFont, TRUE);
                }
            }
            initialed = 1;
            break;
        }
        case WM_SIZE: {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            // 调整状态栏大小
            SendMessage(hStatusBar, WM_SIZE, 0, 0);
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

            if (cellBackgrounds.find(hwndStatic) != cellBackgrounds.end()) {
                SetBkMode(hdcStatic, TRANSPARENT);
                return (LRESULT)cellBackgrounds[hwndStatic]; // 返回对应的 HBRUSH
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
    for (int i = 0; i < 3; i++) {
        SetWindowTextW(hGrid[i][1], L"--");
        UpdateColor(i, 0);
    }
}


void UpdateStatus(WCHAR *str) {
    SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)str);
}

void UpdateText(int row, WCHAR str[]) {
    SetWindowTextW(hGrid[row][1], str);
}

void UpdateColor(int row, int color) {
    HWND hwndCell1 = hGrid[row][0];
    HWND hwndCell2 = hGrid[row][1];
    if (color == 0) {
        cellBackgrounds[hwndCell1] = hb1;
        cellBackgrounds[hwndCell2] = hb1;
    } else if (color == 1) {
        cellBackgrounds[hwndCell1] = hb2;
        cellBackgrounds[hwndCell2] = hb2;
    }
    InvalidateRect(hwndCell1, NULL, TRUE); // 触发重绘
    InvalidateRect(hwndCell2, NULL, TRUE); // 触发重绘
}