@echo off
gcc .\rf4_sniffer.c .\rf4_ports.c .\icon.o -static -lwpcap -liphlpapi -lws2_32 -lwinmm -o v2.exe -I"D:\hyx\npcap-sdk-1.15\Include" -L"D:\hyx\npcap-sdk-1.15\Lib\x64"
powershell Compress-Archive -Path v2.exe, wpcap.dll, Packet.dll, resource, data -DestinationPath fish-on-rod.zip -Update