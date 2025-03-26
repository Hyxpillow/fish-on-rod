#include <iostream>
#include <pcap.h>
#include <string>
#include <vector>
#include <thread>
#include <fstream>

#include "packet_handler.h"
#include "ui.h"
#include "wav_data.h"

void StartPcapLoop(pcap_t* handle) {
    while (!initialed) {}
    UI_reset();
    local_parser.init();
    remote_parser.init();
    wav = std::vector<char>(wavData, wavData + wavDataSize);
    pcap_loop(handle, 0, packet_handler, NULL);
}

int main() {
    // 选择网口
    setlocale(LC_ALL, "en_US.UTF-8");
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs, *dev;
    std::vector<pcap_if_t*> devices;
    int i = 0;
    
    // 获取所有网络接口
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "查找设备失败: " << errbuf << std::endl;
        return 1;
    }
    
    // 显示网络接口列表
    std::cout << "可用网络接口:\n";
    for (dev = alldevs; dev; dev = dev->next) {
        std::cout << ++i << ". " << (dev->description ? dev->description : dev->name) << std::endl;
        devices.push_back(dev);
    }
    
    // 检查是否找到了设备
    if (i == 0) {
        std::cerr << "未找到网络接口\n";
        pcap_freealldevs(alldevs);
        return 1;
    }
    
    // 用户选择接口
    std::cout << "选择网口号 (1-" << i << "): ";
    std::cin >> dev_choice;
    
    // 验证选择
    if (dev_choice < 1 || dev_choice > i) {
        std::cerr << "无效选择\n";
        pcap_freealldevs(alldevs);
        return 1;
    }
    snprintf(dev_description, 252, "%s", devices[dev_choice-1]->description);
    // 打开选择的设备
    pcap_t *handle = pcap_open_live(devices[dev_choice-1]->name, 65536, 1, 1000, errbuf);
    if (handle == NULL) {
        std::cerr << "无法打开设备: " << errbuf << std::endl;
        pcap_freealldevs(alldevs);
        return 1;
    }
    pcap_freealldevs(alldevs);

    std::thread t1(StartPcapLoop, handle);
    t1.detach();

    startWinUI();
    
    pcap_breakloop(handle);
    pcap_close(handle);
    return 0;
}