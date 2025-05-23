#include "rf4_parser.h"
#include "rf4_fish_name.h"

void rf4_parser_init() {
    if (wavList[0] == NULL) {
        wavList[0] = load_wav_file("./resource/1.wav");
        wavList[1] = load_wav_file("./resource/2.wav");
        wavList[2] = load_wav_file("./resource/3.wav");
    }
    valid_data_count = 0;
    rod_table.clear();
    UI_reset();
}
void save_packet(u_char* buffer, int size, u_int opcode) {
    char filename[256];
    // snprintf(filename, sizeof(filename), "./log/%08X", opcode);
    // CreateDirectoryA(filename, NULL);
    snprintf(filename, sizeof(filename), "./log/%lld_%08X_%d.log", valid_data_count, opcode, size);
    FILE* file_ptr = fopen(filename, "wb+");
    fwrite(buffer, sizeof(u_char), size, file_ptr);
    fclose(file_ptr);
    file_ptr = NULL;
}

bool contains_value(const u_char* buffer, u_int size, u_int value) {
    for (u_int i = 0; i <= size - sizeof(u_int); ++i) {
        if (memcmp(buffer + i, &value, sizeof(u_int)) == 0) {
            return true;
        }
    }
    return false;
}

void rf4_parser(u_char* buffer, u_int size) {
    WCHAR cellText[256];
    valid_data_count++;
    if ((*(u_short*)(buffer + 2)) == (u_short)0xFFFF) {
        swprintf(cellText, 256, L"已连接到服务器(状态:解密正常 总计:%d)", valid_data_count & 0xFFFF);
    } else {
        swprintf(cellText, 256, L"解密失败");
    }
    UpdateStatus(cellText);
    u_int opcode = *(u_int*)(buffer + 8);
    // if (opcode != 0x03131800 
    //     && opcode != 0x03090300) // 消息
    //     save_packet(buffer, size, opcode);
        // 0x03 == *(u_char*)(buffer + 11) 

        // 0x03090300 发送定时信息
        // 03131800 发送 确认消息

        // 03010E00 抛竿
        // 03020E00 入水？
        // 03030E00 
        // 03040E00 收杆
        // 03050E00 入户
        // 03060E00 
        // 03070E00 在水中 位置改变
        // 03080E00 拉鱼
        // 03090E00 
        // 03010C00 打窝 

    // 01920100 切换场景
    // 03200100 吃东西
    // 031b1800 收消息
    // 37_3A9B0100_3528 开船
    //685_009E0100_54 抛竿
    // 03210100 手杆
    // 03200100 鱼入户 切换棒子 装配件 打窝杆入水 竿子入水
    // 03010200 快照
    if (opcode == 0x030E0E00) { //刷新鱼    
        parse_fish_packet(buffer, size); 
    } else if (opcode == 0x00880100) { //快捷键
        parse_rod_packet(buffer, size); 
    } else if (opcode == 0x03160400) {  
        u_int oprand1 = *(u_int*)(buffer + 12);
        u_int oprand2 = *(u_int*)(buffer + 16);
        if (oprand1 == 0x01323602 && oprand2 == 0x00830100) { //鱼咬钩
            parse_fish_on_rod_packet(buffer, size);
        } else { // 手抛窝子
    
        }
       
    } else if (opcode == 0x03200100) { // 
        u_int oprand = *(u_int*)(buffer + 0x14);
        if (oprand == 0x00950100 || oprand == 0x009D0100) { //杆子入水
            parse_rod_into_water_packet(buffer, size); 
        }
    } else if (opcode == 0x03210100) { // 收杆
        parse_rod_back_packet(buffer, size);
    }
}


void parse_rod_packet(u_char* buffer, u_int size) {
    u_int shortcut = *(u_int*)(buffer + 0x0C);
    u_int rod_hash1 = *(u_int*)(buffer + 0x10);
    int rod_short_cut = 0;
    if (shortcut == 0x010000) {
        rod_short_cut = 1;
    } else if (shortcut == 0x020000) {
        rod_short_cut = 2;
    } else if (shortcut == 0x030000) {
        rod_short_cut = 3;
    } else {
        return;
    }
    if (rod_hash1 == 0) { //移除竿子，需要找到table中hotkey==rod_short_cut的，并设置hotkey=0
        for (auto it = rod_table.begin(); it != rod_table.end(); it++) {
            if (it->second.short_cut == rod_short_cut) {
                it->second.short_cut = 0;
                WCHAR tmp[10] = L"--";
                UpdateText(rod_short_cut - 1, tmp);
                UpdateColor(rod_short_cut - 1, 0);
            }
        }
    } else { //设置竿子快捷键，如果竿子没见过，设置初始状态
        if (auto it = rod_table.find(rod_hash1); it == rod_table.end()) {
            swprintf(rod_table[rod_hash1].state, 256, L"就绪");
            rod_table[rod_hash1].color = 0;
            UpdateText(rod_short_cut - 1, rod_table[rod_hash1].state);
            UpdateColor(rod_short_cut - 1, rod_table[rod_hash1].color);
        } else { //如果见过了，就替换为
            UpdateText(rod_short_cut - 1, it->second.state);
            UpdateColor(rod_short_cut - 1, it->second.color);
        }
        rod_table[rod_hash1].short_cut = rod_short_cut;
    }
}

