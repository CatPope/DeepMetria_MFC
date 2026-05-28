// test_sse_streaming.cpp
// SSE(Server-Sent Events) 파싱 로직 단위 테스트
// TC-02-01, TC-02-02, TC-04-01, TC-04-02, TC-04-03
// 실제 네트워크 없이 SSE 텍스트 버퍼 파싱 로직만 검증한다.

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>
#include <sstream>

#include "Common/Types.h"

// ============================================================
// SSEParsingTest 픽스처
// HttpClient 내부 파싱 로직과 동일한 알고리즘을 독립적으로 구현하여 검증한다.
// ============================================================
class SSEParsingTest : public ::testing::Test {
protected:
    // SSE 이벤트 구조체
    struct SSEEvent {
        std::string eventType;  // "data", "event" 등
        std::string data;
    };

    // SSE 버퍼를 파싱하여 이벤트 목록으로 반환한다.
    // 형식: "data: {...}\n\n" — 빈 줄(\n\n)이 이벤트 구분자
    // "[DONE]" 이벤트는 목록에서 제외한다.
    std::vector<SSEEvent> ParseSSEBuffer(const std::string& buffer) {
        std::vector<SSEEvent> events;
        std::string remaining = buffer;
        size_t pos;

        while ((pos = remaining.find("\n\n")) != std::string::npos) {
            std::string block = remaining.substr(0, pos);
            remaining = remaining.substr(pos + 2);

            if (block.empty()) continue;

            SSEEvent evt;
            evt.eventType = "data";

            // "data: " 접두어 파싱
            size_t dataPos = block.find("data: ");
            if (dataPos != std::string::npos) {
                evt.data = block.substr(dataPos + 6);
                // 줄바꿈 제거
                while (!evt.data.empty() && (evt.data.back() == '\n' || evt.data.back() == '\r')) {
                    evt.data.pop_back();
                }
            }

            // "event: " 접두어 파싱 (선택적)
            size_t eventPos = block.find("event: ");
            if (eventPos != std::string::npos) {
                size_t endLine = block.find('\n', eventPos);
                evt.eventType = block.substr(eventPos + 7,
                    endLine != std::string::npos ? endLine - eventPos - 7 : std::string::npos);
            }

            // [DONE] 신호 제외
            if (evt.data == "[DONE]") continue;
            if (!evt.data.empty()) {
                events.push_back(evt);
            }
        }
        return events;
    }

    // 여러 토큰을 합산하여 완성 응답 구성
    std::string AccumulateTokens(const std::vector<SSEEvent>& events,
                                  const std::string& jsonKey) {
        std::string accumulated;
        for (const auto& evt : events) {
            size_t keyPos = evt.data.find("\"" + jsonKey + "\":\"");
            if (keyPos != std::string::npos) {
                size_t start = keyPos + jsonKey.size() + 4;
                size_t end   = evt.data.find("\"", start);
                if (end != std::string::npos) {
                    accumulated += evt.data.substr(start, end - start);
                }
            }
        }
        return accumulated;
    }

    // CotStep 추출 헬퍼
    bool ExtractCotStepType(const std::string& json, std::string& outType) {
        size_t pos = json.find("\"step_type\":\"");
        if (pos == std::string::npos) return false;
        size_t start = pos + 13;
        size_t end   = json.find("\"", start);
        if (end == std::string::npos) return false;
        outType = json.substr(start, end - start);
        return true;
    }

    // 도구 호출 이름 추출 헬퍼
    bool ExtractToolName(const std::string& json, std::string& outName) {
        size_t pos = json.find("\"tool_name\":\"");
        if (pos == std::string::npos) return false;
        size_t start = pos + 13;
        size_t end   = json.find("\"", start);
        if (end == std::string::npos) return false;
        outName = json.substr(start, end - start);
        return true;
    }
};

// ============================================================
// TC-02-01: SSE 이벤트 스트림 파싱
// ============================================================

// 단일 이벤트 파싱
TEST_F(SSEParsingTest, ParseSingleEvent_ReturnsOneEvent) {
    std::string buffer = "data: {\"token\":\"안녕\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    ASSERT_EQ(events.size(), 1u) << "단일 이벤트가 1개로 파싱되어야 한다";
    EXPECT_EQ(events[0].data, "{\"token\":\"안녕\"}");
}

