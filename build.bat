@echo off
setlocal enabledelayedexpansion

rem Move to script directory to avoid %~dp0 trailing-backslash issues
pushd "%~dp0" >nul 2>&1
set "ROOT_DIR=%CD%"
set "BUILD_DIR=%ROOT_DIR%\build"

rem default generator; prefer NMake Makefiles (no MSBuild). Can be overridden by passing a generator as first arg
set "GENERATOR=NMake Makefiles"
if not "%~1"=="" set "GENERATOR=%~1"

echo Running: cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%"
rem Ensure MSVC environment is initialized for x64 if cl is not already on PATH
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
  set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if defined VSINSTALL (
      set "VCVARS=%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat"
      if exist "%VCVARS%" (
        echo Initializing MSVC environment (vcvarsall amd64)...
        call "%VCVARS%" amd64
      )
    )
  ) else (
    echo Warning: 'cl' not found and vswhere.exe not present; ensure Visual Studio Developer Tools are available.
  )
)

cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%"
if %ERRORLEVEL% neq 0 (
  echo CMake configure with generator "%GENERATOR%" failed.
  echo Trying fallback generator: Ninja
  cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -G "Ninja"
  if %ERRORLEVEL% neq 0 (
    echo Ninja configure failed.
    echo All configure attempts failed. Install NMake or Ninja, or run CMake with an appropriate -G option.
    exit /b %ERRORLEVEL%
  ) else (
    set "GENERATOR=Ninja"
  )
)

echo Building with generator: %GENERATOR%
cmake --build "%BUILD_DIR%" --config Release
if %ERRORLEVEL% neq 0 (
  echo Build with --config Release failed; trying default build command for single-config generators.
  cmake --build "%BUILD_DIR%"
  if %ERRORLEVEL% neq 0 (
    echo Build failed.
    exit /b %ERRORLEVEL%
  )
)

rem Copy the built executables to dist\bin\1.0
set "OUT_DIR=%ROOT_DIR%\dist\bin\1.0"
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

set "BUILD_RELEASE_PATH=%BUILD_DIR%\bin\Release"
if exist "%BUILD_RELEASE_PATH%\koxl.exe" (
  copy /Y "%BUILD_RELEASE_PATH%\koxl.exe" "%OUT_DIR%\" >nul 2>&1
) else (
  copy /Y "%BUILD_DIR%\Release\koxl.exe" "%OUT_DIR%\" >nul 2>&1
)

rem If the copies above didn't find the executables (single-config generators), search recursively
if not exist "%OUT_DIR%\koxl.exe" (
  for /R "%BUILD_DIR%" %%F in (koxl.exe) do (
    copy /Y "%%F" "%OUT_DIR%\" >nul 2>&1
    goto :_koxl_found
  )
  :_koxl_found
)

echo Build complete. Executables copied to %OUT_DIR%
endlocal
