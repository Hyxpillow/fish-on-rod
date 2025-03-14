@echo off
@REM windres -i "icon.rc" -o "icon.o"
gcc .\src\rf4_sniffer.c .\resource\icon.o -static -lwpcap -lws2_32 -lgdi32 -lwinmm -o v3.exe -I"D:\hyx\npcap-sdk-1.15\Include" -L"D:\hyx\npcap-sdk-1.15\Lib\x64"
powershell Compress-Archive -Path v3.exe, wpcap.dll, Packet.dll, resource -DestinationPath fish-on-rod.zip -Update