void parse_fish_packet(u_char* buffer, u_int size) {
    u_int rod_hash1 = *(u_int*)(buffer + 0x13);

    u_char fish_name_len = buffer[0x38];
    char fishname[64];
    memcpy(fishname, buffer + 0x39, fish_name_len);
    fishname[fish_name_len] = 0;

    float fish_weight = *(float*)(buffer + 62 + fish_name_len);

    if (fish_table.find(fishname) != fish_table.end()) {
        const Fish_Data& fish = fish_table[fishname];
        rod_table[rod_hash1].fish_name = fish.name; // CN
    } else {
        rod_table[rod_hash1].fish_name = "未知"; // EN
    }
    rod_table[rod_hash1].fish_weight = fish_weight;
    swprintf(rod_table[rod_hash1].state, 256, L"等待咬钩 (%s, %.3fkg)", rod_table[rod_hash1].fish_name, fish_weight);

    int rod_type = rod_table[rod_hash1].rod_type;
    int rod_short_cut = rod_table[rod_hash1].short_cut;
    rod_table[rod_hash1].color = 1;

    if (rod_short_cut != 0) {
        if (rod_type != 3) { // 如果不是底部钓组 就响
            UpdateColor(rod_short_cut - 1, 1);
            PlaySound(wavList[rand() % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
        }
        UpdateText(rod_short_cut - 1, rod_table[rod_hash1].state);
    }
}

void parse_fish_on_rod_packet(u_char* buffer, u_int size) {
    u_int rod_hash1 = *(u_int*)(buffer + 0x2B);
    swprintf(rod_table[rod_hash1].state, 256, L"咬钩了 (%s, %.3fkg)", rod_table[rod_hash1].fish_name, rod_table[rod_hash1].fish_weight);
    int rod_type = rod_table[rod_hash1].rod_type;
    int rod_short_cut = rod_table[rod_hash1].short_cut;
    rod_table[rod_hash1].color = 1;

    if (rod_short_cut != 0) {
        if (rod_type == 3) { // 如果是底部钓组 就响
            UpdateColor(rod_short_cut - 1, 1);
            PlaySound(wavList[rand() % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
        }
        UpdateText(rod_short_cut - 1, rod_table[rod_hash1].state);
    }
}

void parse_rod_into_water_packet(u_char* buffer, u_int size) {
    u_int rod_hash1;
    u_int oprand = *(u_int*)(buffer + 0x14);
    if (oprand == 0x00950100) { // 其他
        rod_hash1 = *(u_int*)(buffer + 0x218);
    } else if (oprand == 0x009D0100) { //浮子
        rod_hash1 = *(u_int*)(buffer + 0x30);
    } else {
       return; 
    }  
    bool seen = rod_table.find(rod_hash1) != rod_table.end();
    if (!seen) {
        rod_table[rod_hash1].short_cut = 0;
    }
    if (rod_table[rod_hash1].rod_type == 0) {
        for (int i = 0; i < size - 1; i++) {
            if (buffer[i] == 0) buffer[i] = 1;
        }
        buffer[size - 1] = 0;
        if (strstr((const char*)buffer, "Float")) {
            rod_table[rod_hash1].rod_type = 1;
        } else if (strstr((const char*)buffer, "Lure")) {
            rod_table[rod_hash1].rod_type = 2;
        } else if (strstr((const char*)buffer, "Bottom")) {
            rod_table[rod_hash1].rod_type = 3;
        } else if (strstr((const char*)buffer, "Marine")) {
            rod_table[rod_hash1].rod_type = 4;
        }
    }
    swprintf(rod_table[rod_hash1].state, 256, L"在水中");
    int rod_short_cut = rod_table[rod_hash1].short_cut;
    rod_table[rod_hash1].color = 0;

    if (rod_short_cut != 0) {
        UpdateColor(rod_short_cut - 1, 0);
        UpdateText(rod_short_cut - 1, rod_table[rod_hash1].state);
    }
}

void parse_rod_back_packet(u_char* buffer, u_int size) {
    for (auto it = rod_table.begin(); it != rod_table.end(); ++it) {
        if (contains_value(buffer, size, it->first)) {
            swprintf(it->second.state, 256, L"就绪");
            it->second.color = 0;
            int rod_short_cut = it->second.short_cut;
            if (rod_short_cut != 0) {
                UpdateColor(rod_short_cut - 1, 0);
                UpdateText(rod_short_cut - 1, it->second.state);
            }
            break;
        }
    }
}

char* load_wav_file(const char* filename) {
    size_t size = 0;
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("%s 不存在\n", filename);
        system("pause");
        exit(1);
    };

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0'; // Null-terminate for safety if it's a text file

    fclose(file);
    return buffer;
}
