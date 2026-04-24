# DeepMetria MFC 배포 가이드

## 목차
1. 사전 요구사항 ................................. L18
2. Release 빌드 절차 ............................. L54
3. 인스톨러 빌드 (Inno Setup / WiX) .............. L94
4. GitHub Actions CI/CD .......................... L164
5. 자동 업데이트 메커니즘 ........................ L219
6. 배포 후 검증 및 롤백 절차 ..................... L264





---

## 1. 사전 요구사항

### 빌드 머신 환경

| 항목 | 요구사항 |
|------|----------|
| OS | Windows 10/11 x64 |
| Visual Studio | 2022 (MFC 워크로드 포함) |
| MSBuild | VS2022와 함께 설치됨 |
| vcpkg | C:\vcpkg (의존 라이브러리 관리) |
| Inno Setup | 6.x (EXE 인스톨러) 또는 WiX 4.x (MSI) |
| Git | 2.x |
| GitHub Actions Runner | self-hosted 또는 windows-latest |

### 코드서명 인증서 (프로덕션)

Windows 배포 시 SmartScreen 경고 방지를 위해 코드서명 필수:
- **EV Code Signing Certificate** (DigiCert, Sectigo 등)
- `signtool.exe` (Windows SDK 포함)

```powershell
# 서명 예시
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 `
    /f "cert.pfx" /p "password" "DeepMetria_Setup.exe"
```

### 배포 채널

| 채널 | 방식 | 대상 |
|------|------|------|
| GitHub Releases | EXE/MSI 인스톨러 첨부 | 최종 사용자 |
| GitHub Actions | 자동 빌드 + 업로드 | CI 아티팩트 |
| 자동 업데이트 | GitHub Releases API 조회 | 설치된 앱 |

---

## 2. Release 빌드 절차

### 2.1 의존 라이브러리 설치 확인

```powershell
C:\vcpkg\vcpkg install sqlite3:x64-windows
C:\vcpkg\vcpkg install curl[openssl]:x64-windows
C:\vcpkg\vcpkg install nlohmann-json:x64-windows
C:\vcpkg\vcpkg install openssl:x64-windows
```

### 2.2 버전 번호 갱신

`src/common/Version.h`:
```cpp
#define DM_VERSION_MAJOR 1
#define DM_VERSION_MINOR 0
#define DM_VERSION_PATCH 0
#define DM_VERSION_STR   "1.0.0"
```

`DeepMetria_MFC.rc` 내 `FILEVERSION`, `PRODUCTVERSION`도 동일하게 갱신합니다.

### 2.3 MSBuild Release 빌드

```powershell
# VS2022 개발자 PowerShell에서 실행
msbuild DeepMetria_MFC.sln `
    /p:Configuration=Release `
    /p:Platform=x64 `
    /p:VcpkgEnabled=true `
    /m  # 병렬 빌드
```

### 2.4 출력 디렉터리 확인

```
x64\Release\
├── DeepMetria_MFC.exe          # 메인 실행 파일
├── libcurl.dll
├── libssl-3-x64.dll
├── libcrypto-3-x64.dll
└── (VC++ 런타임 DLL — 인스톨러에 포함)
```

### 2.5 빌드 검증

```powershell
# 의존성 확인
dumpbin /dependents x64\Release\DeepMetria_MFC.exe

