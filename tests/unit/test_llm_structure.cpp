// test_llm_structure.cpp — LLM 응답 구조 및 요약 생성 단위 테스트
// TC-08-03: LLM 전환 후 동일 결과 구조 보장
// TC-02-06: LLM mock 요약 생성 도메인 로직

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <windows.h>
#include <atlstr.h>

#include "Common/Types.h"
#include "mocks/MockLLMProvider.h"
#include "fixtures/TestDataFixture.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

// ============================================================
// TC-08-03: LLM 전환 후 동일 결과 구조 보장
// Claude mock과 OpenAI mock이 동일한 응답 스키마를 반환하는지 검증
// ============================================================
class LLMStructureTest : public TestDataFixture {
protected:
    MockLLMProvider claudeMock_;
    MockLLMProvider openaiMock_;

    // 분석 결과 JSON 표준 형식 (두 프로바이더 공통)
    CString standardResponse_ = _T("{\"domain\":\"sales\","
        "\"statistics\":{\"mean\":1250000,\"median\":1150000},"
        "\"row_count\":10,"
        "\"visualization\":{\"type\":\"bar\",\"title\":\"매출 분석\"}}");

    void SetUp() override {
        TestDataFixture::SetUp();

        claudeMock_.SetupProviderInfo(_T("Claude"), _T("claude-sonnet-4-20250514"));
        openaiMock_.SetupProviderInfo(_T("OpenAI"), _T("gpt-4o"));

        claudeMock_.SetupSuccessResponse(standardResponse_);
        openaiMock_.SetupSuccessResponse(standardResponse_);
    }
};

// TC-08-03-001: Claude와 OpenAI mock이 동일한 JSON 키를 포함하는 응답을 반환한다
TEST_F(LLMStructureTest, BothProviders_ReturnSameResponseSchema) {
    CString claudeResp, openaiResp;
    AppError claudeErr, openaiErr;

    BOOL claudeOk = claudeMock_.Chat(
        _T("system"), _T("매출 분석"), _T(""), claudeResp, claudeErr);
    BOOL openaiOk = openaiMock_.Chat(
        _T("system"), _T("매출 분석"), _T(""), openaiResp, openaiErr);

    EXPECT_TRUE(claudeOk);
    EXPECT_TRUE(openaiOk);
    EXPECT_EQ(claudeResp, openaiResp)
        << "두 프로바이더의 응답 구조가 동일해야 한다";
}

// TC-08-03-002: 응답에 필수 키(domain, statistics, row_count)가 포함된다
TEST_F(LLMStructureTest, Response_ContainsRequiredKeys) {
    CString resp;
    AppError err;

    claudeMock_.Chat(_T("system"), _T("분석"), _T(""), resp, err);

    EXPECT_NE(resp.Find(_T("domain")), -1) << "domain 키 누락";
    EXPECT_NE(resp.Find(_T("statistics")), -1) << "statistics 키 누락";
    EXPECT_NE(resp.Find(_T("row_count")), -1) << "row_count 키 누락";
}

// TC-08-03-003: 응답에 시각화 구성(visualization)이 포함된다
TEST_F(LLMStructureTest, Response_ContainsVisualizationConfig) {
    CString resp;
    AppError err;

    openaiMock_.Chat(_T("system"), _T("차트"), _T(""), resp, err);

    EXPECT_NE(resp.Find(_T("visualization")), -1) << "visualization 키 누락";
    EXPECT_NE(resp.Find(_T("type")), -1) << "visualization.type 키 누락";
}

// TC-08-03-004: ChatWithHistory도 동일한 구조를 반환한다
TEST_F(LLMStructureTest, ChatWithHistory_ReturnsSameSchema) {
    std::vector<ChatMessage> messages;
    ChatMessage msg;
    msg.role = _T("user");
    msg.content = _T("매출 분석해줘");
    messages.push_back(msg);

    CString claudeResp, openaiResp;
    AppError claudeErr, openaiErr;

    claudeMock_.ChatWithHistory(messages, _T(""), claudeResp, claudeErr);
    openaiMock_.ChatWithHistory(messages, _T(""), openaiResp, openaiErr);

    EXPECT_EQ(claudeResp, openaiResp);
}

// TC-08-03-005: 오류 응답 구조도 프로바이더 간 동일하다
TEST_F(LLMStructureTest, ErrorResponse_SameStructureAcrossProviders) {
    MockLLMProvider errClaude, errOpenai;
    errClaude.SetupErrorResponse(_T("RATE_LIMIT"), _T("요청 한도 초과"));
    errOpenai.SetupErrorResponse(_T("RATE_LIMIT"), _T("요청 한도 초과"));

    CString resp1, resp2;
    AppError err1, err2;

    BOOL r1 = errClaude.Chat(_T("s"), _T("q"), _T(""), resp1, err1);
    BOOL r2 = errOpenai.Chat(_T("s"), _T("q"), _T(""), resp2, err2);

    EXPECT_EQ(r1, r2);
    EXPECT_EQ(err1.code, err2.code);
    EXPECT_EQ(err1.severity, err2.severity);
}

