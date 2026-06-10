# scripts/list-gemini-models.ps1
# 레지스트리에 저장된 DPAPI Gemini 키로 사용 가능한 모델 목록 조회.
# 키는 절대 stdout/명령행에 노출하지 않음 — Header(X-goog-api-key) 로 전송.

[CmdletBinding()]
param(
    [string] $RegKey = 'HKCU:\Software\DeepMetria\Settings'
)

Add-Type -AssemblyName System.Security

$reg    = Get-ItemProperty -LiteralPath $RegKey
$cipher = $reg.'ApiKey.Gemini'
if (-not $cipher) {
    Write-Host '[FATAL] ApiKey.Gemini 레지스트리에 없음.'
    exit 1
}
$plain = [System.Security.Cryptography.ProtectedData]::Unprotect(
    [byte[]]$cipher, $null,
    [System.Security.Cryptography.DataProtectionScope]::CurrentUser)
$key   = [System.Text.Encoding]::UTF8.GetString($plain)

try {
    $resp = Invoke-WebRequest `
        -Uri 'https://generativelanguage.googleapis.com/v1beta/models' `
        -Headers @{ 'X-goog-api-key' = $key } `
        -UseBasicParsing -ErrorAction Stop
} catch {
    Write-Host "[FAIL] $($_.Exception.Message)"
    exit 1
}

$obj = $resp.Content | ConvertFrom-Json
Write-Host '==== generateContent 지원 모델 ===='
$obj.models |
    Where-Object { $_.supportedGenerationMethods -contains 'generateContent' } |
    ForEach-Object { Write-Host (' ' + $_.name) }
exit 0
