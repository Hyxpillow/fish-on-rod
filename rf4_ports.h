#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <iphlpapi.h>

DWORD GetProcessIdByName(const char* processName);
int GetTcpConnectionsForPid(DWORD pid, int** localPorts);