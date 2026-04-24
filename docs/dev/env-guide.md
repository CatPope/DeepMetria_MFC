# 환경 설정 가이드

**DeepMetria MFC** C++ 데스크톱 앱의 개발 환경 구축 및 의존 라이브러리 설정 가이드.

## 목차
1. 개발 환경 요구사항 .......................... L18
2. Visual Studio 2022 프로젝트 설정 ........... L48
3. 의존 라이브러리 설치 ....................... L93
4. 앱 런타임 설정 (API 키 등) ................. L158
5. 빌드 및 실행 확인 .......................... L188




---

## 1. 개발 환경 요구사항

### 필수 소프트웨어

| 항목 | 버전 | 비고 |
|------|------|------|
| Windows | 10 / 11 (x64) | 배포 대상 OS |
| Visual Studio 2022 | 17.x | Community 이상 |
| MFC 워크로드 | VS2022 포함 | "C++를 사용한 데스크톱 개발" 선택 |
| Windows SDK | 10.0.22621 이상 | VS2022 설치 시 자동 포함 |
| Git | 2.x | 소스 버전 관리 |
| CMake | 3.28+ | vcpkg 의존성 관리에 사용 (선택) |

### 권장 VS2022 워크로드

VS2022 설치 관리자에서 다음 항목 선택:
- **C++를 사용한 데스크톱 개발** (MFC, ATL 포함)
- **C++ CMake 도구** (선택)
- **Windows 10/11 SDK**

### 선택 도구

| 도구 | 용도 |
|------|------|
| vcpkg | C++ 패키지 관리자 (libcurl, nlohmann/json 등) |
| DB Browser for SQLite | SQLite DB 직접 확인 |
| Inno Setup 6 또는 WiX 4 | MSI/EXE 인스톨러 빌드 |
| GitHub Actions | CI/CD 자동 빌드 |

---

## 2. Visual Studio 2022 프로젝트 설정

### 2.1 프로젝트 속성 (모든 구성 공통)

**구성 형식:** 응용 프로그램 (.exe)
**문자 집합:** 유니코드 문자 집합 사용

**C/C++ → 일반 → 추가 포함 디렉터리:**
```
$(SolutionDir)third_party\sqlite\include
$(SolutionDir)third_party\curl\include
$(SolutionDir)third_party\nlohmann\include
$(SolutionDir)third_party\openssl\include
```

**링커 → 일반 → 추가 라이브러리 디렉터리:**
```
$(SolutionDir)third_party\sqlite\lib\$(Platform)
$(SolutionDir)third_party\curl\lib\$(Platform)
$(SolutionDir)third_party\openssl\lib\$(Platform)
```

**링커 → 입력 → 추가 종속성:**
```
sqlite3.lib
libcurl.lib
libssl.lib
libcrypto.lib
Ws2_32.lib
Crypt32.lib
```

### 2.2 구성별 설정

| 항목 | Debug | Release |
|------|-------|---------|
| 최적화 | 사용 안 함 (/Od) | 속도 최대화 (/O2) |
| 런타임 라이브러리 | 다중 스레드 디버그 DLL (/MDd) | 다중 스레드 DLL (/MD) |
| 디버그 정보 | 전체 (/Zi) | 없음 |
| 전처리기 정의 | `_DEBUG;DM_DEBUG_BUILD` | `NDEBUG` |

### 2.3 전처리기 정의 (공통)

```
WIN32
_WINDOWS
_AFXDLL
UNICODE
_UNICODE
CURL_STATICLIB
```

### 2.4 솔루션 구조

```
DeepMetria_MFC.sln
├── DeepMetria_MFC/          # 메인 MFC 앱 프로젝트
│   ├── src/
│   │   ├── data/            # DataSourceManager
│   │   ├── analysis/        # AnalysisEngine
│   │   ├── dashboard/       # DashboardManager
│   │   ├── llm/             # LLMClient (libcurl)
│   │   ├── db/              # SQLiteWrapper
│   │   ├── ui/              # MFC 다이얼로그/뷰
│   │   └── common/          # 공통 타입, 메시지 상수
│   └── res/                 # 리소스 파일
├── third_party/             # 의존 라이브러리 바이너리/헤더
└── installer/               # Inno Setup / WiX 스크립트
```

---

## 3. 의존 라이브러리 설치

### 3.1 vcpkg를 이용한 자동 설치 (권장)

```powershell
# vcpkg 클론 및 부트스트랩 (최초 1회)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

# VS2022 통합 (최초 1회, 관리자 권한)
C:\vcpkg\vcpkg integrate install

# 의존 라이브러리 설치 (x64-windows)
C:\vcpkg\vcpkg install sqlite3:x64-windows
C:\vcpkg\vcpkg install curl[openssl]:x64-windows
C:\vcpkg\vcpkg install nlohmann-json:x64-windows
C:\vcpkg\vcpkg install openssl:x64-windows
```

