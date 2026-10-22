<#
.SYNOPSIS
    Build the LyraOS kernel and create a bootable ISO.

.DESCRIPTION
    1. Configures CMake with Ninja and Clang (cross-compile to x86_64-unknown-elf).
    2. Builds the kernel ELF.
    3. Verifies the output is ELF x86-64.
    4. Creates a bootable UEFI ISO using Limine and xorriso.

.PARAMETER Clean
    Remove the build directory before building.
#>
param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot

# --- Resolve tools -----------------------------------------------------------

function Find-Tool {
    param([string]$Name, [string[]]$ExtraPaths = @())
    foreach ($p in $ExtraPaths) {
        $full = Join-Path $p "$Name.exe"
        if (Test-Path $full) { return $full }
    }
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

# MSYS2 paths (common on Windows with MSYS2/UCRT64)
$Msys2Usr = $null
$Msys2Ucrt64 = $null
# Try to find msys2 via scoop or standard install
$scoopMsys2 = Join-Path $env:USERPROFILE "scoop\apps\msys2\current"
if (Test-Path $scoopMsys2) {
    $Msys2Usr = Join-Path $scoopMsys2 "usr\bin"
    $Msys2Ucrt64 = Join-Path $scoopMsys2 "ucrt64\bin"
}
$extraPaths = @()
if ($Msys2Usr)    { $extraPaths += $Msys2Usr }
if ($Msys2Ucrt64) { $extraPaths += $Msys2Ucrt64 }

$cmake  = Find-Tool "cmake"  $extraPaths
$ninja  = Find-Tool "ninja"  $extraPaths
$clang  = Find-Tool "clang"  $extraPaths
$xorriso = Find-Tool "xorriso" $extraPaths

if (-not $cmake)  { Write-Error "cmake not found. Install CMake and add to PATH." }
if (-not $ninja)  { Write-Error "ninja not found. Install Ninja and add to PATH." }
if (-not $clang)  { Write-Error "clang not found. Install Clang/LLVM and add to PATH." }
if (-not $xorriso) { Write-Error "xorriso not found. Install xorriso (MSYS2: pacman -S xorriso)." }

Write-Host "Tools:"
Write-Host "  cmake   = $cmake"
Write-Host "  ninja   = $ninja"
Write-Host "  clang   = $clang"
Write-Host "  xorriso = $xorriso"

# --- Paths -------------------------------------------------------------------

$BuildDir   = Join-Path $ProjectRoot "build"
$KernelElf  = Join-Path $BuildDir "LyraOS.elf"
$IsoDir     = Join-Path $BuildDir "iso_root"
$IsoFile    = Join-Path $BuildDir "LyraOS.iso"
$LimineBin  = Join-Path $ProjectRoot "boot\limine-bin"

# --- Clean -------------------------------------------------------------------

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "`nCleaning build directory..."
    Remove-Item -Recurse -Force $BuildDir
}

# --- CMake Configure ---------------------------------------------------------

Write-Host "`n--- CMake Configure ---"

$Toolchain = Join-Path $ProjectRoot `
    "cmake\toolchains\x86_64-elf-clang.cmake"

if (-not (Test-Path $Toolchain)) {
    Write-Error "Toolchain não encontrado: $Toolchain"
}

& $cmake -S $ProjectRoot -B $BuildDir `
    -G Ninja `
    "-DCMAKE_TOOLCHAIN_FILE=$Toolchain" `
    "-DCMAKE_C_COMPILER=$clang" `
    "-DCMAKE_CXX_COMPILER=$clang" `
    -DCMAKE_BUILD_TYPE=Debug

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed."
}
# --- Build -------------------------------------------------------------------

Write-Host "`n--- Ninja Build ---"
& $cmake --build $BuildDir
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed." }

# --- Verify ELF --------------------------------------------------------------

Write-Host "`n--- Verify kernel ELF ---"
$llvmReadobj = Find-Tool "llvm-readobj" $extraPaths
if (-not $llvmReadobj) {
    $llvmReadobj = Find-Tool "llvm-readelf" $extraPaths
}
if ($llvmReadobj) {
    & $llvmReadobj -h $KernelElf 2>&1 | Select-String -Pattern "Class|Machine|Type|Entry"
} else {
    Write-Host "(llvm-readobj not found; skipping ELF verification)"
    Write-Host "You can verify manually: llvm-readobj -h $KernelElf"
}

