@echo off

set SHARED_FLAGS=/nologo /Zo /Zi /Oi /GR- /EHa- /WX /W4 /wd4201 /fp:fast /fp:except- -DUNICODE=1 -D_CRT_SECURE_NO_WARNINGS
set MSVC_FLAGS=-DCOMPILER_MSVC=1 
set LLVM_FLAGS=-Wno-missing-field-initializers -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-writable-strings -Wno-missing-field-initializers -Wno-missing-braces -DCOMPILER_LLVM=1
set SANITIZE_FLAGS=/Od /MT -DDUNGEONS_INTERNAL=1 -DDUNGEONS_SLOW=1 -fsanitize=address
set DEBUG_FLAGS=/Od /MTd -DDUNGEONS_INTERNAL=1 -DDUNGEONS_SLOW=1 
set RELEASE_FLAGS=/O2 /MT
set LINKER_FLAGS=/opt:ref /incremental:no
set LINKER_LIBRARIES=user32.lib gdi32.lib dwmapi.lib

set FLAGS=%SHARED_FLAGS%
echo]
if "%1" equ "release" (
    echo ------------------------------------------
    echo *** BUILDING RELEASE BUILD FROM SOURCE ***
    echo ------------------------------------------
    set FLAGS=%FLAGS% %RELEASE_FLAGS%
) else if "%1" equ "sanitize" (
    echo --------------------------------------------
    echo *** BUILDING SANITIZED BUILD FROM SOURCE ***
    echo --------------------------------------------
    set FLAGS=%FLAGS% %SANITIZE_FLAGS%
) else (
    echo ----------------------------------------
    echo *** BUILDING DEBUG BUILD FROM SOURCE ***
    echo ----------------------------------------
    set FLAGS=%FLAGS% %DEBUG_FLAGS%
)

misc\ctime.exe -begin ctm\dungeons.ctm

echo I'm here to be depressed and I'm not out of that at all > build\dungeons_lock.temp

if "%DUNGEONS_USE_LLVM%" equ "1" goto build_llvm

:build_msvc
pushd build
echo COMPILER: CL
cl ..\code\dungeons.cpp -DDUNGEONS_BUILD_DLL=1 %FLAGS% %MSVC_FLAGS% /Fe"dungeons.dll" /LD /link %LINKER_FLAGS%
cl ..\code\win32_dungeons.cpp %FLAGS% %MSVC_FLAGS% /Fe"win32_dungeons.exe" %LINKER_LIBRARIES% 
popd
goto build_finished

:build_llvm
pushd build
echo COMPILER: CLANG-CL
clang-cl ..\code\dungeons.cpp -DDUNGEONS_BUILD_DLL=1 %FLAGS% %LLVM_FLAGS% /Fe"dungeons.dll" /LD /link %LINKER_FLAGS%
clang-cl ..\code\win32_dungeons.cpp %FLAGS% %LLVM_FLAGS% /Fe"win32_dungeons.exe" %LINKER_LIBRARIES% 
popd
goto build_finished

:build_finished

set LAST_ERROR=%ERRORLEVEL%
del build\dungeons_lock.temp

misc\ctime.exe -end ctm\dungeons.ctm
