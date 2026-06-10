// tests/SelfTest.h - 자동 검증 모듈 (Command 패턴)
#pragma once

namespace deepmetria {

// Command 패턴 — 각 테스트는 ISelfTestCommand.
// RunSelfTestSuite 가 stdout 으로 결과 출력 후 종료코드 반환.
int RunSelfTestSuite();

} // namespace deepmetria
