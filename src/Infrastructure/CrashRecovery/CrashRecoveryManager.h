#pragma once
#include "../../Common/Types.h"

namespace CrashRecovery {

struct AppLastState {
    CString lastFilePath;
    CString activeDashboardId;
    int     windowX, windowY, windowW, windowH;
    bool    dirty;

    AppLastState() : windowX(0), windowY(0), windowW(1280), windowH(800), dirty(false) {}
};

class CrashRecoveryManager {
public:
    static BOOL SaveLastState(const AppLastState& state);
    static BOOL LoadLastState(AppLastState& outState);
    static BOOL HandleCorruptState();
    static CString GetStateFilePath();
private:
    static CString GetAppDataDir();
};

} // namespace CrashRecovery
