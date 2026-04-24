# DeepMetria 보안 점검 보고서
점검일: 2026-04-21 | 점검 범위: MFC C++ 데스크톱 앱 초기 보안 검토

## 목차
1. 점검 환경 및 범위 ........................ L13
2. 데스크톱 앱 보안 점검 결과 .............. L30
3. 추가 보안 점검 항목 ................... L194
4. 발견된 취약점 및 문제 사항 .......... L281
5. 권장 조치 사항 .......................... L321

---

## 1. 점검 환경 및 범위

**점검 대상**: DeepMetria MFC C++ 데스크톱 애플리케이션 (Windows 10/11)
**점검 방식**: 정적 코드 분석 (PVS-Studio), 설정 검토, 수동 점검
**검토 파일**:
- `src/core/AuthManager.cpp` — 자격증명 저장/로드, API 키 관리
- `src/core/FileParser.cpp` — 파일 로드 및 검증
- `src/core/LLMRouter.cpp` — LLM API 통신
- `src/core/SettingsManager.cpp` — 레지스트리/INI 설정 관리
- `src/ui/MainFrame.cpp` — 메인 윈도우, 메뉴 처리
- `src/infra/HttpClient.cpp` — WinHTTP 기반 HTTP 클라이언트
- `src/domain/DatasourceService.cpp` — 파일 검증 및 처리

**거버넌스 단계**: 표준 (Standard)

---

## 2. 데스크톱 앱 보안 점검 결과

### D01: 로컬 자격증명 및 API 키 보안

**상태**: PARTIAL

**검증 내용**:
- API 키 저장: Windows DPAPI(Data Protection API) 사용 여부 확인
  - `CryptProtectData()` / `CryptUnprotectData()` 호출 확인
- 레지스트리 저장 경로: `HKCU\Software\DeepMetria\Settings`
- 메모리 내 키 관리: 사용 후 `SecureZeroMemory()` 호출 여부

**발견된 문제**:
- API 키 일부가 `CString` 형태로 메모리에 장기 보유됨 (즉시 소거 미흡)
- 레지스트리 값이 DPAPI 없이 평문으로 저장되는 경로 존재

**위험도**: 높음 (API 키 탈취 가능성)

---

### D02: 메모리 안전성 (Buffer Overflow / Use-After-Free)

**상태**: PARTIAL

**검증 내용**:
- 문자열 처리: `strcpy` / `sprintf` 사용 여부 → `strcpy_s` / `sprintf_s` 대체 확인
- 버퍼 오버플로: PVS-Studio 정적 분석 결과
- 스마트 포인터: `std::unique_ptr` / `std::shared_ptr` 사용 패턴
- 배열 경계 검사: STL 컨테이너 vs 원시 배열 혼용 여부

**발견된 문제**:
- `FileParser.cpp` 내 `sprintf` 비보안 함수 2건 (PVS-Studio V576)
- 일부 MFC 콜백에서 원시 포인터 소유권 불명확

**위험도**: 중간

---

### D03: DLL 사이드로딩 / DLL 인젝션 방지

**상태**: PARTIAL

**검증 내용**:
- DLL 검색 순서: `SetDllDirectory("")` 호출로 CWD 제거 여부
- 서명 검증: 로드하는 DLL의 디지털 서명 확인 여부
- 절대 경로 로드: `LoadLibraryEx` + `LOAD_LIBRARY_SEARCH_SYSTEM32` 플래그 사용
- 매니페스트: `<trustInfo>`, `<requestedExecutionLevel>` 설정 확인

**발견된 문제**:
- `LoadLibrary` 호출 시 절대 경로 미사용 구간 존재 (상대 경로 2건)
- 서드파티 DLL 서명 검증 없음

**위험도**: 중간

---

### D04: 파일 시스템 보안 (경로 탈출 방지)

**상태**: PASS

