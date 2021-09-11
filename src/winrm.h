#ifndef _WINRM_H_
#define _WINRM_H_

#include <windows.h>
#include <iostream>

#define WSMAN_API_VERSION_1_0
#include <wsman.h>


class WinRM
{
public:
    WinRM();
    ~WinRM();

    BOOL Setup(std::wstring connection, std::wstring username, std::wstring password);
    BOOL Execute(std::wstring commandLine);

private:

    WSMAN_API_HANDLE hAPI;
    WSMAN_SESSION_HANDLE hSession;
    WSMAN_SHELL_HANDLE hShell;
    WSMAN_COMMAND_HANDLE hCommand;

    DWORD dwError;
    DWORD dwReceieveError;
    WSMAN_SHELL_ASYNC async;
    WSMAN_SHELL_ASYNC receiveAsync;
    HANDLE hEvent;
    HANDLE hReceiveEvent;
    BOOL bSetup;
    BOOL bExecute;
    std::wstring resourceUri = L"http://schemas.microsoft.com/wbem/wsman/1/windows/shell/cmd";

    static void CALLBACK WSManShellCompletionFunction
    (
        __in_opt PVOID operationContext,
        DWORD flags,
        __in WSMAN_ERROR* error,
        __in WSMAN_SHELL_HANDLE shell,
        __in_opt WSMAN_COMMAND_HANDLE command,
        __in_opt WSMAN_OPERATION_HANDLE operationHandle,
        __in_opt WSMAN_RECEIVE_DATA_RESULT* data
    );

    void CALLBACK m_WSManShellCompletionFunction
    (
        DWORD flags,
        __in WSMAN_ERROR* error,
        __in WSMAN_SHELL_HANDLE shell,
        __in_opt WSMAN_COMMAND_HANDLE command,
        __in_opt WSMAN_OPERATION_HANDLE operationHandle,
        __in_opt WSMAN_RECEIVE_DATA_RESULT* data
    );

    static void CALLBACK ReceiveCallback
    (
        __in_opt PVOID operationContext,
        DWORD flags,
        __in WSMAN_ERROR* error,
        __in WSMAN_SHELL_HANDLE shell,
        __in_opt WSMAN_COMMAND_HANDLE command,
        __in_opt WSMAN_OPERATION_HANDLE operationHandle,
        __in_opt WSMAN_RECEIVE_DATA_RESULT* data
    );
    void CALLBACK m_ReceiveCallback
    (
        DWORD flags,
        __in WSMAN_ERROR* error,
        __in WSMAN_SHELL_HANDLE shell,
        __in_opt WSMAN_COMMAND_HANDLE command,
        __in_opt WSMAN_OPERATION_HANDLE operationHandle,
        __in_opt WSMAN_RECEIVE_DATA_RESULT* data
    );

    VOID Cleanup();
};

typedef HANDLE (WINAPI* _CreateEventA)
(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL                  bManualReset,
    BOOL                  bInitialState,
    LPCSTR                lpName
);

typedef DWORD (WINAPI* _WaitForSingleObject)
(
    HANDLE hHandle,
    DWORD  dwMilliseconds
);

typedef BOOL (WINAPI* _CloseHandle)
(
    HANDLE hObject
);

typedef BOOL (WINAPI* _FreeLibrary)
(
    HMODULE hLibModule
);

typedef BOOL (WINAPI* _SetEvent)
(
    HANDLE hEvent
);

typedef HANDLE (WINAPI* _GetStdHandle)
(
    _In_ DWORD nStdHandle
);

typedef BOOL (WINAPI* _WriteFile)
(
    HANDLE       hFile,
    LPCVOID      lpBuffer,
    DWORD        nNumberOfBytesToWrite,
    LPDWORD      lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
);

typedef DWORD (WINAPI* _WSManInitialize)
(
    DWORD            flags,
    WSMAN_API_HANDLE* apiHandle
);

typedef DWORD (WINAPI* _WSManCreateSession)
(
    WSMAN_API_HANDLE                 apiHandle,
    PCWSTR                           connection,
    DWORD                            flags,
    WSMAN_AUTHENTICATION_CREDENTIALS* serverAuthenticationCredentials,
    WSMAN_PROXY_INFO* proxyInfo,
    WSMAN_SESSION_HANDLE* session
);

typedef DWORD (WINAPI* _WSManSetSessionOption)
(
    WSMAN_SESSION_HANDLE session,
    WSManSessionOption   option,
    WSMAN_DATA* data
);

typedef void (WINAPI* _WSManCreateShell)
(
    WSMAN_SESSION_HANDLE     session,
    DWORD                    flags,
    PCWSTR                   resourceUri,
    WSMAN_SHELL_STARTUP_INFO* startupInfo,
    WSMAN_OPTION_SET* options,
    WSMAN_DATA* createXml,
    WSMAN_SHELL_ASYNC* async,
    WSMAN_SHELL_HANDLE* shell
);

typedef void (WINAPI* _WSManRunShellCommand)
(
    WSMAN_SHELL_HANDLE    shell,
    DWORD                 flags,
    PCWSTR                commandLine,
    WSMAN_COMMAND_ARG_SET* args,
    WSMAN_OPTION_SET* options,
    WSMAN_SHELL_ASYNC* async,
    WSMAN_COMMAND_HANDLE* command
);

typedef void (WINAPI* _WSManReceiveShellOutput)
(
    WSMAN_SHELL_HANDLE     shell,
    WSMAN_COMMAND_HANDLE   command,
    DWORD                  flags,
    WSMAN_STREAM_ID_SET* desiredStreamSet,
    WSMAN_SHELL_ASYNC* async,
    WSMAN_OPERATION_HANDLE* receiveOperation
);

typedef DWORD (WINAPI* _WSManCloseOperation)
(
    WSMAN_OPERATION_HANDLE operationHandle,
    DWORD                  flags
);

typedef void (WINAPI* _WSManCloseCommand)
(
    WSMAN_COMMAND_HANDLE commandHandle,
    DWORD                flags,
    WSMAN_SHELL_ASYNC* async
);

typedef void (WINAPI* _WSManCloseShell)
(
    WSMAN_SHELL_HANDLE shellHandle,
    DWORD              flags,
    WSMAN_SHELL_ASYNC* async
);

typedef DWORD (WINAPI* _WSManCloseSession)
(
    WSMAN_SESSION_HANDLE session,
    DWORD                flags
);

typedef DWORD (WINAPI* _WSManDeinitialize)
(
    WSMAN_API_HANDLE apiHandle,
    DWORD            flags
);

#endif