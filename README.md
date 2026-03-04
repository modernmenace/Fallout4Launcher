# Custom Fallout 4 Launcher

This program was created beacuse I run the game from a few different monitors, and use Steam's remote play to stream to another device. 

Unfortunately changing the resolution regularly is hard to do, because F4SE runs without a launcher and changing the resolution is not available from the game itself.

This program automatically detects the primary display's resolution before startup, makes the adjustment in the game's INI file, and then attempts to launch F4SE if it is available. If F4SE is not found it launches the vanilla Fallout4.exe.

The launcher also outputs a log located in the fallout 4 directory called launcher_log.txt detailing any errors that may occur (the game will still launch regardless).

# Instructions
1. backup Fallout4Launcher.exe in your Fallout4 directory
2. Rename the built file Fallout4Launcher.exe
3. Replace Fallout4Launcher.exe in the game's directory
