# scripts/verify-key.ps1 - 레지스트리에 저장된 DPAPI 키가 정상적으로 복호화되는지 점검.
[CmdletBinding()]
param(
    [string] $RegKey = 'HKCU:\Software\DeepMetria\Settings'
)

if (-not (Test-Path -LiteralPath $RegKey)) {
    Write-Host "[--] 레지스트리 키 없음: $RegKey"
    exit 1
}

$item = Get-ItemProperty -LiteralPath $RegKey
Write-Host ('Provider = ' + $item.Provider)
Write-Host ('Model    = ' + $item.Model)

Add-Type -AssemblyName System.Security

foreach ($prop in 'Claude','OpenAI','Gemini') {
    $name = "ApiKey.$prop"
    $val  = Get-ItemProperty -LiteralPath $RegKey -Name $name -ErrorAction SilentlyContinue
    if (-not $val) { Write-Host "[--] $name  (없음)"; continue }
    try {
        $cipher = $val.$name
        $plain  = [System.Security.Cryptography.ProtectedData]::Unprotect(
            [byte[]]$cipher, $null,
            [System.Security.Cryptography.DataProtectionScope]::CurrentUser)
        $len = $plain.Length
        Write-Host ("[ok] $name  복호화 OK (length=$len)")
    } catch {
        Write-Host ("[!!] $name  복호화 실패: $_")
    }
}
exit 0