# --- Create ISO ---------------------------------------------------------------

Write-Host "`n--- Create bootable ISO ---"

# Check that Limine binary files exist
$requiredFiles = @("BOOTX64.EFI", "limine-uefi-cd.bin")
foreach ($f in $requiredFiles) {
    $fp = Join-Path $LimineBin $f
    if (-not (Test-Path $fp)) {
        Write-Error "Missing Limine file: $fp. Run: git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1 boot/limine-bin"
    }
}

# Create ISO directory structure
if (Test-Path $IsoDir) { Remove-Item -Recurse -Force $IsoDir }
New-Item -ItemType Directory -Path $IsoDir -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $IsoDir "boot") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $IsoDir "EFI\BOOT") -Force | Out-Null

# Copy kernel
Copy-Item $KernelElf (Join-Path $IsoDir "boot\LyraOS.elf")

# Copy Limine config
Copy-Item (Join-Path $ProjectRoot "boot\limine.conf") (Join-Path $IsoDir "boot\limine.conf")

# Copy Limine UEFI files
Copy-Item (Join-Path $LimineBin "BOOTX64.EFI") (Join-Path $IsoDir "EFI\BOOT\BOOTX64.EFI")
Copy-Item (Join-Path $LimineBin "limine-uefi-cd.bin") (Join-Path $IsoDir "boot\limine-uefi-cd.bin")

# Also copy BIOS files if available (for BIOS boot support)
$biosCd = Join-Path $LimineBin "limine-bios-cd.bin"
$biosSys = Join-Path $LimineBin "limine-bios.sys"
if ((Test-Path $biosCd) -and (Test-Path $biosSys)) {
    Copy-Item $biosCd (Join-Path $IsoDir "boot\limine-bios-cd.bin")
    Copy-Item $biosSys (Join-Path $IsoDir "boot\limine-bios.sys")
    $hasBios = $true
} else {
    $hasBios = $false
}

# Build ISO with xorriso

# Convert Windows paths to MSYS2/POSIX paths for xorriso.
$cygpath = Find-Tool "cygpath" $extraPaths

if (-not $cygpath) {
    Write-Error "cygpath not found. It should be in MSYS2 usr\bin."
}

$IsoFilePosix = (& $cygpath -u $IsoFile).Trim()
$IsoDirPosix  = (& $cygpath -u $IsoDir).Trim()

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to convert ISO paths."
}

Write-Host "Running xorriso..."

$xorrisoArgs = @(
    "-as", "mkisofs"
    "-R"                            # Rock Ridge extensions
    "-no-emul-boot"
    "-boot-load-size", "4"
    "-boot-info-table"
)

if ($hasBios) {
    $xorrisoArgs += @(
        "-b", "boot/limine-bios-cd.bin"   # BIOS El Torito boot image
    )
} else {
    # UEFI-only: use the UEFI CD image
    $xorrisoArgs += @(
        "-b", "boot/limine-uefi-cd.bin"
    )
}

$xorrisoArgs += @(
    "--efi-boot", "boot/limine-uefi-cd.bin"
    "-efi-boot-part"
    "--efi-boot-image"
    "--protective-msdos-label"
    "-o", $IsoFilePosix
    $IsoDirPosix
)

& $xorriso @xorrisoArgs
if ($LASTEXITCODE -ne 0) { Write-Error "xorriso ISO creation failed." }

# Install Limine for BIOS boot (if the limine CLI is available)
$limineCli = Join-Path $LimineBin "limine.exe"
if ((Test-Path $limineCli) -and $hasBios) {
    Write-Host "Installing Limine BIOS stages..."
    & $limineCli bios-install $IsoFile
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Warning: limine bios-install failed (UEFI boot still works)"
    }
}

# --- Done --------------------------------------------------------------------

Write-Host "`n=== Build complete ==="
Write-Host "Kernel : $KernelElf"
Write-Host "ISO    : $IsoFile"
Write-Host ""
Write-Host "Run with: .\scripts\run-qemu.ps1"
