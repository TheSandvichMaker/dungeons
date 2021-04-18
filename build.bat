@echo off

set SOURCE=code\win32_dungeons.cpp
set OUTPUT=build\dungeons.exe

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

set SHARED_FLAGS=-g -gcodeview -W -Wall -Wextra -Werror -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-parameter -Wno-unused-variable -Wno-writable-strings -Wno-missing-field-initializers -DUNICODE=1 -Icode\external -Icode\external\glad
set DEBUG_FLAGS=-O0 -DDEBUG_BUILD
set RELEASE_FLAGS=-O3
set LINKER_LIBRARIES=-luser32.lib -lgdi32.lib -ldwmapi.lib

if "%1" equ "release" (
    set FLAGS=%SHARED_FLAGS% %RELEASE_FLAGS%
) else (
    set FLAGS=%SHARED_FLAGS% %DEBUG_FLAGS%
)

clang %SOURCE% %FLAGS% -o %OUTPUT% %LINKER_LIBRARIES% 
set LAST_ERROR=%ERRORLEVEL%

misc\ctime.exe -end build\dungeons.ctm
