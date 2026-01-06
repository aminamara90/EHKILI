// client.c - Client Project Only
#include "common.h"
#include "chat_window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    // Initialize common controls - CORRECTED VERSION
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Initialize Winsock for client
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBox(NULL, L"Winsock initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create chat window
    ChatWindow* cw = create_chat_window(hInstance);
    if (!cw) {
        MessageBox(NULL, L"Window creation failed", L"Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    WSACleanup();
    return (int)msg.wParam;
}