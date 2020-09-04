@echo off
set curpath=%~dp0
@rem if defined KBE_ROOT (echo %KBE_ROOT%) else 
set KBE_ROOT=C:/TheWorld/KBEngine/kbengine/
@rem if defined KBE_RES_PATH (echo %KBE_RES_PATH%) else 
set KBE_RES_PATH=C:/TheWorld/KBEngine/kbengine/kbe/res/;C:/TheWorld/Client/TheWorld_Assets/;C:/TheWorld/Client/TheWorld_Assets/scripts/;C:/TheWorld/Client/TheWorld_Assets/res/
set KBE_BIN_PATH=%curpath%

@echo KBE_ROOT=%KBE_ROOT%
@echo KBE_RES_PATH=%KBE_RES_PATH%
@echo KBE_BIN_PATH=%KBE_BIN_PATH%

del /Q %KBE_BIN_PATH%\log.log

cd /D %curpath%
start TheWorld_ConsoleClient_d.exe
