# scripts/test-llm.ps1
# 레지스트리에 저장된 DPAPI 키로 현재 설정된 LLM provider 에 직접 POST 해
# 응답 본문/상태 코드/파싱 결과를 콘솔과 파일에 dump.
#
# 사용:
#   powershell -ExecutionPolicy Bypass -File scripts\test-llm.ps1
#   powershell -ExecutionPolicy Bypass -File scripts\test-llm.ps1 -Question "월별 매출 추이 보여줘"

[CmdletBinding()]
param(
    [string] $RegKey   = 'HKCU:\Software\DeepMetria\Settings',
    [string] $Question = '데이터 요약 알려줘',
    [string] $OutDir   = "$env:TEMP\deepmetria_llm_test",
    # -Model 로 레지스트리 설정 무시하고 강제 모델 지정 가능
    [string] $Model    = ''
)

if (-not (Test-Path -LiteralPath $RegKey)) {
    Write-Host "[FATAL] 레지스트리 키 없음: $RegKey"
    Write-Host "        먼저 setup-secrets.bat 로 키 등록 필요."
    exit 1
}

if (-not (Test-Path -LiteralPath $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
}

$item     = Get-ItemProperty -LiteralPath $RegKey
$provider = "$($item.Provider)".ToLower()
$model    = if ($Model) { $Model } else { "$($item.Model)" }

Write-Host "================================================================"
Write-Host " DeepMetria LLM 직접 호출 테스트"
Write-Host "================================================================"
Write-Host " Provider : $provider"
Write-Host " Model    : $model"
Write-Host " Question : $Question"
Write-Host " OutDir   : $OutDir"
Write-Host ""

Add-Type -AssemblyName System.Security

$secretName = $null
if ($provider -eq 'claude') { $secretName = 'ApiKey.Claude' }
if ($provider -eq 'openai') { $secretName = 'ApiKey.OpenAI' }
if ($provider -eq 'gemini') { $secretName = 'ApiKey.Gemini' }

if (-not $secretName) {
    Write-Host "[FATAL] 알 수 없는 provider: $provider"
    exit 1
}

$cipher = (Get-ItemProperty -LiteralPath $RegKey -Name $secretName -ErrorAction SilentlyContinue).$secretName
if (-not $cipher) {
    Write-Host "[FATAL] $secretName 가 레지스트리에 없음."
    exit 1
}

try {
    $plain  = [System.Security.Cryptography.ProtectedData]::Unprotect(
        [byte[]]$cipher, $null,
        [System.Security.Cryptography.DataProtectionScope]::CurrentUser)
    $apiKey = [System.Text.Encoding]::UTF8.GetString($plain)
} catch {
    Write-Host "[FATAL] DPAPI 복호화 실패: $_"
    exit 1
}

Write-Host "[ok] API 키 복호화 성공 (length=$($apiKey.Length))"
Write-Host ""

# 간단 시스템 프롬프트 — 메인 앱 Provider 와 동일 패턴 (params 스키마 포함)
$sys = @"
당신은 한국어 데이터 분석 어시스턴트입니다.
[데이터] 행 100, 컬럼 3 (수치 2, 날짜 1)
[컬럼]
- 월 (날짜), 예: "2024-01"
- 매출 (수치), 평균 1234.56
- 카테고리 (텍스트), 5개 고유값
[도구] (모두 시각화 생성형 — 호출 시 viz 카드가 만들어짐)
각 도구의 params 키 이름은 스키마와 정확히 일치해야 함. 임의로 추측 금지.
- group_by_sum: 그룹별 합계 막대 차트
    params 스키마: {"group":"string","value":"string"}
- trend_over_time: 날짜 기준 라인 추이
    params 스키마: {"date":"string","value":"string"}
- table_sample: 표 미리보기
    params 스키마: {"rows":"integer"}

필드 규칙:
- title: 8자 이내 짧은 라벨
- description: ★★ 반드시 포함 ★★. title과 절대 같으면 안 됨. 실제 데이터 인사이트 1-3문장.

응답 형식 (반드시 JSON):
시각화 필요: {"toolName":"...","params":{...},"title":"짧은 제목","description":"결과 설명","reasoning":"사고"}
단순 대화: {"toolName":"","description":"답변","reasoning":"사고"}
"@

$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'

function Save-File($name, $content) {
    $path = Join-Path $OutDir ($stamp + '_' + $name)
    Set-Content -LiteralPath $path -Value $content -Encoding utf8
    Write-Host "[ok] 저장됨: $path"
}

$uri     = $null
$bodyObj = $null
$headers = $null

if ($provider -eq 'claude') {
    if (-not $model) { $model = 'claude-opus-4-7' }
    $bodyObj = @{
        model      = $model
        max_tokens = 1024
        system     = $sys
        messages   = @(@{ role = 'user'; content = $Question })
    }
    $headers = @{
        'x-api-key'         = $apiKey
        'anthropic-version' = '2023-06-01'
    }
    $uri = 'https://api.anthropic.com/v1/messages'
}
elseif ($provider -eq 'openai') {
    if (-not $model) { $model = 'gpt-4o-mini' }
    $bodyObj = @{
        model    = $model
        messages = @(
            @{ role = 'system'; content = $sys },
            @{ role = 'user';   content = $Question }
        )
    }
    $headers = @{ 'Authorization' = "Bearer $apiKey" }
    $uri = 'https://api.openai.com/v1/chat/completions'
}
elseif ($provider -eq 'gemini') {
    if (-not $model) { $model = 'gemini-2.5-flash' }
    $bodyObj = @{
        systemInstruction = @{ parts = @(@{ text = $sys }) }
        contents = @(@{
            role  = 'user'
            parts = @(@{ text = $Question })
        })
    }
    $headers = @{}
    $uri = 'https://generativelanguage.googleapis.com/v1beta/models/' + $model + ':generateContent?key=' + $apiKey
}

$body = $bodyObj | ConvertTo-Json -Depth 8 -Compress
Save-File 'request_body.json' $body

Write-Host ""
Write-Host "==== HTTP POST ===="
$uriShown = $uri
if ($uriShown.Contains('key=')) {
    $uriShown = $uriShown.Substring(0, $uriShown.IndexOf('key=') + 4) + '***'
}
Write-Host " URI : $uriShown"
Write-Host " Body length: $($body.Length)"
Write-Host ""

try {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $resp = Invoke-WebRequest -Method POST -Uri $uri `
        -Headers $headers -Body $body `
        -ContentType 'application/json; charset=utf-8' `
        -UseBasicParsing -ErrorAction Stop
    $sw.Stop()

    Write-Host "[ok] HTTP $($resp.StatusCode) — $($sw.ElapsedMilliseconds) ms — $($resp.Content.Length) bytes"
    Save-File 'response_body.json' $resp.Content

    # 응답에서 LLM 텍스트 추출
    try {
        $obj = $resp.Content | ConvertFrom-Json
        $text = $null
        if ($provider -eq 'claude') { $text = $obj.content[0].text }
        if ($provider -eq 'openai') { $text = $obj.choices[0].message.content }
        if ($provider -eq 'gemini') { $text = $obj.candidates[0].content.parts[0].text }
        if ($text) {
            Save-File 'llm_text.txt' $text
            Write-Host ""
            Write-Host "==== LLM 텍스트 (앞 500자) ===="
            Write-Host ($text.Substring(0, [Math]::Min(500, $text.Length)))
            Write-Host ""

            # JSON 파싱
            $stripped = $text
            if ($stripped.StartsWith('```json')) { $stripped = $stripped.Substring(7) }
            if ($stripped.StartsWith('```'))     { $stripped = $stripped.Substring(3) }
            $stripped = $stripped.Trim()
            if ($stripped.EndsWith('```')) { $stripped = $stripped.Substring(0, $stripped.Length - 3) }
            $stripped = $stripped.Trim()

            try {
                $parsed = $stripped | ConvertFrom-Json -ErrorAction Stop
                Write-Host "==== 파싱된 필드 ===="
                Write-Host (" toolName    = " + $parsed.toolName)
                Write-Host (" title       = " + $parsed.title)
                Write-Host (" description = " + $parsed.description)
                Write-Host (" reasoning   = " + $parsed.reasoning)
                if ($parsed.params) {
                    Write-Host " params:"
                    $parsed.params.psobject.Properties | ForEach-Object {
                        Write-Host ("   " + $_.Name + " = " + $_.Value)
                    }
                }
            } catch {
                Write-Host "[WARN] LLM 응답 JSON 파싱 실패: $_"
            }
        } else {
            Write-Host "[WARN] LLM 응답에서 텍스트 추출 못함"
        }
    } catch {
        Write-Host "[WARN] 응답 JSON 파싱 실패: $_"
    }
} catch {
    $err = $_.Exception
    Write-Host "[FAIL] HTTP 요청 실패: $($err.Message)"
    if ($err.Response) {
        try {
            $sr = New-Object System.IO.StreamReader($err.Response.GetResponseStream())
            $errBody = $sr.ReadToEnd()
            Save-File 'response_error.json' $errBody
            Write-Host "==== 에러 응답 본문 ===="
            Write-Host $errBody
        } catch {
            Write-Host "[WARN] 에러 본문 읽기 실패: $_"
        }
    }
    exit 1
}

Write-Host ""
Write-Host "================================================================"
Write-Host " 완료. 자세한 본문은 $OutDir 폴더 확인."
Write-Host "================================================================"
exit 0