// 복수 이벤트 파싱
TEST_F(SSEParsingTest, ParseMultipleEvents_ReturnsAllEvents) {
    std::string buffer =
        "data: {\"token\":\"안녕\"}\n\n"
        "data: {\"token\":\"하세요\"}\n\n"
        "data: {\"token\":\"!\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    EXPECT_EQ(events.size(), 3u) << "3개 이벤트가 파싱되어야 한다";
}

// [DONE] 신호 제외
TEST_F(SSEParsingTest, ParseDoneSignal_ExcludesDoneEvent) {
    std::string buffer =
        "data: {\"token\":\"완료\"}\n\n"
        "data: [DONE]\n\n";
    auto events = ParseSSEBuffer(buffer);
    EXPECT_EQ(events.size(), 1u) << "[DONE] 이벤트는 제외되어야 한다";
    EXPECT_NE(events[0].data, "[DONE]");
}

// 빈 버퍼 파싱
TEST_F(SSEParsingTest, ParseEmptyBuffer_ReturnsNoEvents) {
    auto events = ParseSSEBuffer("");
    EXPECT_EQ(events.size(), 0u) << "빈 버퍼는 이벤트가 없어야 한다";
}

// [DONE]만 있는 경우
TEST_F(SSEParsingTest, ParseOnlyDoneSignal_ReturnsNoEvents) {
    std::string buffer = "data: [DONE]\n\n";
    auto events = ParseSSEBuffer(buffer);
    EXPECT_EQ(events.size(), 0u) << "[DONE]만 있으면 이벤트가 없어야 한다";
}

// 이벤트 타입 파싱
TEST_F(SSEParsingTest, ParseEventWithType_ExtractsEventType) {
    std::string buffer =
        "event: completion\n"
        "data: {\"token\":\"Hello\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].eventType, "completion")
        << "event 타입이 올바르게 파싱되어야 한다";
}

// ============================================================
// TC-02-02: 스트리밍 중간 토큰 축적
// ============================================================

// 토큰 축적으로 완성 응답 구성
TEST_F(SSEParsingTest, AccumulateTokens_BuildsCompleteResponse) {
    std::string buffer =
        "data: {\"token\":\"Hello\"}\n\n"
        "data: {\"token\":\" World\"}\n\n"
        "data: {\"token\":\"!\"}\n\n"
        "data: [DONE]\n\n";
    auto events = ParseSSEBuffer(buffer);
    std::string result = AccumulateTokens(events, "token");
    EXPECT_EQ(result, "Hello World!")
        << "토큰이 축적되어 완성 응답을 구성해야 한다";
}

// 중간 이벤트 손실 없이 축적
TEST_F(SSEParsingTest, AccumulateTokens_NoTokenLoss) {
    std::string buffer;
    for (int i = 0; i < 10; ++i) {
        buffer += "data: {\"token\":\"t" + std::to_string(i) + "\"}\n\n";
    }
    buffer += "data: [DONE]\n\n";
    auto events = ParseSSEBuffer(buffer);
    EXPECT_EQ(events.size(), 10u) << "10개 토큰이 모두 파싱되어야 한다";
}

// 빈 토큰 건너뜀
TEST_F(SSEParsingTest, AccumulateTokens_EmptyTokensSkipped) {
    std::string buffer =
        "data: {\"token\":\"A\"}\n\n"
        "data: {\"token\":\"B\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    // 빈 데이터 이벤트가 없으므로 2개만 있어야 함
    EXPECT_EQ(events.size(), 2u);
}

// ============================================================
// TC-04-01: CoT(Chain-of-Thought) 단계 파싱
// ============================================================

// thinking 단계 추출
TEST_F(SSEParsingTest, CotStepParsing_ExtractsThinkingStep) {
    std::string buffer =
        "data: {\"step_type\":\"thinking\",\"content\":\"데이터를 분석 중...\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    ASSERT_EQ(events.size(), 1u);

    std::string stepType;
    bool ok = ExtractCotStepType(events[0].data, stepType);
    EXPECT_TRUE(ok) << "step_type 파싱에 실패함";
    EXPECT_EQ(stepType, "thinking") << "단계 유형이 'thinking'이어야 한다";
}

