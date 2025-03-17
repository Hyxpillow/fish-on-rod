#include "rf4_parser.h"
#include "rf4_fish_name.h"

void rf4_parser_init() {
    if (wavList[0] == NULL) {
        wavList[0] = load_wav_file("./resource/1.wav");
        wavList[1] = load_wav_file("./resource/2.wav");
        wavList[2] = load_wav_file("./resource/3.wav");
    }
    valid_data_count = 0;
}

void rf4_parser(u_char* buffer, u_int size) {
    if ((*(u_short*)(buffer + 2)) == (u_short)0xFFFF) {
        valid_data_count++;
    }
    u_int opcode = *(u_int*)(buffer + 8);
    // save_packet(size, opcode);
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
    } else if (opcode == 0x03200100) { // 切换棒子 装配件 打窝杆入水
        u_int oprand = *(u_int*)(buffer + 0x14);
        if (oprand == 0x00950100 || oprand == 0x009D0100) { //杆子入水
            parse_rod_into_water_packet(buffer, size); 
        }
    } else if (opcode == 0x03210100) { // 收杆

    }
}


void parse_rod_packet(u_char* buffer, u_int size) {
    u_int shortcut = *(u_int*)(buffer + 0x0C);
    u_int rod_hash1 = *(u_int*)(buffer + 0x10);
    int rod_shortcut = 0;
    if (shortcut == 0x010000) {
        rod_shortcut = 1;
    } else if (shortcut == 0x020000) {
        rod_shortcut = 2;
    } else if (shortcut == 0x030000) {
        rod_shortcut = 3;
    } else {
        return;
    }
    if (rod_hash1 == 0) { //移除竿子，需要找到table中hotkey==rod_shortcut的，并设置hotkey=0
        for (auto it = rod_table.begin(); it != rod_table.end(); it++) {
            if (it->second.short_cut == rod_shortcut) {
                it->second.short_cut = 0;
            }
        }
    } else { //设置竿子快捷键
        set_rod_shortcut(rod_hash1, rod_shortcut);
    }
    UpdateUI();
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
        rod_table[rod_hash1].fish_name = fishname; // EN
    }
    rod_table[rod_hash1].fish_weight = fish_weight;

    if (get_rod_type(rod_hash1) != 3) { // 如果不是底部钓组 就响
        // Set BackColor
        PlaySound(wavList[rand() % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
    }
    UpdateUI();
}

void parse_fish_on_rod_packet(u_char* buffer, u_int size) {
    u_int rod_hash1 = *(u_int*)(buffer + 0x2B);
    if (get_rod_type(rod_hash1) == 3) { // 如果是底部钓组 响并打印
        // Set BackColor
        PlaySound(wavList[rand() % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
    } 
    UpdateUI();
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
    
    int rod_type = get_rod_type(rod_hash1);
    if (rod_type != 0) return; // 竿子记录过了

    for (int i = 0; i < size - 1; i++) {
        if (buffer[i] == 0) buffer[i] = 1;
    }
    buffer[size - 1] = 0;
    if (strstr((const char*)buffer, "Float")) {
        rod_type = 1;
    } else if (strstr((const char*)buffer, "Lure")) {
        rod_type = 2;
    } else if (strstr((const char*)buffer, "Bottom")) {
        rod_type = 3;
    } else if (strstr((const char*)buffer, "Marine")) {
        rod_type = 4;
    }
    set_rod_type(rod_hash1, rod_type);
    UpdateUI();
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

void set_rod_type(u_int rod_hash, u_int rod_type) {
    if(auto it = rod_table.find(rod_hash); it != rod_table.end()) {
        it->second.rod_type = rod_type;
    } else {
        rod_table[rod_hash] = FishingRod();
        rod_table[rod_hash].rod_type = rod_type;
    }
}

void set_rod_shortcut(u_int rod_hash, u_int rod_shortcut) {
    if(auto it = rod_table.find(rod_hash); it != rod_table.end()) {
        it->second.short_cut = rod_shortcut;
    } else {
        rod_table[rod_hash] = FishingRod();
        rod_table[rod_hash].short_cut = rod_shortcut;
    }
}

u_int get_rod_type(u_int rod_hash) {
    if(auto it = rod_table.find(rod_hash); it != rod_table.end()) {
        return it->second.rod_type;
    }
    return 0;
}

u_int get_rod_shortcut(u_int rod_hash) {
    if(auto it = rod_table.find(rod_hash); it != rod_table.end()) {
        return it->second.short_cut;
    }
    return 0;
}