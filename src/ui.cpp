#include "ui.h"

HWND hGridSingle; // Grid of window controls
HWND hGridMulti[3];      // 多个格子

bool multiMode = false;

HWND hStatusBar;
HWND hwndMain;
HFONT hStatusBarFont;
HFONT hGridFont;

int initialed;
HBRUSH hb1;
HBRUSH hb2;
HBRUSH hb3;
HBRUSH current_hb_single;
HBRUSH current_hb_multi[3];
HWND hImageControl = NULL;
HWND hToolbar;
HMENU hMenuBar;
float meter; // 卡米长度

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
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 窗口尺寸（假设你的窗口大小是 600x150）
    int windowWidth = 600;
    int windowHeight = 150;

    // 计算窗口位置（屏幕中心向下偏移 1/4 屏幕高度）
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2 + screenHeight / 4;
    // 创建主窗口
    hwndMain = CreateWindowW(
        L"Hyxpillow", L"青年大学习", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );
    SetForegroundWindow(hwndMain);
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
    // DeleteObject(hStatusBarFont);
    
}

bool isMenuChecked(int id) {
    return (GetMenuState(hMenuBar, id, MF_BYCOMMAND) & MF_CHECKED) != 0;
}

void CreateFishingAppMenu(HMENU hMenuBar) {
    // 创建主菜单项
    HMENU hDisplayMenu = CreatePopupMenu();
    HMENU hSoundMenu = CreatePopupMenu();
    HMENU hColorMenu = CreatePopupMenu();
    HMENU hMarineMenu = CreatePopupMenu();
    HMENU hDebugMenu = CreatePopupMenu();

    // 1. 展示设置菜单
    AppendMenuW(hDisplayMenu, MF_STRING | MF_CHECKED, ID_DISPLAY_SINGLE_ROD, L"单竿模式");
    AppendMenuW(hDisplayMenu, MF_STRING, ID_DISPLAY_MULTI_ROD, L"多竿模式");

    // 2. 音效设置菜单
    AppendMenuW(hSoundMenu, MF_STRING, ID_SOUND_FISH_HOOKED, L"鱼咬钩时");
    AppendMenuW(hSoundMenu, MF_STRING, ID_SOUND_FISH_SPAWN, L"鱼刷新时 (浮子)");
    AppendMenuW(hSoundMenu, MF_STRING, ID_SOUND_MARINE, L"海钓设置条件满足时");

    // 3. 颜色设置菜单
    AppendMenuW(hColorMenu, MF_STRING, ID_COLOR_FISH_HOOKED, L"鱼咬钩时");
    AppendMenuW(hColorMenu, MF_STRING, ID_COLOR_FISH_SPAWN, L"鱼刷新时");
    AppendMenuW(hColorMenu, MF_STRING, ID_COLOR_MARINE, L"海钓设置条件满足时");

    // 3. 海钓音效设置菜单
    AppendMenuW(hMarineMenu, MF_STRING | MF_CHECKED, ID_MARINE_BOTTOM_MOTION, L"首次出现底层运动");
    AppendMenuW(hMarineMenu, MF_STRING, ID_MARINE_DISTANCE_MARKER, L"首次到达[卡米]");

    // 4. 其他菜单
    AppendMenuW(hDebugMenu, MF_STRING, ID_PROMICRO_SEND, L"调试模式1");
    AppendMenuW(hDebugMenu, MF_STRING, ID_PROMICRO_AUTO, L"调试模式2");

    // 将子菜单添加到主菜单栏
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hDisplayMenu, L"显示");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hSoundMenu, L"音效");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hColorMenu, L"颜色");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hMarineMenu, L"海钓设置");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hDebugMenu, L"其他");
}