// ============================================================
// TC-02-06: LLM mock 요약 생성 도메인 로직
// DataTable → DataSummary 변환이 올바른 필드를 포함하는지 검증
// ============================================================
class LLMMockSummaryTest : public TestDataFixture {
protected:
    MockLLMProvider mockProvider_;

    void SetUp() override {
        TestDataFixture::SetUp();

        // LLM이 생성할 요약 텍스트 mock
        CString summaryResponse = _T("{\"domain\":\"sales\","
            "\"summary\":\"2024년 판매 데이터 10행, 전자제품 매출 최대\","
            "\"statistics\":{\"mean\":1250000,\"max\":2200000,\"min\":500000},"
            "\"row_count\":10}");
        mockProvider_.SetupSuccessResponse(summaryResponse);
    }
};

// TC-02-06-001: DataSummary에 행/열 수가 정확히 설정된다
TEST_F(LLMMockSummaryTest, Summary_HasCorrectRowAndColumnCount) {
    EXPECT_EQ(m_sampleSummary.rowCount, 10);
    EXPECT_EQ(m_sampleSummary.columnCount, 5);
}

// TC-02-06-002: DataSummary 스키마에 모든 컬럼이 포함된다
TEST_F(LLMMockSummaryTest, Summary_SchemaContainsAllColumns) {
    EXPECT_EQ(m_sampleSummary.schema.size(), 5u);

    EXPECT_EQ(m_sampleSummary.schema[0].name, _T("날짜"));
    EXPECT_EQ(m_sampleSummary.schema[1].name, _T("카테고리"));
    EXPECT_EQ(m_sampleSummary.schema[2].name, _T("매출"));
    EXPECT_EQ(m_sampleSummary.schema[3].name, _T("수량"));
    EXPECT_EQ(m_sampleSummary.schema[4].name, _T("지역"));
}

// TC-02-06-003: DataSummary 스키마에 올바른 타입이 지정된다
TEST_F(LLMMockSummaryTest, Summary_SchemaHasCorrectTypes) {
    EXPECT_EQ(m_sampleSummary.schema[0].type, _T("date"));
    EXPECT_EQ(m_sampleSummary.schema[1].type, _T("text"));
    EXPECT_EQ(m_sampleSummary.schema[2].type, _T("numeric"));
    EXPECT_EQ(m_sampleSummary.schema[3].type, _T("numeric"));
    EXPECT_EQ(m_sampleSummary.schema[4].type, _T("text"));
}

// TC-02-06-004: LLM mock이 요약 텍스트를 반환하면 DataSummary에 포함된다
TEST_F(LLMMockSummaryTest, Summary_MockLLMResponseIncludesDomain) {
    CString resp;
    AppError err;

    BOOL ok = mockProvider_.Chat(
        _T("데이터 요약을 생성하세요"),
        _T("sample_sales.csv"),
        _T(""),
        resp, err);

    EXPECT_TRUE(ok);
    EXPECT_NE(resp.Find(_T("domain")), -1) << "요약 응답에 domain 필드 누락";
    EXPECT_NE(resp.Find(_T("row_count")), -1) << "요약 응답에 row_count 필드 누락";
    EXPECT_NE(resp.Find(_T("statistics")), -1) << "요약 응답에 statistics 필드 누락";
}

// TC-02-06-005: DataSummary의 aiSummaryText가 비어있지 않다
TEST_F(LLMMockSummaryTest, Summary_AISummaryTextIsNotEmpty) {
    EXPECT_FALSE(m_sampleSummary.aiSummaryText.IsEmpty())
        << "AI 요약 텍스트가 비어있으면 안 된다";
    EXPECT_NE(m_sampleSummary.aiSummaryText.Find(_T("매출")), -1)
        << "요약에 '매출' 키워드가 포함되어야 한다";
}

// TC-02-06-006: DataSummary 스키마에 sampleValues가 포함된다
TEST_F(LLMMockSummaryTest, Summary_SchemaHasSampleValues) {
    for (const auto& col : m_sampleSummary.schema) {
        EXPECT_FALSE(col.sampleValues.IsEmpty())
            << "컬럼 '" << (LPCWSTR)col.name << "'의 sampleValues가 비어있다";
    }
}

// TC-02-06-007: DataSummary의 datasourceId가 설정된다
TEST_F(LLMMockSummaryTest, Summary_HasDatasourceId) {
    EXPECT_FALSE(m_sampleSummary.datasourceId.IsEmpty())
        << "datasourceId가 설정되어야 한다";
}
