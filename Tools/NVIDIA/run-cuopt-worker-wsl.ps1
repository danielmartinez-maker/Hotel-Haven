param(
    [Parameter(Mandatory = $true)][string]$InputJson,
    [Parameter(Mandatory = $true)][string]$OutputJson,
    [string]$Distro = ""
)

$ErrorActionPreference = "Stop"
$workerWindows = (Resolve-Path (Join-Path $PSScriptRoot "..\CuOptWorker\hotel_haven_cuopt_worker.py")).Path
$inputWindows = (Resolve-Path $InputJson).Path
$outputWindows = [System.IO.Path]::GetFullPath($OutputJson)

function Invoke-WslCommand([string[]]$CommandArgs) {
    $wslArgs = @()
    if ($Distro) {
        $wslArgs += @("-d", $Distro)
    }
    $wslArgs += "--"
    $wslArgs += $CommandArgs
    & wsl.exe @wslArgs
    if ($LASTEXITCODE -ne 0) {
        throw "WSL command failed with exit code $LASTEXITCODE"
    }
}

function Get-WslPath([string]$WindowsPath) {
    $wslArgs = @()
    if ($Distro) {
        $wslArgs += @("-d", $Distro)
    }
    $wslArgs += @("--", "wslpath", "-a", $WindowsPath)
    $value = & wsl.exe @wslArgs
    if ($LASTEXITCODE -ne 0) {
        throw "wslpath failed with exit code $LASTEXITCODE"
    }
    return ($value | Out-String).Trim()
}

$workerLinux = Get-WslPath $workerWindows
$inputLinux = Get-WslPath $inputWindows
$outputLinux = Get-WslPath $outputWindows
Invoke-WslCommand @("python3", $workerLinux, "--input", $inputLinux, "--output", $outputLinux)
Write-Host "cuOpt plan written to $outputWindows"
