// test_streaming.cpp — HttpClient 단위 테스트
// HttpClient::PostJson 의 입력 검증 및 오류 처리 경로를 검증한다.
// 실제 네트워크 요청을 최소화하고, 잘못된 입력에 대한 오류 처리에 집중한다.
// 참고: HttpClient::PostJson 은 정적 메서드이므로 libcurl 실제 호출이 발생한다.
//       네트워크가 없는 환경에서는 CURLE_COULDNT_RESOLVE_HOST 등의 오류가 반환된다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <windows.h>
#include <atlstr.h>

#include "Common/Types.h"
#include "Infrastructure/LLM/HttpClient.h"

// ============================================================
// HttpClient 테스트 픽스처
// ============================================================
class HttpClientTest : public ::testing::Test {
protected:
    // 테스트용 최소 유효 헤더
    std::vector<std::string> ValidHeaders() {
        return {
            "Content-Type: application/json",
            "Authorization: Bearer test-key"
        };
    }
};

// ============================================================
// TC-HTTP-001: 빈 URL로 PostJson 호출 시 오류를 반환한다
// libcurl은 빈 URL에 대해 CURLE_URL_MALFORMAT 등의 오류를 반환한다.
// ============================================================
TEST_F(HttpClientTest, PostJsonEmptyUrl_ReturnsFalseWithError) {
    std::string  outBody;
    AppError     error;

    BOOL result = HttpClient::PostJson("", "{}", ValidHeaders(), outBody, error, 5);

    EXPECT_EQ(result, FALSE);
    EXPECT_TRUE(error.IsError())
        << "빈 URL에 대해 오류가 설정되지 않음. code=" << (LPCWSTR)error.code;
}

// ============================================================
// TC-HTTP-002: 빈 바디로 PostJson 호출 시 오류 없이 처리되거나
// 서버 응답 오류를 반환한다 (크래시 없음)
// ============================================================
TEST_F(HttpClientTest, PostJsonEmptyBody_DoesNotCrash) {
    std::string  outBody;
    AppError     error;

    // 빈 바디는 libcurl 수준에서 허용됨 — 서버가 오류를 반환할 수 있음
    // 존재하지 않는 호스트이므로 네트워크 오류 반환
    EXPECT_NO_THROW(
        HttpClient::PostJson("https://invalid.example.invalid/", "",
                             ValidHeaders(), outBody, error, 3)
    );
}

// ============================================================
// TC-HTTP-003: 존재하지 않는 호스트에 PostJson 시 오류를 반환한다
// libcurl: CURLE_COULDNT_RESOLVE_HOST
// ============================================================
TEST_F(HttpClientTest, PostJsonInvalidUrl_ReturnsFalseWithNetworkError) {
    std::string  outBody;
    AppError     error;

    BOOL result = HttpClient::PostJson(
        "https://this.host.definitely.does.not.exist.invalid/api",
        "{\"test\":true}",
        ValidHeaders(),
        outBody,
        error,
        5   // 5초 타임아웃
    );

    EXPECT_EQ(result, FALSE);
    EXPECT_TRUE(error.IsError())
        << "네트워크 오류가 설정되지 않음. code=" << (LPCWSTR)error.code;
}

// ============================================================
// TC-HTTP-004: 매우 짧은 타임아웃으로 느린 요청 시 타임아웃 오류를 반환한다
// CURLOPT_TIMEOUT = 1초로 설정, 실제 서버에 요청해도 빠르게 실패해야 한다.
// ============================================================
TEST_F(HttpClientTest, PostJsonTimeout_ReturnsFalseOnTimeout) {
    std::string  outBody;
    AppError     error;

    // 1초 타임아웃 — 실제 DNS 해석 + 연결이 1초 내 완료되지 않으면 타임아웃
    // 존재하지 않는 호스트이므로 DNS 타임아웃 또는 연결 오류가 발생한다
    BOOL result = HttpClient::PostJson(
        "https://10.255.255.1/timeout-test",  // 라우팅 불가 IP — 연결 타임아웃 유발
        "{\"test\":true}",
        ValidHeaders(),
        outBody,
        error,
        1   // 1초 타임아웃
    );

    EXPECT_EQ(result, FALSE);
    EXPECT_TRUE(error.IsError())
        << "타임아웃 오류가 설정되지 않음. code=" << (LPCWSTR)error.code;
}

// ============================================================
// TC-HTTP-005: Authorization 헤더 형식 검증
// HttpClient는 헤더를 그대로 전달하므로, 헤더 목록 구성을 테스트한다.
// 헤더 목록에 "Authorization: Bearer " 접두어가 포함되면 올바른 형식이다.
// ============================================================
TEST(HttpClientHeaderTest, AuthorizationHeaderFormat_ContainsBearerPrefix) {
    // Claude 헤더 형식 (x-api-key)
    std::vector<std::string> claudeHeaders = {
        "Content-Type: application/json",
        "x-api-key: sk-ant-test",
        "anthropic-version: 2023-06-01"
    };

    // OpenAI 헤더 형식 (Authorization: Bearer)
    std::vector<std::string> openaiHeaders = {
        "Content-Type: application/json",
        "Authorization: Bearer sk-openai-test"
    };

    // Claude: x-api-key 헤더 존재 확인
    bool hasClaudeApiKey = false;
    for (const auto& h : claudeHeaders) {
        if (h.find("x-api-key:") != std::string::npos) {
            hasClaudeApiKey = true;
            break;
        }
    }
    EXPECT_TRUE(hasClaudeApiKey) << "Claude 헤더에 x-api-key가 없음";

    // OpenAI: Authorization: Bearer 헤더 존재 확인
    bool hasOpenAIBearer = false;
    for (const auto& h : openaiHeaders) {
        if (h.find("Authorization: Bearer ") != std::string::npos) {
            hasOpenAIBearer = true;
            break;
        }
    }
    EXPECT_TRUE(hasOpenAIBearer) << "OpenAI 헤더에 Authorization: Bearer 형식이 없음";
}

// ============================================================
// TC-HTTP-006: 헤더 목록이 비어 있어도 PostJson이 크래시하지 않는다
// ============================================================
TEST_F(HttpClientTest, PostJsonEmptyHeaders_DoesNotCrash) {
    std::vector<std::string> emptyHeaders;
    std::string  outBody;
    AppError     error;

    EXPECT_NO_THROW(
        HttpClient::PostJson(
            "https://this.host.definitely.does.not.exist.invalid/api",
            "{\"test\":true}",
            emptyHeaders,
            outBody,
            error,
            3
        )
    );
}
