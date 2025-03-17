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
    }
    UpdateUI();
}


void parse_rod_packet(u_char* buffer, u_int size) {
    u_int shortcut = *(u_int*)(buffer + 0x0C);

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
    u_int rod_hash1 = *(u_int*)(buffer + 0x10);
    set_rod_shortcut(rod_hash1, rod_shortcut);
    // if (rod_hash1 != 0) {
    //     printf("%02d:%02d:%02d  设置%d号竿 [%08X]\n",tm_info->tm_hour,tm_info->tm_min, tm_info->tm_sec, rod_shortcut, rod_hash1);
    //     set_rod_shortcut(rod_hash1, rod_shortcut);
    // } else {
    //     printf("%02d:%02d:%02d  移除%d号竿\n",tm_info->tm_hour,tm_info->tm_min, tm_info->tm_sec, rod_shortcut);
    // }
}

void parse_fish_packet(u_char* buffer, u_int size) {
    // u_int rod_hash1 = *(u_int*)(buffer + 0x13);

    // int rod = get_rod_shortcut(rod_hash1);
    // int rod_color = 0x0F;
    // if (rod == 1) {
    //     rod_color = 0x0C;
    // } else if (rod == 2) {
    //     rod_color = 0x0B;
    // } else if (rod == 3) {
    //     rod_color = 0x0E;
    // }

    // u_char fish_name_len = buffer[0x38];
    // char fishname[64];
    // memcpy(fishname, buffer + 0x39, fish_name_len);
    // fishname[fish_name_len] = 0;

    // float fish_weight = *(float*)(buffer + 62 + fish_name_len);

    // if (fish_table.find(fishname) != fish_table.end()) {
    //     const Fish_Data& fish = fish_table[fishname];
    // }

    // const char* to_be_print_fish_name = "未知";
    // int back_color = 0x00;
    // int font_color = 0x0F;
    // if (fish_data) {
    //     to_be_print_fish_name = fish_data->name;
    //     if ((double)fish_weight * 1000 >= (double)fish_data->trophy) {back_color = 0xE0; font_color = 0x00;}
    //     if ((double)fish_weight * 1000 >= (double)fish_data->super_trophy) back_color = 0x90;
    // }
    
    // time_t t;
    // struct tm *tm_info;
    // time(&t);
    // tm_info = localtime(&t);
    // SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), back_color | font_color);
    // printf("%02d:%02d:%02d    ",
    //     tm_info->tm_hour,
    //     tm_info->tm_min,
    //     tm_info->tm_sec);

    // if (get_rod_type(rod_hash1) != 3) { // 如果不是底部钓组 就响
    //     PlaySound(wavList[rand() % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
    // } else {
    //     rod_color = font_color; //如果是底部钓组，不显示钓竿颜色
    //     printf("等待咬钩    ");
    // }
    

    // SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), back_color | rod_color);
    // printf("[%d号竿]", rod);
    // SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), back_color | font_color);
    // printf("    %.3f kg    %s\n", 
    //     fish_weight,
    //     to_be_print_fish_name);
    // SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
}

void parse_fish_on_rod_packet(u_char* buffer, u_int size) {
    // u_int rod_hash1 = *(u_int*)(buffer + 0x2B);

    // if (get_rod_type(rod_hash1) == 3) { // 如果是底部钓组 响并打印
    //     PlaySound(wavList[rand() % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
    // } else {
    //     return; // 如果是其他钓组，
    // }

    // int rod = get_rod_shortcut(rod_hash1);
    // int rod_color = 0x0F;
    // if (rod == 1) {
    //     rod_color = 0x0C;
    // } else if (rod == 2) {
    //     rod_color = 0x0B;
    // } else if (rod == 3) {
    //     rod_color = 0x0E;
    // }


    // time_t t;
    // struct tm *tm_info;
    // time(&t);
    // tm_info = localtime(&t);
    // printf("%02d:%02d:%02d    ",
    //     tm_info->tm_hour,
    //     tm_info->tm_min,
    //     tm_info->tm_sec);
    // SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), rod_color);
    // printf("[%d号竿] 鱼咬钩了\n", rod);
    // SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
}

void parse_rod_into_water_packet(u_char* buffer, u_int size) {
    u_int rod_hash1;
    u_int oprand = *(u_int*)(buffer + 0x14);
    if (oprand == 0x00950100) {
        rod_hash1 = *(u_int*)(buffer + 0x218);
    } else if (oprand == 0x009D0100) {
        rod_hash1 = *(u_int*)(buffer + 0x30);
    } else {
       return; 
    }  
    // 如果竿子hash没见过
    
    int rod_type = get_rod_type(rod_hash1);
    if (rod_type != 0) return;

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
    // time_t t;
    // struct tm *tm_info;
    // time(&t);
    // tm_info = localtime(&t);
    // printf("%02d:%02d:%02d  ",
    //     tm_info->tm_hour,
    //     tm_info->tm_min,
    //     tm_info->tm_sec);
    // if (rod_type == 1) {
    //     printf("鱼竿[%08X]类型为[浮子]\n", rod_hash1);
    // } else if (rod_type == 2) {
    //     printf("鱼竿[%08X]类型为[路亚]\n", rod_hash1);
    // } else if (rod_type == 3) {
    //     printf("鱼竿[%08X]类型为[水底]\n", rod_hash1);
    // } else if (rod_type == 4) {
    //     printf("鱼竿[%08X]类型为[海钓]\n", rod_hash1);
    // } else {
    //     printf("鱼竿[%08X]类型为[未知钓法]\n", rod_hash1);
    // }
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
        it->second.first = rod_type;
    } else {
        rod_table[rod_hash] = std::pair<u_int, u_int>(rod_type, 0);
    }
}

void set_rod_shortcut(u_int rod_hash, u_int rod_shortcut) {
    if(auto it = rod_table.find(rod_hash); it != rod_table.end()) {
        it->second.second = rod_shortcut;
    } else {
        rod_table[rod_hash] = std::pair<u_int, u_int>(0, rod_shortcut);
    }
}

u_int get_rod_type(u_int rod_hash) {
    if(auto it = rod_table.find(rod_hash); it != rod_table.end()) {
        return it->second.first;
    }
    return 0;
}

u_int get_rod_shortcut(u_int rod_hash) {
    if(auto it = rod_table.find(rod_hash); it != rod_table.end()) {
        return it->second.second;
    }
    return 0;
}