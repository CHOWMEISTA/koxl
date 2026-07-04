#!/usr/bin/env pwsh
Param()

$root = Split-Path -Parent $MyInvocation.MyCommand.Definition
$src = Join-Path $root 'src\koxlc.cpp'
$outdir = Join-Path $root 'dist\bin\1.0'
$exe = Join-Path $root 'koxl.exe'
New-Item -Path $outdir -ItemType Directory -Force | Out-Null

Write-Host "Building koxl from: $src"
Write-Host "Output executable: $exe"
if (-not (Test-Path $src)) {
  Write-Error "Source not found: $src"
  exit 1
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsinst = $null
if (Test-Path $vswhere) {
  $vsinst = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
}

if ($vsinst) {
  $vcvars = Join-Path $vsinst 'VC\Auxiliary\Build\vcvarsall.bat'
  if (-not (Test-Path $vcvars)) {
    Write-Error "vcvarsall not found at $vcvars"
    exit 1
  }
  $cmd = "`"$vcvars`" amd64 && cl /nologo /EHsc /std:c++17 `"$src`" /Fe`"$exe`""
  Write-Host "Invoking: cmd.exe /c $vcvars amd64 && cl ..."
  cmd.exe /c $cmd
  exit $LASTEXITCODE
} else {
  Write-Host "vswhere not found or Visual Studio not detected; trying cl from PATH."
  cmd.exe /c "cl /nologo /EHsc /std:c++17 `"$src`" /Fe`"$exe`""
  exit $LASTEXITCODE
}
