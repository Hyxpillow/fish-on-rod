#include <iostream>
#include <windows.h>  // Windows API
#include <string>

// 设备名称和波特率
const char* DEVICE_NAME = "\\\\.\\COM11";  // 替换为您的串口名称
const int BAUD_RATE = 9600;

// 全局串口句柄
HANDLE g_hSerial = INVALID_HANDLE_VALUE;
// 上次尝试连接的时间戳，用于限制重连频率
DWORD g_lastConnectAttempt = 0;
// 重连尝试的最小间隔（毫秒）
const DWORD RECONNECT_INTERVAL = 1000;

// 关闭串口连接
void close_serial() {
    if (g_hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hSerial);
        g_hSerial = INVALID_HANDLE_VALUE;
    }
}

// 初始化串口连接
bool initialize_serial(bool forceReconnect = false) {
    // 如果强制重连，则关闭现有连接
    if (forceReconnect && g_hSerial != INVALID_HANDLE_VALUE) {
        close_serial();
    }
    
    // 如果已经初始化且有效，直接返回成功
    if (g_hSerial != INVALID_HANDLE_VALUE) {
        // 简单检查连接是否有效
        DWORD errors;
        COMSTAT comStat;
        if (ClearCommError(g_hSerial, &errors, &comStat)) {
            return true;  // 连接正常
        } else {
            // 连接已失效，关闭它
            close_serial();
        }
    }
    
    // 限制重连频率
    DWORD currentTime = GetTickCount();
    if (!forceReconnect && (currentTime - g_lastConnectAttempt < RECONNECT_INTERVAL)) {
        return false;  // 重连太频繁，暂时不尝试
    }
    
    g_lastConnectAttempt = currentTime;
    
    // 打开串口
    g_hSerial = CreateFileA(
        DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    // 如果无法打开串口，返回失败
    if (g_hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 配置串口参数
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(g_hSerial, &dcbSerialParams)) {
        close_serial();
        return false;
    }

    dcbSerialParams.BaudRate = BAUD_RATE;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(g_hSerial, &dcbSerialParams)) {
        close_serial();
        return false;
    }

    // 设置超时
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(g_hSerial, &timeouts)) {
        close_serial();
        return false;
    }

    return true;
}

// 通用的信号发送函数
void try_send(char signal) {
    // 尝试初始化串口，首先尝试现有连接
    if (!initialize_serial()) {
        // 如果连接失败，尝试强制重连一次
        if (!initialize_serial(true)) {
            return;  // 如果重连也失败，直接返回
        }
    }

    // 发送指定信号
    char szBuff[1] = {signal};
    DWORD dwBytesWritten = 0;
    
    // 尝试写入
    if (!WriteFile(g_hSerial, szBuff, 1, &dwBytesWritten, NULL)) {
        // 写入失败，可能是连接断开，下次发送时会尝试重连
        close_serial();
    }
}