void UI_reset() {
    SetWindowTextW(hGridSingle, L"--");
    UpdateColorSingle(0);
    WCHAR cellText[256];
    swprintf(cellText, 256, L"[空]");
    UpdateTextMulti(0, cellText);
    UpdateTextMulti(1, cellText);
    UpdateTextMulti(2, cellText);
    UpdateColorMulti(0, 0);
    UpdateColorMulti(1, 0);
    UpdateColorMulti(2, 0);

    UpdatePngSingle(nullptr);
    UpdatePngMulti(0, nullptr);
    UpdatePngMulti(1, nullptr);
    UpdatePngMulti(2, nullptr);

    UpdateTrophySingle(nullptr);
    UpdateTrophyMulti(0, nullptr);
    UpdateTrophyMulti(1, nullptr);
    UpdateTrophyMulti(2, nullptr);

    UpdateRaritySingle(nullptr);
    UpdateRarityMulti(0, nullptr);
    UpdateRarityMulti(1, nullptr);
    UpdateRarityMulti(2, nullptr);
}


void UpdateStatus(const WCHAR *str, int part) {
    if (part >= 0 && part <= 2) {
        SendMessageW(hStatusBar, SB_SETTEXTW, part, (LPARAM)str);
    }
}

void UpdateStatus(WCHAR *str) {
    UpdateStatus(str, 0);
}

ULONG_PTR g_gdiplusToken;

class PngControl {
    public:
        HWND hwnd;
        Gdiplus::Image* image;
    
        PngControl(HWND parent, LPCWSTR path, int x, int y, int w, int h) {
            image = new Gdiplus::Image(path);
            hwnd = CreateWindowExW(0, L"STATIC", nullptr, WS_CHILD | WS_VISIBLE, // 不含 WS_VISIBLE
                                  x, y, w, h, parent, nullptr, nullptr, nullptr);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
            SetWindowSubclass(hwnd, SubclassProc, 0, 0);
        }
    
        ~PngControl() {
            delete image;
            RemoveWindowSubclass(hwnd, SubclassProc, 0);
        }
    
        void Show(bool visible) {
            ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
        }
    
        void SetImage(Gdiplus::Image* newImage) {
            if (image != newImage) {
                image = newImage;
                InvalidateRect(hwnd, nullptr, TRUE);
            }
        }
    
        void OnPaint(HDC hdc) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            Gdiplus::Graphics g(hdc);
            g.DrawImage(image, Gdiplus::Rect(0, 0, rect.right, rect.bottom));
        }
    
        static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR, DWORD_PTR) {
            if (msg == WM_PAINT) {
                PngControl* ctrl = (PngControl*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                ctrl->OnPaint(hdc);
                EndPaint(hwnd, &ps);
                return 0;
            }
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }
    };
    
PngControl* pngCtrlSingle;
PngControl* rarityCtrlSingle;
PngControl* trophyCtrlSingle;

PngControl* pngCtrlMulti[3];
PngControl* rarityCtrlMulti[3];
PngControl* trophyCtrlMulti[3];


// 初始化 GDI+（程序启动时调用）
void InitGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
}

// 释放 GDI+（程序退出时调用）
void ShutdownGDIPlus() {
    Gdiplus::GdiplusShutdown(g_gdiplusToken);
}

