#include "winrm.h"
#include "peb.h"

WinRM::WinRM()
    : hAPI(NULL),
    hSession(NULL),
    hShell(NULL),
    hCommand(NULL),
    dwError(NO_ERROR),
    dwReceieveError(NO_ERROR),
    hEvent(NULL),
    hReceiveEvent(NULL),
    bSetup(FALSE),
    bExecute(FALSE)
{
    memset(&async, 0, sizeof(async));
}

WinRM::~WinRM()
{
    Cleanup();
}

BOOL WinRM::Setup(std::wstring host, std::wstring username, std::wstring password)
{
    if (!LoadLibraryA("WsmSvc.dll"))
    {
        return FALSE;
    }

    if (bSetup) return TRUE;

    _WSManInitialize pWSManInitialize = reinterpret_cast<_WSManInitialize>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManInitialize"));

    dwError = pWSManInitialize(0, &hAPI);
    if (NO_ERROR != dwError)
    {
        wprintf(L"WSManInitialize failed: %d\n", dwError);
        return FALSE;
    }

    WSMAN_AUTHENTICATION_CREDENTIALS authCreds;

    authCreds.authenticationMechanism = WSMAN_FLAG_AUTH_NEGOTIATE;
    if (username.size() == 0 && password.size() == 0)
    {
        authCreds.userAccount.username = NULL;
        authCreds.userAccount.password = NULL;
    }
    else
    {
        authCreds.userAccount.username = username.c_str();
        authCreds.userAccount.password = password.c_str();
    }

    _WSManCreateSession pWSManCreateSession = reinterpret_cast<_WSManCreateSession>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManCreateSession"));
    dwError = pWSManCreateSession(hAPI, host.c_str(), 0, &authCreds, NULL, &hSession);

    if (dwError != 0)
    {
        wprintf(L"WSManCreateSession failed: %d\n", dwError);
        return FALSE;
    }

    WSManSessionOption option = WSMAN_OPTION_DEFAULT_OPERATION_TIMEOUTMS;
    WSMAN_DATA data;
    data.type = WSMAN_DATA_TYPE_DWORD;
    data.number = 60000;

    _WSManSetSessionOption pWSManSetSessionOption = reinterpret_cast<_WSManSetSessionOption>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManSetSessionOption"));
    dwError = pWSManSetSessionOption(hSession, option, &data);
    if (dwError != 0)
    {
        wprintf(L"WSManSetSessionOption failed: %d\n", dwError);
        return FALSE;
    }

    _CreateEventA pCreateEventA = reinterpret_cast<_CreateEventA>(zzGetProcAddress(zzGetModuleHandle(L"kernel32.dll"), "CreateEventA"));
    hEvent = pCreateEventA(0, FALSE, FALSE, NULL);
    if (NULL == hEvent)
    {
        dwError = GetLastError();
        wprintf(L"CreateEvent failed: %d\n", dwError);
        return FALSE;
    }
    async.operationContext = this;
    async.completionFunction = &WSManShellCompletionFunction;

    hReceiveEvent = pCreateEventA(0, FALSE, FALSE, NULL);
    if (NULL == hReceiveEvent)
    {
        dwError = GetLastError();
        wprintf(L"CreateEvent failed: %d\n", dwError);
        return FALSE;
    }
    receiveAsync.operationContext = this;
    receiveAsync.completionFunction = &ReceiveCallback;

    bSetup = TRUE;

    return TRUE;
}

