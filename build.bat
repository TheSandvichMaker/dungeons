@echo off

set SHARED_FLAGS=-g -gcodeview -ffast-math -W -Wall -Wextra -Werror -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-parameter -Wno-unused-variable -Wno-writable-strings -Wno-missing-field-initializers -Wno-c99-designator -DUNICODE=1 -Icode\external
set DEBUG_FLAGS=-O0 -DDUNGEONS_INTERNAL=1 -DDUNGEONS_SLOW=1 
set RELEASE_FLAGS=-O3
set LINKER_LIBRARIES=-luser32.lib -lgdi32.lib -ldwmapi.lib

ECHO]
if "%1" equ "release" (
    ECHO ------------------------------------------
    ECHO *** BUILDING RELEASE BUILD FROM SOURCE ***
    ECHO ------------------------------------------
    set FLAGS=%SHARED_FLAGS% %RELEASE_FLAGS%
) else (
    ECHO ----------------------------------------
    ECHO *** BUILDING DEBUG BUILD FROM SOURCE ***
    ECHO ----------------------------------------
    set FLAGS=%SHARED_FLAGS% %DEBUG_FLAGS%
)

misc\ctime.exe -begin build\dungeons.ctm

echo I'm here to be depressed and I'm not out of that at all > build\dungeons_lock.temp

clang code\dungeons.cpp %FLAGS% -DDUNGEONS_BUILD_DLL=1 -shared -o build\dungeons.dll 
clang code\win32_dungeons.cpp %FLAGS% -o build\win32_dungeons.exe %LINKER_LIBRARIES% 
set LAST_ERROR=%ERRORLEVEL%

del build\dungeons_lock.temp

misc\ctime.exe -end build\dungeons.ctm
