# scripts/setup-secrets.ps1 - secrets.json 의 키들을 DPAPI 로 암호화해
# HKCU\Software\DeepMetria\Settings 에 저장한 뒤 secrets.json 을 안전하게 지운다.
[CmdletBinding()]
param(
    [string] $SecretsPath = (Join-Path $PSScriptRoot '..\secrets.json'),
    [string] $RegKey      = 'HKCU:\Software\DeepMetria\Settings'
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $SecretsPath)) {
    Write-Error "secrets.json 이 없습니다: $SecretsPath"
    exit 1
}

$json = Get-Content -LiteralPath $SecretsPath -Raw | ConvertFrom-Json

if (-not (Test-Path -LiteralPath $RegKey)) {
    New-Item -Path $RegKey -Force | Out-Null
}

# provider / model 평문 저장
if ($json.provider) { Set-ItemProperty -LiteralPath $RegKey -Name 'Provider' -Value $json.provider }
if ($json.model)    { Set-ItemProperty -LiteralPath $RegKey -Name 'Model'    -Value $json.model }

Add-Type -AssemblyName System.Security

function Protect-Utf8([string] $plain) {
    if ([string]::IsNullOrEmpty($plain)) { return $null }
    $bytes  = [System.Text.Encoding]::UTF8.GetBytes($plain)
    $cipher = [System.Security.Cryptography.ProtectedData]::Protect(
        $bytes, $null,
        [System.Security.Cryptography.DataProtectionScope]::CurrentUser)
    return ,$cipher
}

foreach ($prop in 'Claude','OpenAI','Gemini') {
    $val = $json.keys.$prop
    if ([string]::IsNullOrWhiteSpace($val)) { continue }
    $cipher = Protect-Utf8 $val
    Set-ItemProperty -LiteralPath $RegKey -Name "ApiKey.$prop" -Value $cipher -Type Binary
    Write-Host "[ok] ApiKey.$prop  ($($cipher.Length) bytes)"
}

# secrets.json 을 0으로 덮어쓴 뒤 삭제 — 평문 잔존 방지
$size = (Get-Item -LiteralPath $SecretsPath).Length
[System.IO.File]::WriteAllBytes($SecretsPath, [byte[]]::new($size))
Remove-Item -LiteralPath $SecretsPath -Force
Write-Host "[ok] secrets.json removed."
exit 0
