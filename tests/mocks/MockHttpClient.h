#pragma once
// MockHttpClient.h — HttpClient 목 구현
//
// HttpClient::PostJson 은 정적 메서드이므로 직접 목킹이 불가능하다.
// 래퍼 인터페이스 IHttpClient 를 도입하고, 프로덕션 코드에서
// IHttpClient 포인터 주입 방식으로 교체하거나,
// 테스트에서 MockHttpClient 를 주입하여 HTTP 호출을 가로챈다.

#include <gmock/gmock.h>
#include "Common/Types.h"
#include <string>
#include <vector>

// ============================================================
// IHttpClient — HTTP 전송 추상 인터페이스
// HttpClient 정적 메서드를 인스턴스 메서드로 래핑한다.
// ============================================================
class IHttpClient {
public:
    virtual ~IHttpClient() {}

    // JSON POST 요청 실행. HTTP 200 반환 시 TRUE.
    // url        : 요청 URL (std::string)
    // jsonBody   : 요청 바디 (JSON 문자열)
    // headers    : 추가 헤더 목록 (예: "Authorization: Bearer xxx")
    // outBody    : 응답 바디 (out)
    // outError   : 오류 정보 (out)
    // timeoutSec : 타임아웃 (초, 기본 30)
    virtual BOOL PostJson(const std::string&              url,
                          const std::string&              jsonBody,
                          const std::vector<std::string>& headers,
                          std::string&                    outBody,
                          AppError&                       outError,
                          int                             timeoutSec = 30) = 0;
};

// ============================================================
// MockHttpClient — IHttpClient 전체 메서드 목
// ============================================================
class MockHttpClient : public IHttpClient {
public:
    MOCK_METHOD(BOOL, PostJson,
        (const std::string&              url,
         const std::string&              jsonBody,
         const std::vector<std::string>& headers,
         std::string&                    outBody,
         AppError&                       outError,
         int                             timeoutSec),
        (override));

    // ── 헬퍼: 성공 응답 고정값 설정 ───────────────────────
    // 호출 시 outBody 에 fixedBody 를 기록하고 TRUE 를 반환하도록 설정한다.
    void SetupSuccessResponse(const std::string& fixedBody) {
        using ::testing::_;
        using ::testing::DoAll;
        using ::testing::Return;
        using ::testing::SetArgReferee;

        ON_CALL(*this, PostJson(_, _, _, _, _, _))
            .WillByDefault(DoAll(
                SetArgReferee<3>(fixedBody),
                Return(TRUE)));
    }

    // ── 헬퍼: HTTP 오류 응답 설정 ─────────────────────────
    // 호출 시 outError 를 채우고 FALSE 를 반환하도록 설정한다.
    void SetupErrorResponse(const CString& errorCode,
                            const CString& errorMessage,
                            int severity = 2) {
        using ::testing::_;
        using ::testing::DoAll;
        using ::testing::Return;
        using ::testing::SetArgReferee;

        AppError err(errorCode, errorMessage, severity);

        ON_CALL(*this, PostJson(_, _, _, _, _, _))
            .WillByDefault(DoAll(
                SetArgReferee<4>(err),
                Return(FALSE)));
    }

    // ── 헬퍼: 특정 URL 패턴에만 성공 응답 설정 ───────────
    void SetupSuccessForUrl(const std::string& expectedUrl,
                            const std::string& fixedBody) {
        using ::testing::_;
        using ::testing::DoAll;
        using ::testing::HasSubstr;
        using ::testing::Return;
        using ::testing::SetArgReferee;

        ON_CALL(*this, PostJson(HasSubstr(expectedUrl), _, _, _, _, _))
            .WillByDefault(DoAll(
                SetArgReferee<3>(fixedBody),
                Return(TRUE)));
    }
};

// ============================================================
// HttpClientStaticShim — 정적 메서드 후킹용 함수 포인터 심
//
// 프로덕션 코드에서 HttpClient::PostJson 을 직접 호출하는 경우,
// 링크 단계에서 아래 전역 포인터를 통해 목으로 교체할 수 있다.
//
// 사용법 (테스트 SetUp):
//   g_httpPostJsonFn = [&](auto& url, auto& body, auto& hdrs,
//                          auto& out, auto& err, int t) {
//       return mockClient.PostJson(url, body, hdrs, out, err, t);
//   };
// 사용법 (테스트 TearDown):
//   g_httpPostJsonFn = nullptr;
// ============================================================
#include <functional>

using HttpPostJsonFn = std::function<
    BOOL(const std::string&,
         const std::string&,
         const std::vector<std::string>&,
         std::string&,
         AppError&,
         int)>;

// 테스트 바이너리에서만 정의. 프로덕션 코드에서는 사용 금지.
inline HttpPostJsonFn g_httpPostJsonFn = nullptr;