BOOL WinRM::Execute(std::wstring commandLine)
{
    _WSManCreateShell pWSManCreateShell = reinterpret_cast<_WSManCreateShell>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManCreateShell"));
    pWSManCreateShell(hSession, 0, resourceUri.c_str(), NULL, NULL, NULL, &async, &hShell);

    _WaitForSingleObject pWaitForSingleObject = reinterpret_cast<_WaitForSingleObject>(zzGetProcAddress(zzGetModuleHandle(L"kernel32.dll"), "WaitForSingleObject"));
    pWaitForSingleObject(hEvent, INFINITE);
    if (dwError != 0)
    {
        wprintf(L"WSManCreateShell failed: %d\n", dwError);
        return FALSE;
    }

    _WSManRunShellCommand pWSManRunShellCommand = reinterpret_cast<_WSManRunShellCommand>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManRunShellCommand"));
    pWSManRunShellCommand(hShell, 0, commandLine.c_str(), NULL, NULL, &async, &hCommand);

    pWaitForSingleObject(hEvent, INFINITE);

    if (dwError != 0)
    {
        wprintf(L"WSManRunShellCommand failed: %d\n", dwError);
        return FALSE;
    }

    WSMAN_OPERATION_HANDLE receiveOp = NULL;

    _WSManReceiveShellOutput pWSManReceiveShellOutput = reinterpret_cast<_WSManReceiveShellOutput>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManReceiveShellOutput"));
    pWSManReceiveShellOutput(hShell, hCommand, 0, NULL, &receiveAsync, &receiveOp);

    pWaitForSingleObject(hReceiveEvent, INFINITE);
    if (dwError != 0)
    {
        wprintf(L"WSManReceiveShellOutput failed: %d\n", dwReceieveError);
        return FALSE;
    }

    _WSManCloseOperation pWSManCloseOperation = reinterpret_cast<_WSManCloseOperation>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManCloseOperation"));
    dwError = pWSManCloseOperation(receiveOp, 0);

    if (dwError != 0)
    {
        wprintf(L"WSManCloseOperation failed: %d\n", dwError);
        return FALSE;
    }

    return TRUE;
}

VOID WinRM::Cleanup()
{
    _WaitForSingleObject pWaitForSingleObject = reinterpret_cast<_WaitForSingleObject>(zzGetProcAddress(zzGetModuleHandle(L"kernel32.dll"), "WaitForSingleObject"));

    if (NULL != hCommand)
    {
        _WSManCloseCommand pWSManCloseCommand = reinterpret_cast<_WSManCloseCommand>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManCloseCommand"));
        pWSManCloseCommand(hCommand, 0, &async);

        pWaitForSingleObject(hEvent, INFINITE);
        if (dwError != 0)
        {
            wprintf(L"WSManCloseCommand failed: %d\n", dwError);
        }
        else
        {
            hCommand = NULL;
        }
    }

    if (NULL != hShell)
    {
        _WSManCloseShell pWSManCloseShell = reinterpret_cast<_WSManCloseShell>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManCloseShell"));
        pWSManCloseShell(hShell, 0, &async);
        pWaitForSingleObject(hEvent, INFINITE);
        if (NO_ERROR != dwError)
        {
            wprintf(L"WSManCloseShell failed: %d\n", dwError);
        }
        else
        {
            hShell = NULL;
        }
    }

    _WSManCloseSession pWSManCloseSession = reinterpret_cast<_WSManCloseSession>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManCloseSession"));
    dwError = pWSManCloseSession(hSession, 0);
    if (dwError != 0)
    {
        wprintf(L"WSManCloseSession failed: %d\n", dwError);
    }

    _WSManDeinitialize pWSManDeinitialize = reinterpret_cast<_WSManDeinitialize>(zzGetProcAddress(zzGetModuleHandle(L"WsmSvc.dll"), "WSManDeinitialize"));
    dwError = pWSManDeinitialize(hAPI, 0);
    if (dwError != 0)
    {
        wprintf(L"WSManDeinitialize failed: %d\n", dwError);
    }

    _CloseHandle pCloseHandle = reinterpret_cast<_CloseHandle>(zzGetProcAddress(zzGetModuleHandle(L"kernel32.dll"), "CloseHandle"));

    if (NULL != hEvent)
    {
        pCloseHandle(hEvent);
        hEvent = NULL;
    }
    if (NULL != hReceiveEvent)
    {
        pCloseHandle(hReceiveEvent);
        hReceiveEvent = NULL;
    }

    bSetup = FALSE;
    bExecute = FALSE;

    HMODULE hModule = zzGetModuleHandle(L"WsmSvc.dll");
    if (hModule)
    {
        reinterpret_cast<_FreeLibrary>(zzGetProcAddress(zzGetModuleHandle(L"kernel32.dll"), "FreeLibrary"))(hModule);
    }
}

