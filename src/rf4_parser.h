#pragma once // 负责站在应用层的视角处理俄钓数据包
#include <winsock2.h>
#include <windows.h>
#include <unordered_map>
#include <iostream>
#include "ui.h"


static char* wavList[3];
static u_int64 valid_data_count;

void rf4_parser_init();
void rf4_parser(u_char* buffer, u_int size);
void parse_rod_packet(u_char* buffer, u_int size);
void parse_fish_packet(u_char* buffer, u_int size);
void parse_fish_on_rod_packet(u_char* buffer, u_int size);
void parse_rod_into_water_packet(u_char* buffer, u_int size);
void parse_rod_back_packet(u_char* buffer, u_int size);

char* load_wav_file(const char* filename);