#include "WinMessagesIdToTextMap.h"
#include <tchar.h>

#include "detours.h"

#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"

static LONG dwMessagesCount = 0;
static BOOL(WINAPI* TrueGetMessage)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) = GetMessage;

BOOL WINAPI GetMessageHook(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
    std::string translatedMessage = wmIntToStringMap[static_cast<int>(lpMsg->message)];
    if (!translatedMessage.empty()) PLOGD << "Got message: " << translatedMessage << ", wParam: " << lpMsg->wParam << ", lParam: " << lpMsg->lParam;
    else PLOGD << "Got unknown message. Message code: " << lpMsg->message << ", wParam: " << lpMsg->wParam << ", lParam: " << lpMsg->lParam;
    ++dwMessagesCount;

    return TrueGetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH) {
        plog::init(plog::debug, "C:\\IDPHooks\\log.txt");

        DetourRestoreAfterWith();

        PLOGD << "Hooks are being attached...";

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueGetMessage, GetMessageHook);
        error = DetourTransactionCommit();

        if (error == NO_ERROR) {
            PLOGD << "Hooks attached!";
        }
        else {
            PLOGD << "Error! : " << error;
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueGetMessage, GetMessageHook);
        error = DetourTransactionCommit();

        PLOGD << "Hooks detached! Encountered messages: " << dwMessagesCount;
    }
    return TRUE;
}