# 간단한 실행 확인
.\x64\Release\DeepMetria_MFC.exe --version
```

---

## 3. 인스톨러 빌드 (Inno Setup / WiX)

### 3.1 Inno Setup으로 EXE 인스톨러 생성 (권장)

`installer\setup.iss`:
```ini
[Setup]
AppName=DeepMetria
AppVersion={#DM_VERSION}
AppPublisher=DeepMetria Team
DefaultDirName={autopf}\DeepMetria
DefaultGroupName=DeepMetria
OutputBaseFilename=DeepMetria_Setup_{#DM_VERSION}
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
MinVersion=10.0.19041  ; Windows 10 20H1

[Files]
Source: "..\x64\Release\DeepMetria_MFC.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\x64\Release\libcurl.dll";        DestDir: "{app}"; Flags: ignoreversion
Source: "..\x64\Release\libssl-3-x64.dll";  DestDir: "{app}"; Flags: ignoreversion
Source: "..\x64\Release\libcrypto-3-x64.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\third_party\vcredist\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\DeepMetria"; Filename: "{app}\DeepMetria_MFC.exe"
Name: "{commondesktop}\DeepMetria"; Filename: "{app}\DeepMetria_MFC.exe"

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/quiet /norestart"; \
    StatusMsg: "Visual C++ 런타임 설치 중..."; \
    Check: VCRedistNeedsInstall

[Code]
function VCRedistNeedsInstall: Boolean;
begin
  Result := not RegKeyExists(HKLM,
    'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64');
end;
```

**빌드:**
```powershell
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\setup.iss `
    /DDM_VERSION=1.0.0
```

출력: `installer\Output\DeepMetria_Setup_1.0.0.exe`

### 3.2 WiX 4로 MSI 인스톨러 생성 (엔터프라이즈 배포 시)

```powershell
# WiX 4 설치
dotnet tool install --global wix

# MSI 빌드
wix build installer\product.wxs -o installer\DeepMetria_1.0.0.msi
```

`installer\product.wxs` 핵심 구조:
```xml
<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs">
  <Package Name="DeepMetria" Version="1.0.0"
           Manufacturer="DeepMetria Team"
           UpgradeCode="PUT-GUID-HERE">
    <MajorUpgrade DowngradeErrorMessage="더 높은 버전이 이미 설치되어 있습니다." />
    <Feature Id="Main">
      <ComponentGroupRef Id="AppFiles" />
    </Feature>
  </Package>
</Wix>
```

### 3.3 코드서명 (프로덕션)

```powershell
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 `
    /f "$(CERT_PFX)" /p "$(CERT_PASSWORD)" `
    "installer\Output\DeepMetria_Setup_1.0.0.exe"
```

---

## 4. GitHub Actions CI/CD

### 4.1 워크플로 파일

`.github/workflows/release.yml`:
```yaml
name: Release Build

on:
  push:
    tags: ['v*.*.*']

jobs:
  build:
    runs-on: windows-latest

    steps:
      - uses: actions/checkout@v4

      - name: Setup vcpkg
        run: |
          git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
          C:\vcpkg\bootstrap-vcpkg.bat
          C:\vcpkg\vcpkg integrate install

      - name: Install dependencies
        run: |
          C:\vcpkg\vcpkg install sqlite3:x64-windows
          C:\vcpkg\vcpkg install "curl[openssl]:x64-windows"
          C:\vcpkg\vcpkg install nlohmann-json:x64-windows

      - name: Setup MSBuild
        uses: microsoft/setup-msbuild@v2

      - name: Extract version
        id: version
        run: echo "VERSION=${GITHUB_REF_NAME#v}" >> $GITHUB_OUTPUT
        shell: bash

      - name: Build Release
        run: |
          msbuild DeepMetria_MFC.sln `
            /p:Configuration=Release `
            /p:Platform=x64 `
            /p:VcpkgEnabled=true `
            /m

      - name: Build Installer
        run: |
          & "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\setup.iss `
            /DDM_VERSION=${{ steps.version.outputs.VERSION }}

      - name: Sign Installer
        if: secrets.CERT_PFX_BASE64 != ''
        run: |
          $cert = [Convert]::FromBase64String("${{ secrets.CERT_PFX_BASE64 }}")
          [IO.File]::WriteAllBytes("cert.pfx", $cert)
          signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 `
            /f cert.pfx /p "${{ secrets.CERT_PASSWORD }}" `
            "installer\Output\DeepMetria_Setup_${{ steps.version.outputs.VERSION }}.exe"
        shell: powershell

      - name: Create GitHub Release
        uses: softprops/action-gh-release@v2
        with:
          files: |
            installer/Output/DeepMetria_Setup_*.exe
          generate_release_notes: true
```

### 4.2 PR 빌드 체크 (main 브랜치 보호)

`.github/workflows/pr-check.yml`:
```yaml
name: PR Build Check

on:
  pull_request:
    branches: [main]

jobs:
  build:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      - uses: microsoft/setup-msbuild@v2
      - name: Build Debug
        run: msbuild DeepMetria_MFC.sln /p:Configuration=Debug /p:Platform=x64 /m
```

---

## 5. 자동 업데이트 메커니즘

### 5.1 업데이트 확인 흐름

앱 시작 시 백그라운드 스레드에서 GitHub Releases API를 조회합니다:

```
GitHub API: GET https://api.github.com/repos/your-org/DeepMetria_MFC/releases/latest
→ 응답의 tag_name 파싱 → 현재 버전과 비교 → 신버전이면 알림
```

### 5.2 AutoUpdater 클래스

```cpp
// src/updater/AutoUpdater.h
class AutoUpdater {
public:
    static AutoUpdater& GetInstance();

    // 백그라운드 업데이트 확인 (앱 시작 시 호출)
    void CheckForUpdates(HWND hNotifyWnd);

    // 업데이트 다운로드 및 실행
    BOOL DownloadAndInstall(const CString& downloadUrl,
                            const CString& version);

private:
    static UINT CheckThread(LPVOID pParam);
    CString FetchLatestVersion();
    BOOL    IsNewerVersion(const CString& remote, const CString& local);
};

// WM_DM_UPDATE_AVAILABLE: wParam=0, lParam=포인터(UpdateInfo*)
#define WM_DM_UPDATE_AVAILABLE (WM_APP + 200)

struct UpdateInfo {
    CString version;
    CString downloadUrl;
    CString releaseNotes;
};
```

### 5.3 업데이트 설치 흐름

1. 사용자에게 업데이트 알림 다이얼로그 표시
2. 사용자 확인 시 → 인스톨러 EXE 임시 폴더에 다운로드
3. `ShellExecute`로 인스톨러 실행 (UAC 프롬프트 자동)
4. 앱 자체 종료 (`PostQuitMessage(0)`)
5. 인스톨러가 이전 버전 자동 덮어쓰기 (Inno Setup MajorUpgrade)

---

## 6. 배포 후 검증 및 롤백 절차

### 6.1 배포 후 체크리스트

- [ ] 인스톨러 정상 실행 확인 (관리자 권한 없이 설치 가능 여부)
- [ ] 설치 경로 `%ProgramFiles%\DeepMetria\` 확인
- [ ] 최초 실행 시 DB 파일 자동 생성 확인
- [ ] API 키 입력 및 LLM 연결 테스트 통과
- [ ] CSV 파일 로드 → 분석 → 시각화 생성 E2E 동작 확인
- [ ] 코드서명 유효성 확인: `signtool verify /pa DeepMetria_MFC.exe`
- [ ] Windows 이벤트 로그 오류 없음 확인

### 6.2 롤백 절차

#### 이전 버전 인스톨러로 롤백

Inno Setup의 `MajorUpgrade` 설정 덕분에 이전 버전 인스톨러를 실행하면
현재 버전을 자동으로 제거하고 이전 버전을 설치합니다.

```powershell
# GitHub Releases에서 이전 버전 인스톨러 다운로드 후 실행
Start-Process "DeepMetria_Setup_0.9.0.exe" -Wait
```

#### GitHub Releases 롤백

```bash
# 잘못된 릴리즈 숨기기 (삭제 대신 draft로 전환)
gh release edit v1.0.1 --draft

# 이전 버전을 latest로 재지정
gh release edit v1.0.0 --latest
```

### 6.3 롤백 시 DB 마이그레이션 고려사항

스키마 변경이 없으면 기존 DB 파일을 그대로 사용합니다.
스키마 변경이 포함된 버전 롤백 시:

1. `%APPDATA%\DeepMetria\deepmetria.db` 백업 (`deepmetria.db.bak`)
2. 이전 버전 실행 → 자동으로 이전 스키마로 재초기화
3. 데이터 보존이 필요한 경우 수동 마이그레이션 스크립트 적용