**검증 내용**:
- 사용자 지정 파일 경로 정규화: `PathCanonicalize()` 사용
- 허용 디렉터리 외 접근 차단: 로드 경로를 `%USERPROFILE%\Documents\DeepMetria\` 내로 제한
- 파일명 검증: 특수문자 및 `..` 경로 포함 여부 필터링
- 임시 파일 정리: 테스트/분석 후 `%TEMP%\deepmetria_*` 자동 삭제

**권장 사항**: `PathIsRelative()` 추가 검증으로 방어 심화

---

### D05: 네트워크 통신 보안 (WinHTTP)

**상태**: PASS

**검증 내용**:
- HTTPS 전용: `WINHTTP_FLAG_SECURE` 플래그 강제
- 인증서 검증: 기본 WinHTTP 인증서 체인 검증 활성화 (비활성화 코드 없음)
- TLS 버전: `WINHTTP_OPTION_SECURE_PROTOCOLS` — TLS 1.2 이상 강제
- 응답 크기 제한: 수신 버퍼 상한 설정 (DoS 방어)

**권장 사항**: 인증서 핀닝(Certificate Pinning) 고려 (LLM API 엔드포인트)

---

### D06: 프로세스 권한 최소화

**상태**: PASS

**검증 내용**:
- UAC 수준: 매니페스트에 `asInvoker` (관리자 권한 불필요)
- 레지스트리 접근: `HKCU` 전용 (HKLM 쓰기 없음)
- 파일 시스템: 사용자 디렉터리만 접근 (`%USERPROFILE%`, `%APPDATA%`, `%TEMP%`)

**상태**: 최소 권한 원칙 준수

---

### D07: 실행 파일 보안 (컴파일러 보호)

**상태**: PARTIAL

**검증 내용**:
- /GS (Stack Buffer Security Check): 활성화 확인
- /DYNAMICBASE (ASLR): 활성화 확인
- /NXCOMPAT (DEP): 활성화 확인
- SafeSEH: x64에서는 기본 적용
- Code Signing: 배포 파일 서명 여부

**발견된 문제**:
- Release 빌드에서 `/LTCG` (Link-Time Code Generation) 미적용 (최적화 및 보안 분석 누락)
- 배포 실행 파일 코드 서명 미완료

**위험도**: 중간 (배포 시)

---

### D08: 입력 검증 (파일 및 사용자 입력)

**상태**: PARTIAL

**검증 내용**:
- 파일 크기 제한: 50MB 상한 적용
- 파일 확장자 검증: `.csv`, `.xlsx`, `.xls`, `.json`만 허용
- MIME 타입(매직 넘버) 검증: 파일 헤더 바이트 확인
- 사용자 텍스트 입력: SQL/명령어 인젝션 경로 없음 (로컬 앱)

**발견된 문제**:
- MIME 매직 넘버 검증이 확장자 기반 검증의 보조 수준에 그침 (완전 독립 검증 필요)
- Excel 파일 내 매크로(VBA) 포함 여부 검사 없음

**위험도**: 낮음

---

### D09: 로깅 및 감사 추적

**상태**: FAIL

**검증 내용**:
- 현재 로깅 수준: OutputDebugString 기반 디버그 로그만 존재
- 파일 로드 이벤트 로깅: 없음
- API 키 사용 이벤트 로깅: 없음
- 오류/예외 로깅: 일부 MessageBox 표시로 대체

**발견된 문제**:
1. 구조화된 로그 파일(`%APPDATA%\DeepMetria\logs\`) 부재
2. 보안 이벤트(파일 접근, 설정 변경) 추적 불가능
3. 크래시 덤프 생성 및 전송 메커니즘 없음

**위험도**: 중간 (디버깅 및 사고 대응 불가)

---

### D10: 업데이트 및 공급망 보안

**상태**: 미검증 (배포 설정 필요)

**권장 사항**:
- 자동 업데이트: HTTPS 서명 검증 후 설치
- 의존성 DLL: vcpkg를 통한 고정 버전 관리
- 공급망 무결성: 빌드 산출물의 SHA-256 해시 게시

---

## 3. 추가 보안 점검 항목

### 3.1 API 키 메모리 보호

**상태**: PARTIAL

**검증**:
- LLM API 키를 `DPAPI`로 암호화 후 레지스트리 저장
- 메모리 로드 후 사용 완료 시 `SecureZeroMemory()` 호출 여부
- `CString` 대신 `std::vector<BYTE>` + 수동 소거 패턴 권장

**주의**: 메모리 덤프 분석 시 평문 키 노출 가능성 확인 필요

---

### 3.2 파일 업로드 / 로드 매직 넘버 검증

**상태**: PARTIAL

**검증**:
- 확장자 기반 검증: `_DetectFileType()` 구현
- 파일 헤더 매직 넘버 검증:
  - CSV: BOM 또는 UTF-8 텍스트 헤더
  - XLSX: PK\x03\x04 (ZIP 헤더)
  - XLS: `\xD0\xCF\x11\xE0` (OLE2 헤더)
  - JSON: `{` 또는 `[` 시작

**권장 사항**: 매직 넘버 검증을 확장자와 독립적으로 실행

---

### 3.3 자격증명 저장 보안

**상태**: PARTIAL

**검증**:
- DPAPI 암호화: `CryptProtectData()` 사용 경로 확인
- 레지스트리 접근 제어: `HKCU` 키 ACL 기본값 사용 (사용자 전용)
- API 키 화면 표시: 마스킹 처리 여부 (`****` 표시)

**발견된 문제**:
- 일부 설정값이 레지스트리에 REG_SZ(평문)으로 저장됨
- 로그아웃 시 메모리 내 자격증명 소거 미완료

---

### 3.4 DLL 의존성 보안

**상태**: PARTIAL

**현황**:
- vcpkg 관리 DLL: Google Test, nlohmann-json, libcurl
- 시스템 DLL: MFC, CRT (Visual Studio 재배포 패키지)

**권장 사항**:
- 서드파티 DLL 디지털 서명 검증 빌드 단계 추가
- `LOAD_LIBRARY_SEARCH_SYSTEM32` 플래그로 시스템 DLL 우선 로드
- 앱 디렉터리 내 DLL은 서명 필수

---

### 3.5 HTTPS 및 인증서 관리

**상태**: PASS

**검증**:
- WinHTTP: `WINHTTP_FLAG_SECURE` 강제
- TLS 최소 버전: 1.2 이상
- 인증서 오류 무시 코드 없음 (`SECURITY_FLAG_IGNORE_*` 미사용)

**발견된 문제**: 없음

---

### 3.6 로컬 데이터 파일 접근 제어

**상태**: PASS

**현황**:
- 데이터 저장 경로: `%USERPROFILE%\Documents\DeepMetria\data\`
- 경로 탈출 방지: `PathCanonicalize()` + 허용 경로 프리픽스 검사

**권장 사항**:
- 향후 멀티사용자 환경 지원 시 ACL 명시적 설정 필요

---

## 4. 발견된 취약점 및 문제 사항

### 심각도: 높음 (High)

1. **D01 API 키 평문 저장**
   - DPAPI 없이 레지스트리에 평문 저장되는 경로 존재
   - 영향: API 키 탈취 → LLM 서비스 무단 사용
   - 조치: `CryptProtectData()` 적용 필수

2. **D09 로깅 부재**
   - 구조화된 로그 파일 없음
   - 영향: 보안 사고 추적 불가, 크래시 원인 분석 어려움

### 심각도: 중간 (Medium)

1. **D02 비보안 문자열 함수 사용**
   - `sprintf`, `strcpy` 2건 (PVS-Studio V576)
   - 조치: `sprintf_s`, `strcpy_s`로 대체

2. **D03 DLL 상대 경로 로드**
   - `LoadLibrary` 상대 경로 2건 → DLL 사이드로딩 위험
   - 조치: `LoadLibraryEx` + `LOAD_LIBRARY_SEARCH_SYSTEM32`

3. **D07 코드 서명 미완료**
   - 배포 실행 파일 서명 없음
   - 영향: Windows SmartScreen 경고, 신뢰도 저하

4. **D09 크래시 덤프 없음**
   - 비정상 종료 시 원인 추적 불가

### 심각도: 낮음 (Low)

1. **D08 Excel 매크로 검사 없음**
   - VBA 매크로 포함 파일 거부 로직 미흡

2. **D03 서드파티 DLL 서명 검증 없음**
   - vcpkg DLL 서명 확인 단계 부재

---

## 5. 권장 조치 사항

### 즉시 조치 (Critical) — 배포 전

1. **API 키 DPAPI 암호화**
   ```cpp
   // SettingsManager.cpp — API 키 저장
   DATA_BLOB plainBlob = { (DWORD)key.size(), (BYTE*)key.data() };
   DATA_BLOB encBlob = {};
   CryptProtectData(&plainBlob, L"APIKey", nullptr, nullptr, nullptr, 0, &encBlob);
   // 레지스트리에 encBlob.pbData 저장
   ```

2. **비보안 문자열 함수 대체**
   ```cpp
   // Before
   sprintf(buf, "%s", input);
   // After
   sprintf_s(buf, sizeof(buf), "%s", input);
   ```

3. **DLL 로드 경로 보안**
   ```cpp
   // Before
   LoadLibrary(L"SomeDll.dll");
   // After
   LoadLibraryEx(L"SomeDll.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
   ```

4. **구조화 로그 구현**
   ```cpp
   // %APPDATA%\DeepMetria\logs\app-YYYY-MM-DD.log
   // 로그 항목: 타임스탬프, 레벨, 이벤트 유형, 메시지
   // 보안 이벤트: FILE_LOAD, API_KEY_USE, SETTING_CHANGE, ERROR
   ```

### 단기 조치 (Important) — 배포 후 1개월 내

1. **코드 서명 인증서 취득 및 적용**
   ```
   - EV Code Signing Certificate 취득
   - signtool.exe로 빌드 후 자동 서명 (CI 파이프라인)
   - SmartScreen 평판 구축
   ```

2. **메모리 내 자격증명 안전 소거**
   ```cpp
   // 사용 완료 후
   SecureZeroMemory(apiKeyBuffer, sizeof(apiKeyBuffer));
   ```

3. **Excel 매크로 포함 파일 거부**
   ```cpp
   // libxlsxwriter 또는 OpenXLSX로 VBA 스트림 존재 확인
   if (HasVBAStream(filePath))
       throw CUnsupportedFileException(L"매크로 포함 파일은 지원하지 않습니다.");
   ```

4. **크래시 덤프 생성**
   ```cpp
   // SetUnhandledExceptionFilter로 미니덤프 생성
   SetUnhandledExceptionFilter(DeepMetriaCrashHandler);
   // %LOCALAPPDATA%\DeepMetria\CrashDumps\ 저장
   ```

### 장기 조치 (Enhancement) — 배포 후 3개월 내

1. **인증서 핀닝 (Certificate Pinning)**
   - LLM API 엔드포인트 인증서 핀 설정
   - 핀 갱신 메커니즘 구현

2. **자동 업데이트 보안 채널**
   - HTTPS + 서명 검증 후 업데이트 패치 적용
   - 롤백 메커니즘 구현

3. **보안 감사 (Penetration Testing)**
   - 메모리 포렌식 (API 키 노출 여부)
   - DLL 인젝션 시도 테스트
   - 로컬 권한 상승 시도

4. **빌드 보안 강화**
   - `/LTCG` + `/GL` 활성화 (링크 타임 코드 생성)
   - AddressSanitizer 빌드 옵션 CI 적용

---

## 점검 결과 요약

| 항목 | 상태 | 심각도 |
|------|------|--------|
| D01 로컬 자격증명 및 API 키 보안 | PARTIAL | High |
| D02 메모리 안전성 | PARTIAL | Medium |
| D03 DLL 사이드로딩 방지 | PARTIAL | Medium |
| D04 파일 시스템 보안 | PASS | — |
| D05 네트워크 통신 보안 | PASS | — |
| D06 프로세스 권한 최소화 | PASS | — |
| D07 실행 파일 보안 | PARTIAL | Medium |
| D08 입력 검증 | PARTIAL | Low |
| D09 로깅 및 감사 추적 | FAIL | Medium |
| D10 업데이트 및 공급망 보안 | 미검증 | — |
| 추가 항목 | PARTIAL | Medium |

**전체 평가**: 네트워크 보안, 파일 시스템 접근 제어, 프로세스 권한은 우수하나, API 키 암호화, 비보안 함수 제거, 로그 구현이 취약함. **배포 전 DPAPI 적용 및 비보안 함수 대체 필수**.