**vcpkg.json (매니페스트 모드 사용 시 프로젝트 루트에 배치):**
```json
{
  "name": "deepmetria-mfc",
  "version": "1.0.0",
  "dependencies": [
    "sqlite3",
    { "name": "curl", "features": ["openssl"] },
    "nlohmann-json",
    "openssl"
  ]
}
```

### 3.2 수동 설치 (vcpkg 미사용 시)

각 라이브러리를 `third_party/` 아래 다음 구조로 배치합니다:

```
third_party/
├── sqlite/
│   ├── include/sqlite3.h
│   └── lib/x64/sqlite3.lib
├── curl/
│   ├── include/curl/curl.h
│   └── lib/x64/libcurl.lib
├── nlohmann/
│   └── include/nlohmann/json.hpp
└── openssl/
    ├── include/openssl/
    └── lib/x64/{libssl.lib, libcrypto.lib}
```

**다운로드 위치:**
- SQLite: https://www.sqlite.org/download.html (amalgamation)
- libcurl: https://curl.se/windows/
- nlohmann/json: https://github.com/nlohmann/json/releases
- OpenSSL: https://slproweb.com/products/Win32OpenSSL.html

### 3.3 런타임 DLL 배포

Release 빌드 시 다음 DLL을 exe와 같은 디렉터리에 포함:

| DLL | 출처 |
|-----|------|
| `libcurl.dll` | curl/bin/ |
| `libssl-3-x64.dll` | OpenSSL/bin/ |
| `libcrypto-3-x64.dll` | OpenSSL/bin/ |
| `vcruntime140.dll` | VC++ 재배포 가능 패키지 |
| `mfc140u.dll` | VC++ 재배포 가능 패키지 |

SQLite는 정적 링킹(`sqlite3.c` amalgamation)으로 DLL 없이 포함 가능합니다.

---

## 4. 앱 런타임 설정 (API 키 등)

### 4.1 설정 저장 위치

모든 런타임 설정은 SQLite `app_settings` 테이블에 저장됩니다.
DB 파일 위치: `%APPDATA%\DeepMetria\deepmetria.db`

API 키는 **Windows DPAPI** (`CryptProtectData`)로 암호화하여 저장합니다.

### 4.2 주요 설정 항목

| 키 | 설명 | 기본값 |
|----|------|--------|
| `llm_provider` | 기본 LLM 제공자 | `anthropic` |
| `llm_model` | 기본 모델 | `claude-3-5-sonnet-20241022` |
| `anthropic_api_key` | Anthropic API 키 (암호화) | (없음) |
| `openai_api_key` | OpenAI API 키 (암호화) | (없음) |
| `gemini_api_key` | Google Gemini API 키 (암호화) | (없음) |
| `llm_timeout_sec` | LLM 호출 타임아웃 (초) | `60` |
| `theme` | UI 테마 | `light` |
| `max_file_size_mb` | 최대 파일 크기 (MB) | `200` |

### 4.3 API 키 입력 방법

앱 메뉴 → **설정(Settings)** → **LLM 설정** 탭에서 입력합니다.
입력된 키는 즉시 DPAPI 암호화 후 `app_settings`에 저장됩니다.

### 4.4 개발 환경 디버그 설정

```cpp
// src/common/DebugConfig.h (Debug 빌드 전용)
#ifdef DM_DEBUG_BUILD
    // 개발 중 API 키를 환경 변수에서 읽기 (선택)
    // 주의: Release 빌드에서는 반드시 앱 설정 UI 사용
    #define DM_DEBUG_READ_APIKEY_FROM_ENV 1
#endif
```

---

## 5. 빌드 및 실행 확인

### 5.1 Debug 빌드

```
VS2022 → 솔루션 플랫폼: x64 → 구성: Debug → 빌드(F7)
```

출력: `x64\Debug\DeepMetria_MFC.exe`

### 5.2 Release 빌드

```
VS2022 → 솔루션 플랫폼: x64 → 구성: Release → 빌드(F7)
```

출력: `x64\Release\DeepMetria_MFC.exe`

### 5.3 빌드 후 체크리스트

- [ ] `DeepMetria_MFC.exe` 생성 확인
- [ ] 필수 DLL 출력 디렉터리에 복사 확인
- [ ] 앱 최초 실행 시 `%APPDATA%\DeepMetria\` 폴더 자동 생성 확인
- [ ] SQLite DB 파일 자동 초기화 확인
- [ ] 설정 UI에서 API 키 입력 후 LLM 연결 테스트 통과 확인

### 5.4 GitHub Actions CI 빌드

`.github/workflows/build.yml`:
```yaml
- uses: microsoft/setup-msbuild@v2
- name: vcpkg install
  run: C:\vcpkg\vcpkg install --triplet x64-windows
- name: Build Release
  run: msbuild DeepMetria_MFC.sln /p:Configuration=Release /p:Platform=x64
```
