@echo off
@REM windres -i "icon.rc" -o "icon.o"
g++ .\src\*.cpp  .\resource\icon.o -static -lwpcap -lws2_32 -lgdi32 -lgdiplus -lwinmm -lcomctl32 -o v4.exe -I"D:\hyx\npcap-sdk-1.15\Include" -L"D:\hyx\npcap-sdk-1.15\Lib\x64"
powershell Compress-Archive -Path v4.exe, wpcap.dll, fishes, Packet.dll -DestinationPath fish-on-rod.zip -Update