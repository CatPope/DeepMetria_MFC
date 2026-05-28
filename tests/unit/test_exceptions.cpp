// test_exceptions.cpp
// 커스텀 예외 클래스 단위 테스트
// TC-04-07: CToolNotFoundException
// TC-08-04: CUnsupportedProviderException
//
// 주의: src/Common/Exceptions.h 는 별도 에이전트가 생성한다.
// 헤더가 없을 경우 컴파일되지 않으므로, Exceptions.h 생성 후 빌드해야 한다.

// MFC 헤더를 Google Test보다 먼저 포함 (PCH 순서 보장)
#include <windows.h>
#include <atlstr.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "Common/Exceptions.h"

// ============================================================
// TC-04-07: CToolNotFoundException
// ============================================================

// 도구 이름 보존 — GetToolName()이 생성자 인자를 반환해야 함
TEST(ExceptionTest, ToolNotFound_PreservesToolName) {
    CToolNotFoundException ex("InvalidTool");
    EXPECT_EQ(ex.GetToolName(), std::string("InvalidTool"))
        << "GetToolName()은 생성자에 전달된 도구 이름을 반환해야 한다";
}

// what() 메시지에 도구 이름 포함
TEST(ExceptionTest, ToolNotFound_WhatMessageContainsToolName) {
    CToolNotFoundException ex("BasicStats");
    std::string msg(ex.what());
    EXPECT_NE(msg.find("BasicStats"), std::string::npos)
        << "what() 메시지에 도구 이름 'BasicStats'가 포함되어야 한다";
}

// std::exception 상속 확인
TEST(ExceptionTest, ToolNotFound_IsStdException) {
    CToolNotFoundException ex("SomeTool");
    EXPECT_NO_THROW({
        const std::exception& base = ex;
        std::string msg(base.what());
        EXPECT_FALSE(msg.empty()) << "what() 메시지가 비어 있으면 안 된다";
    });
}

// 빈 도구 이름 처리
TEST(ExceptionTest, ToolNotFound_EmptyToolName_Handled) {
    CToolNotFoundException ex("");
    EXPECT_EQ(ex.GetToolName(), std::string(""))
        << "빈 도구 이름도 보존되어야 한다";
}

// throw/catch 동작 확인
TEST(ExceptionTest, ToolNotFound_CanBeThrownAndCaught) {
    bool caught = false;
    try {
        throw CToolNotFoundException("GroupByAggregate");
    } catch (const CToolNotFoundException& e) {
        caught = true;
        EXPECT_EQ(e.GetToolName(), std::string("GroupByAggregate"));
    } catch (...) {
        FAIL() << "CToolNotFoundException이 아닌 다른 예외가 잡혔다";
    }
    EXPECT_TRUE(caught) << "CToolNotFoundException이 잡혀야 한다";
}

// std::exception으로 캐치 가능
TEST(ExceptionTest, ToolNotFound_CatchableAsStdException) {
    bool caught = false;
    try {
        throw CToolNotFoundException("TopN");
    } catch (const std::exception& e) {
        caught = true;
        std::string msg(e.what());
        EXPECT_FALSE(msg.empty());
    }
    EXPECT_TRUE(caught) << "std::exception으로 잡혀야 한다";
}

// ============================================================
// TC-08-04: CUnsupportedProviderException
// ============================================================

// 프로바이더 이름 보존 — GetProviderName()이 생성자 인자를 반환해야 함
TEST(ExceptionTest, UnsupportedProvider_PreservesProviderName) {
    CUnsupportedProviderException ex("UnknownLLM");
    EXPECT_EQ(ex.GetProviderName(), std::string("UnknownLLM"))
        << "GetProviderName()은 생성자에 전달된 프로바이더 이름을 반환해야 한다";
}

// what() 메시지에 프로바이더 이름 포함
TEST(ExceptionTest, UnsupportedProvider_WhatMessageContainsProviderName) {
    CUnsupportedProviderException ex("CustomLLM");
    std::string msg(ex.what());
    EXPECT_NE(msg.find("CustomLLM"), std::string::npos)
        << "what() 메시지에 프로바이더 이름 'CustomLLM'이 포함되어야 한다";
}

// std::exception 상속 확인
TEST(ExceptionTest, UnsupportedProvider_IsStdException) {
    CUnsupportedProviderException ex("SomeProvider");
    EXPECT_NO_THROW({
        const std::exception& base = ex;
        std::string msg(base.what());
        EXPECT_FALSE(msg.empty());
    });
}

// throw/catch 동작 확인
TEST(ExceptionTest, UnsupportedProvider_CanBeThrownAndCaught) {
    bool caught = false;
    try {
        throw CUnsupportedProviderException("Gemini");
    } catch (const CUnsupportedProviderException& e) {
        caught = true;
        EXPECT_EQ(e.GetProviderName(), std::string("Gemini"));
    } catch (...) {
        FAIL() << "CUnsupportedProviderException이 아닌 다른 예외가 잡혔다";
    }
    EXPECT_TRUE(caught);
}

// std::exception으로 캐치 가능
TEST(ExceptionTest, UnsupportedProvider_CatchableAsStdException) {
    bool caught = false;
    try {
        throw CUnsupportedProviderException("OllamaLocal");
    } catch (const std::exception& e) {
        caught = true;
        std::string msg(e.what());
        EXPECT_FALSE(msg.empty());
    }
    EXPECT_TRUE(caught) << "std::exception으로 잡혀야 한다";
}

// 빈 프로바이더 이름 처리
TEST(ExceptionTest, UnsupportedProvider_EmptyProviderName_Handled) {
    CUnsupportedProviderException ex("");
    EXPECT_EQ(ex.GetProviderName(), std::string(""))
        << "빈 프로바이더 이름도 보존되어야 한다";
}