void CALLBACK WinRM::WSManShellCompletionFunction
(
    __in_opt PVOID operationContext,
    DWORD flags,
    __in WSMAN_ERROR* error,
    __in WSMAN_SHELL_HANDLE shell,
    __in_opt WSMAN_COMMAND_HANDLE command,
    __in_opt WSMAN_OPERATION_HANDLE operationHandle,
    __in_opt WSMAN_RECEIVE_DATA_RESULT* data
)
{
    if (operationContext)
    {
        WinRM* context = reinterpret_cast<WinRM*>(operationContext);
        context->m_WSManShellCompletionFunction(flags, error, shell, command, operationHandle, data);
    }
}

void CALLBACK WinRM::m_WSManShellCompletionFunction
(
    DWORD flags,
    __in WSMAN_ERROR* error,
    __in WSMAN_SHELL_HANDLE shell,
    __in_opt WSMAN_COMMAND_HANDLE command,
    __in_opt WSMAN_OPERATION_HANDLE operationHandle,
    __in_opt WSMAN_RECEIVE_DATA_RESULT* data
)
{
    if (error && 0 != error->code)
    {
        dwError = error->code;
        wprintf(error->errorDetail);
    }
    reinterpret_cast<_SetEvent>(zzGetProcAddress(zzGetModuleHandle(L"kernel32.dll"), "SetEvent"))(hEvent);
}

void CALLBACK WinRM::ReceiveCallback
(
    __in_opt PVOID operationContext,
    DWORD flags,
    __in WSMAN_ERROR* error,
    __in WSMAN_SHELL_HANDLE shell,
    __in_opt WSMAN_COMMAND_HANDLE command,
    __in_opt WSMAN_OPERATION_HANDLE operationHandle,
    __in_opt WSMAN_RECEIVE_DATA_RESULT* data
)
{
    if (operationContext)
    {
        WinRM* context = reinterpret_cast<WinRM*>(operationContext);
        context->m_ReceiveCallback(flags, error, shell, command, operationHandle, data);
    }
}
void CALLBACK WinRM::m_ReceiveCallback
(
    DWORD flags,
    __in WSMAN_ERROR* error,
    __in WSMAN_SHELL_HANDLE shell,
    __in_opt WSMAN_COMMAND_HANDLE command,
    __in_opt WSMAN_OPERATION_HANDLE operationHandle,
    __in_opt WSMAN_RECEIVE_DATA_RESULT* data
)
{
    if (error && 0 != error->code)
    {
        dwReceieveError = error->code;
        wprintf(error->errorDetail);
    }

    if (data && data->streamData.type == WSMAN_DATA_TYPE_BINARY && data->streamData.binaryData.dataLength)
    {
        _GetStdHandle pGetStdHandle = reinterpret_cast<_GetStdHandle>(zzGetProcAddress(zzGetModuleHandle(L"kernel32.dll"), "GetStdHandle"));
        HANDLE hFile = ((0 == _wcsicmp(data->streamId, WSMAN_STREAM_ID_STDERR)) ? pGetStdHandle(STD_ERROR_HANDLE) : pGetStdHandle(STD_OUTPUT_HANDLE));

        DWORD t_BufferWriteLength = 0;

        _WriteFile pWriteFile = reinterpret_cast<_WriteFile>(zzGetProcAddress(zzGetModuleHandle(L"kernel32.dll"), "WriteFile"));
        pWriteFile(hFile, data->streamData.binaryData.data, data->streamData.binaryData.dataLength, &t_BufferWriteLength, NULL);
    }

    if ((error && 0 != error->code) || (data && data->commandState && wcscmp(data->commandState, WSMAN_COMMAND_STATE_DONE) == 0))
    {
        reinterpret_cast<_SetEvent>(zzGetProcAddress(zzGetModuleHandle(L"kernel32.dll"), "SetEvent"))(hReceiveEvent);
    }
}