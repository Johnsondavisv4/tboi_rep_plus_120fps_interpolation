[CmdletBinding()]
param(
    [string]$Executable,
    [switch]$VerifyOnly
)

$ErrorActionPreference = 'Stop'
$arguments = @('-B', (Join-Path $PSScriptRoot 'tools\update_interpolator.py'))
if ($Executable) {
    $arguments += @('--exe', (Resolve-Path -LiteralPath $Executable).Path)
}
if ($VerifyOnly) {
    $arguments += '--verify-only'
}

$installManagers = @(
    (Join-Path $env:LOCALAPPDATA 'Python\bin\python.exe')
    (Join-Path ([Environment]::GetFolderPath('UserProfile')) 'AppData\Local\Python\bin\python.exe')
)
foreach ($installManager in $installManagers) {
    if (Test-Path -LiteralPath $installManager -PathType Leaf) {
        & $installManager @arguments
        exit $LASTEXITCODE
    }
}

$python = Get-Command py.exe -CommandType Application -ErrorAction SilentlyContinue
if ($python) {
    & $python.Source -3 @arguments
    exit $LASTEXITCODE
}

$python = Get-Command python.exe -CommandType Application -ErrorAction SilentlyContinue
if ($python) {
    & $python.Source @arguments
    exit $LASTEXITCODE
}

throw 'Se necesita Python 3.9 o posterior para ejecutar el buscador.'