LRESULT CALLBACK InputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit;

    switch (msg) {
    case WM_CREATE:
        CreateWindowExW(0, L"STATIC", L"卡米长度:",
            WS_CHILD | WS_VISIBLE,
            10, 10, 260, 20,
            hwnd, NULL, NULL, NULL);

        hEdit = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 40, 260, 25,
            hwnd, (HMENU)1, NULL, NULL);

        CreateWindowExW(0, L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            50, 80, 70, 25,
            hwnd, (HMENU)2, NULL, NULL);

        CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE,
            150, 80, 70, 25,
            hwnd, (HMENU)3, NULL, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 2) { // OK
            WCHAR buf[100];
            GetWindowTextW(hEdit, buf, 100);
            meter = (float)wcstod(buf, NULL);
            DestroyWindow(hwnd);
        }
        else if (LOWORD(wParam) == 3) { // Cancel
            DestroyWindow(hwnd);
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowInputDialog(HINSTANCE hInstance, HWND parent) {
    const wchar_t CLASS_NAME[] = L"InputFloatWindow";

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = InputProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 窗口尺寸（假设你的窗口大小是 600x150）
    int windowWidth = 300;
    int windowHeight = 160;

    // 计算窗口位置（屏幕中心向下偏移 1/4 屏幕高度）
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2 + screenHeight / 4;
    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"设置",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, windowWidth, windowHeight,
        parent, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
void UpdateMenuText(HWND hwnd, WCHAR newText[]) {
    HMENU hMenu = GetMenu(hwnd);
    HMENU hSubMenu = GetSubMenu(hMenu, 3); // 假设“设置”是菜单栏的第2项，索引从0开始

    if (hSubMenu) {
        ModifyMenuW(hSubMenu, ID_MARINE_DISTANCE_MARKER, MF_BYCOMMAND | MF_STRING, ID_MARINE_DISTANCE_MARKER, newText);
        DrawMenuBar(hwnd);
    }
}

void AdjustLayout(HWND hwnd) {
    RECT rcClient, rcWindow, rcStatus;
    GetClientRect(hwnd, &rcClient);
    GetWindowRect(hwnd, &rcWindow);
    GetWindowRect(hStatusBar, &rcStatus);

    int gridHeight = multiMode ? 50 : 75; // 单竿模式和多竿模式的高度
    int sbHeight = rcStatus.bottom - rcStatus.top;
    int rodCount = multiMode ? 3 : 1;
    int requiredClientHeight = rodCount * gridHeight + sbHeight;

    // 获取当前窗口外框和客户区高度差（边框 + 标题栏等）
    int windowFrameHeight = (rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top);

    // 设置窗口新高度 = 所需客户区高度 + 框架高度
    int newWindowHeight = requiredClientHeight + windowFrameHeight;

    // 保持宽度和位置不变，调整高度
    MoveWindow(hwnd,
        rcWindow.left,
        rcWindow.top,
        rcWindow.right - rcWindow.left,
        newWindowHeight,
        TRUE);

    // 显示对应控件
    if (multiMode) {
        ShowWindow(hGridSingle, SW_HIDE);
        for (int i = 0; i < 3; ++i) {
            ShowWindow(hGridMulti[i], SW_SHOW);
        }
        for (int i = 0; i < 3; ++i) {
            MoveWindow(hGridMulti[i], 0, i * gridHeight, rcClient.right, gridHeight, TRUE);
        }
    } else {
        ShowWindow(hGridSingle, SW_SHOW);
        for (int i = 0; i < 3; ++i) {
            ShowWindow(hGridMulti[i], SW_HIDE);
        }
        MoveWindow(hGridSingle, 0, 0, rcClient.right, gridHeight, TRUE);
    }

    // 重新获取 client 区（因为 MoveWindow 可能刚更新）
    GetClientRect(hwnd, &rcClient);

    // 移动状态栏到底部
    MoveWindow(hStatusBar,
        0,
        rcClient.bottom - sbHeight,
        rcClient.right,
        sbHeight,
        TRUE);
}

void UpdateTextSingle(WCHAR str[]) {
    SetWindowTextW(hGridSingle, str);
}
void UpdateColorSingle(int color) {
    if (color == 0) {
        current_hb_single = hb1;
    } else if (color == 1) {
        current_hb_single = hb2;
    } else if (color == 2) {
        current_hb_single = hb3;
    }
    InvalidateRect(hGridSingle, NULL, TRUE); // 触发重绘
}
void UpdateTextMulti(int idx, WCHAR str[]) {
    if (idx < 0 || idx >= 3) return; // 确保索引在范围内
    SetWindowTextW(hGridMulti[idx], str);
}
void UpdateColorMulti(int idx, int color) {
    if (idx < 0 || idx >= 3) return; // 确保索引在范围内
    if (color == 0) {
        current_hb_multi[idx] = hb1;
    } else if (color == 1) {
        current_hb_multi[idx] = hb2;
    } else if (color == 2) {
        current_hb_multi[idx] = hb3;
    }
    InvalidateRect(hGridMulti[idx], NULL, TRUE); // 触发重绘
}

void UpdatePngSingle(Gdiplus::Image* data) {
    pngCtrlSingle->SetImage(data);
}
void UpdatePngMulti(int idx, Gdiplus::Image* data) {
    if (idx < 0 || idx >= 3) return; // 确保索引在范围内
    pngCtrlMulti[idx]->SetImage(data);
}
void UpdateRaritySingle(Gdiplus::Image* data) {
    rarityCtrlSingle->SetImage(data);
}
void UpdateRarityMulti(int idx, Gdiplus::Image* data) {
    if (idx < 0 || idx >= 3) return; // 确保索引在范围内
    rarityCtrlMulti[idx]->SetImage(data);
}
void UpdateTrophySingle(Gdiplus::Image* data) {
    trophyCtrlSingle->SetImage(data);
}
void UpdateTrophyMulti(int idx, Gdiplus::Image* data) {
    if (idx < 0 || idx >= 3) return; // 确保索引在范围内
    trophyCtrlMulti[idx]->SetImage(data);
}


// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WCHAR newText[64];
    switch (uMsg) {
        case WM_CREATE: {
            InitGDIPlus();
            // 创建字体
            hStatusBarFont = CreateFontW(
                18,                     // Height (increased for better readability)
                0,                      // Width (0 means default)
                0,                      // Escapement
                0,                      // Orientation
                FW_NORMAL,              // Weight (can use FW_BOLD for bolder text)
                FALSE,                  // Italic
                FALSE,                  // Underline
                FALSE,                  // StrikeOut
                DEFAULT_CHARSET,        // Charset (important for Unicode/Chinese)
                OUT_DEFAULT_PRECIS,     // Output Precision
                CLIP_DEFAULT_PRECIS,    // Clipping Precision
                CLEARTYPE_QUALITY,      // Quality (provides smoother text)
                DEFAULT_PITCH | FF_SWISS, // Pitch and Family
                L"Microsoft YaHei"      // Typeface name (excellent for Chinese)
            );

            hGridFont = CreateFontW(
                22,                     // Height (increased for better readability)
                0,                      // Width (0 means default)
                0,                      // Escapement
                0,                      // Orientation
                FW_NORMAL,              // Weight (can use FW_BOLD for bolder text)
                FALSE,                  // Italic
                FALSE,                  // Underline
                FALSE,                  // StrikeOut
                DEFAULT_CHARSET,        // Charset (important for Unicode/Chinese)
                OUT_DEFAULT_PRECIS,     // Output Precision
                CLIP_DEFAULT_PRECIS,    // Clipping Precision
                CLEARTYPE_QUALITY,      // Quality (provides smoother text)
                DEFAULT_PITCH | FF_SWISS, // Pitch and Family
                L"SimHei"      // Typeface name (excellent for Chinese)
            );
            hMenuBar = CreateMenu();
            CreateFishingAppMenu(hMenuBar);

            SendMessageW(hwnd, WM_SETFONT, (WPARAM)hStatusBarFont, TRUE);
            SetMenu(hwnd, hMenuBar);

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
            
            SendMessageW(hStatusBar, WM_SETFONT, (WPARAM)hStatusBarFont, TRUE);

            hb2 = CreateSolidBrush(RGB(255, 167, 37));
            hb1 = CreateSolidBrush(RGB(255, 245, 228));
            hb3 = CreateSolidBrush(RGB(193, 216, 195));
            
            
            hGridSingle = CreateWindowExW(
                WS_EX_CLIENTEDGE, // Border style
                L"STATIC", // Using STATIC control for cells
                L"", // Initial text
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                0,
                0, // Y position (each row is 50px high)
                600, // Width (column 1 is 60px, column 2 is 540px)
                75, // Height (each cell is 50px high)
                hwnd,
                (HMENU)MAKEINTRESOURCE(200), // Unique ID for each cell
                ((LPCREATESTRUCT)lParam)->hInstance,
                NULL
            );
            SendMessageW(hGridSingle, WM_SETFONT, (WPARAM)hGridFont, TRUE);
            pngCtrlSingle = new PngControl(hGridSingle, L"fish_pic/syb_elets.png", 0, 15, 48, 48);
            trophyCtrlSingle = new PngControl(hGridSingle, nullptr, 60, 22, 20, 33);
            rarityCtrlSingle = new PngControl(hGridSingle, nullptr, 85, 22, 20, 33);
            current_hb_single = hb1;

            
            for (int i = 0; i < 3; ++i) {
                hGridMulti[i] = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"STATIC", L"[空]",
                    WS_CHILD | SS_CENTER | SS_CENTERIMAGE,
                    0, 50 * i, 600, 50,
                    hwnd, (HMENU)(INT_PTR)(200 + i + 1), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            
                SendMessageW(hGridMulti[i], WM_SETFONT, (WPARAM)hGridFont, TRUE);
                pngCtrlMulti[i] = new PngControl(hGridMulti[i], L"fish_pic/syb_elets.png", 0, 0, 48, 48);
                rarityCtrlMulti[i] = new PngControl(hGridMulti[i], nullptr, 60, 7, 20, 33);
                trophyCtrlMulti[i] = new PngControl(hGridMulti[i], nullptr, 85, 7, 20, 33);
                current_hb_multi[i] = hb1;
            }
            initialed = 1;
            break;
        }
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
            if (dis->CtlType == ODT_MENU) {
                FillRect(dis->hDC, &dis->rcItem, CreateSolidBrush(RGB(240, 240, 240)));
                SetBkMode(dis->hDC, TRANSPARENT);
                SetTextColor(dis->hDC, RGB(0, 0, 0));
        
                WCHAR text[256] = {};
                GetMenuStringW(hMenuBar, dis->itemID, text, 255, MF_BYCOMMAND);
                DrawTextW(dis->hDC, text, -1, &dis->rcItem, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
                return TRUE;
            }
            break;
        }
        case WM_SIZE: {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // 重新计算状态栏分割
            int parts[3];
            parts[0] = rcClient.right * 7 / 10; // 第一部分占70%
            parts[1] = rcClient.right * 8.5 / 10; // 第二部分结束位置（85%）
            parts[2] = -1;  // 第三部分延伸到窗口右边缘
            
            // 调整状态栏位置和大小
            RECT rcStatus;
            GetWindowRect(hStatusBar, &rcStatus);
            int statusHeight = rcStatus.bottom - rcStatus.top;
        
            // 移动状态栏到窗口底部
            MoveWindow(hStatusBar, 
                0, 
                rcClient.bottom - statusHeight, 
                rcClient.right, 
                statusHeight, 
                TRUE);
            
            // 应用新的分割
            SendMessageW(hStatusBar, SB_SETPARTS, 3, (LPARAM)parts);
            
            return 0;
        }
    
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                // 展示设置处理
                case ID_DISPLAY_SINGLE_ROD:
                    CheckMenuItem(GetSubMenu(GetMenu(hwnd), 0), ID_DISPLAY_SINGLE_ROD, MF_CHECKED);
                    CheckMenuItem(GetSubMenu(GetMenu(hwnd), 0), ID_DISPLAY_MULTI_ROD, MF_UNCHECKED);
                    multiMode = false;
                    AdjustLayout(hwnd);
                    break;
                case ID_DISPLAY_MULTI_ROD:
                    CheckMenuItem(GetSubMenu(GetMenu(hwnd), 0), ID_DISPLAY_MULTI_ROD, MF_CHECKED);
                    CheckMenuItem(GetSubMenu(GetMenu(hwnd), 0), ID_DISPLAY_SINGLE_ROD, MF_UNCHECKED);
                    multiMode = true;
                    AdjustLayout(hwnd);
                    break;

                // 音效设置处理
                case ID_SOUND_FISH_HOOKED:
                case ID_SOUND_FISH_SPAWN:
                case ID_SOUND_MARINE:
                    // 切换音效开关，不互斥
                    if (GetMenuState(GetSubMenu(GetMenu(hwnd), 1), LOWORD(wParam), MF_BYCOMMAND) & MF_CHECKED)
                        CheckMenuItem(GetSubMenu(GetMenu(hwnd), 1), LOWORD(wParam), MF_UNCHECKED);
                    else
                        CheckMenuItem(GetSubMenu(GetMenu(hwnd), 1), LOWORD(wParam), MF_CHECKED);
                    break;

                case ID_COLOR_FISH_HOOKED:
                case ID_COLOR_FISH_SPAWN:
                case ID_COLOR_MARINE:
                    // 切换颜色开关，不互斥
                    if (GetMenuState(GetSubMenu(GetMenu(hwnd), 2), LOWORD(wParam), MF_BYCOMMAND) & MF_CHECKED)
                        CheckMenuItem(GetSubMenu(GetMenu(hwnd), 2), LOWORD(wParam), MF_UNCHECKED);
                    else
                        CheckMenuItem(GetSubMenu(GetMenu(hwnd), 2), LOWORD(wParam), MF_CHECKED);
                    break;


                case ID_PROMICRO_SEND:
                case ID_PROMICRO_AUTO:
                    // 切换调试模式开关
                    if (GetMenuState(GetSubMenu(GetMenu(hwnd), 4), LOWORD(wParam), MF_BYCOMMAND) & MF_CHECKED)
                        CheckMenuItem(GetSubMenu(GetMenu(hwnd), 4), LOWORD(wParam), MF_UNCHECKED);
                    else
                        CheckMenuItem(GetSubMenu(GetMenu(hwnd), 4), LOWORD(wParam), MF_CHECKED);
                    break;

                case ID_MARINE_DISTANCE_MARKER: {
                    if ((GetMenuState(GetSubMenu(GetMenu(hwnd), 3), ID_MARINE_DISTANCE_MARKER, MF_BYCOMMAND) & MF_CHECKED) == 0) {
                        ShowInputDialog(GetModuleHandleW(NULL), hwnd);
                        meter <= 10.0 ? meter = 10.0 : 0; // 最小值限制
                        swprintf(newText, 64, L"首次到达[%.1fm]时", meter);
                        UpdateMenuText(hwnd, newText);
                    }
                    CheckMenuItem(GetSubMenu(GetMenu(hwnd), 3), ID_MARINE_DISTANCE_MARKER, MF_CHECKED);
                    CheckMenuItem(GetSubMenu(GetMenu(hwnd), 3), ID_MARINE_BOTTOM_MOTION, MF_UNCHECKED);

                    break;
                }
                case ID_MARINE_BOTTOM_MOTION:
                    CheckMenuItem(GetSubMenu(GetMenu(hwnd), 3), ID_MARINE_DISTANCE_MARKER, MF_UNCHECKED);
                    CheckMenuItem(GetSubMenu(GetMenu(hwnd), 3), ID_MARINE_BOTTOM_MOTION, MF_CHECKED);
                    break;
            }
            break;
        }

        case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            HWND hwndStatic = (HWND)lParam;

            if (hwndStatic == hGridSingle) {
                SetBkMode(hdcStatic, TRANSPARENT);
                return (LRESULT)current_hb_single; // 返回对应的 HBRUSH
            }
            for (int i = 0; i < 3; ++i) {
                if (hwndStatic == hGridMulti[i]) {
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return (LRESULT)current_hb_multi[i]; // 返回对应的 HBRUSH
                }
            }
            break;
        }
            
        case WM_DESTROY:
            ShutdownGDIPlus();
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}