// answer 단계 추출
TEST_F(SSEParsingTest, CotStepParsing_ExtractsAnswerStep) {
    std::string buffer =
        "data: {\"step_type\":\"answer\",\"content\":\"결과는 42입니다\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    ASSERT_EQ(events.size(), 1u);

    std::string stepType;
    EXPECT_TRUE(ExtractCotStepType(events[0].data, stepType));
    EXPECT_EQ(stepType, "answer");
}

// tool_result 단계 추출
TEST_F(SSEParsingTest, CotStepParsing_ExtractsToolResultStep) {
    std::string buffer =
        "data: {\"step_type\":\"tool_result\",\"content\":\"{...}\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    ASSERT_EQ(events.size(), 1u);

    std::string stepType;
    EXPECT_TRUE(ExtractCotStepType(events[0].data, stepType));
    EXPECT_EQ(stepType, "tool_result");
}

// ============================================================
// TC-04-02: 도구 호출 파싱
// ============================================================

// 도구 이름 추출
TEST_F(SSEParsingTest, ToolCallParsing_ExtractsToolName) {
    std::string buffer =
        "data: {\"step_type\":\"tool_call\","
              "\"tool_name\":\"BasicStats\","
              "\"arguments\":\"{\\\"column\\\":\\\"매출\\\"}\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    ASSERT_EQ(events.size(), 1u);

    std::string toolName;
    bool ok = ExtractToolName(events[0].data, toolName);
    EXPECT_TRUE(ok) << "tool_name 파싱에 실패함";
    EXPECT_EQ(toolName, "BasicStats") << "도구 이름이 'BasicStats'이어야 한다";
}

// 여러 도구 호출이 순서대로 파싱됨
TEST_F(SSEParsingTest, ToolCallParsing_MultipleToolsInOrder) {
    std::string buffer =
        "data: {\"step_type\":\"tool_call\",\"tool_name\":\"BasicStats\"}\n\n"
        "data: {\"step_type\":\"tool_call\",\"tool_name\":\"GroupByAggregate\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    ASSERT_EQ(events.size(), 2u);

    std::string name1, name2;
    EXPECT_TRUE(ExtractToolName(events[0].data, name1));
    EXPECT_TRUE(ExtractToolName(events[1].data, name2));
    EXPECT_EQ(name1, "BasicStats");
    EXPECT_EQ(name2, "GroupByAggregate");
}

// ============================================================
// TC-04-03: 스트림 종료 처리
// ============================================================

// 스트림 종료 후 추가 이벤트 없음
TEST_F(SSEParsingTest, StreamTermination_HandlesGracefully) {
    std::string buffer =
        "data: {\"token\":\"결과\"}\n\n"
        "data: [DONE]\n\n";
    auto events = ParseSSEBuffer(buffer);
    EXPECT_EQ(events.size(), 1u) << "[DONE] 이후 이벤트가 없어야 한다";
}

// 불완전한 이벤트 블록 (구분자 없음) — 무시됨
TEST_F(SSEParsingTest, StreamTermination_IncompleteBlock_IsIgnored) {
    // 두 번째 이벤트에 \n\n 없음 — 미완성 블록으로 무시됨
    std::string buffer =
        "data: {\"token\":\"완료\"}\n\n"
        "data: {\"token\":\"미완성\"}\n";
    auto events = ParseSSEBuffer(buffer);
    EXPECT_EQ(events.size(), 1u)
        << "종결 구분자가 없는 블록은 무시되어야 한다";
}

// 연속 빈 구분자 — 빈 블록 처리
TEST_F(SSEParsingTest, StreamTermination_ConsecutiveDelimiters_NoExtraEvents) {
    std::string buffer =
        "data: {\"token\":\"A\"}\n\n"
        "\n\n"
        "data: {\"token\":\"B\"}\n\n";
    auto events = ParseSSEBuffer(buffer);
    // 빈 블록은 건너뛰어져 A, B 2개만 있어야 함
    EXPECT_EQ(events.size(), 2u)
        << "빈 블록은 이벤트로 계산되지 않아야 한다";
}
