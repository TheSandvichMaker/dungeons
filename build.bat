@echo off

set SOURCE=code\win32_dungeons.cpp code\dungeons.cpp
set OUTPUT=build\dungeons.exe

set SHARED_FLAGS=-g -gcodeview -ffast-math -W -Wall -Wextra -Werror -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-parameter -Wno-unused-variable -Wno-writable-strings -Wno-missing-field-initializers -Wno-c99-designator -DUNICODE=1 -Icode\external
set DEBUG_FLAGS=-O0 -DDUNGEONS_INTERNAL=1 -DDUNGEONS_SLOW=1
set RELEASE_FLAGS=-O3
set LINKER_LIBRARIES=-luser32.lib -lgdi32.lib -ldwmapi.lib

if "%1" equ "release" (
    set FLAGS=%SHARED_FLAGS% %RELEASE_FLAGS%
) else (
    set FLAGS=%SHARED_FLAGS% %DEBUG_FLAGS%
)

misc\ctime.exe -begin build\dungeons.ctm

ECHO]
if "%1" equ "release" (
    ECHO ------------------------------------------
    ECHO *** BUILDING RELEASE BUILD FROM SOURCE ***
    ECHO ------------------------------------------
) else (
    ECHO ----------------------------------------
    ECHO *** BUILDING DEBUG BUILD FROM SOURCE ***
    ECHO ----------------------------------------
)

clang %SOURCE% %FLAGS% -o %OUTPUT% %LINKER_LIBRARIES% 
set LAST_ERROR=%ERRORLEVEL%

misc\ctime.exe -end build\dungeons.ctm
