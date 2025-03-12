#include "rf4_ports.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// Function to find process ID by name
DWORD GetProcessIdByName(const char* processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);
        
        if (Process32First(snapshot, &processEntry)) {
            do {
                if (stricmp(processEntry.szExeFile, processName) == 0) {
                    pid = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    
    return pid;
}

// Function to get TCP connections for a specific PID
int GetTcpConnectionsForPid(DWORD pid, int** localPorts) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 0;
    }
    
    // Get the table of TCP connections
    MIB_TCPTABLE_OWNER_PID* pTcpTable = NULL;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    int ret = 0;
    
    // Make an initial call to GetExtendedTcpTable to get the necessary size
    dwRetVal = GetExtendedTcpTable(NULL, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
        pTcpTable = (MIB_TCPTABLE_OWNER_PID*)malloc(dwSize);
        if (pTcpTable == NULL) {
            printf("Error allocating memory\n");
            WSACleanup();
            return 0;
        }
    }
    
    // Make a second call to GetExtendedTcpTable to get the actual data
    dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (dwRetVal == NO_ERROR) {
        // printf("TCP Ports for PID %lu:\n", pid);
        DWORD tmpPort[1000] = {0};
        // Loop through the TCP connections
        for (DWORD i = 0; i < pTcpTable->dwNumEntries && i < 1000; i++) {
            if (pTcpTable->table[i].dwOwningPid == pid) {
                // Convert the port from network byte order to host byte order
                DWORD localPort = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
                DWORD remotePort = ntohs((u_short)pTcpTable->table[i].dwRemotePort);
                
                if (remotePort == 80) 
                    continue;
                tmpPort[ret] = localPort;
                ret++;
            }
        }
        if (ret == 0) {
            printf("No TCP connections found for this process.\n");
            return 0;
        }
        *localPorts = (int*)malloc(ret * sizeof(int));
        for (int i = 0; i < ret; i++) {
            (*localPorts)[i] = tmpPort[i];
        }

    } else {
        printf("GetExtendedTcpTable failed with error: %lu\n", dwRetVal);
    }
    
    // Clean up
    if (pTcpTable != NULL) {
        free(pTcpTable);
        pTcpTable = NULL;
    }
    
    WSACleanup();
    return ret;
}