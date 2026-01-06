#ifndef CHAT_WINDOW_H
#define CHAT_WINDOW_H

#include "common.h"

typedef struct {
    HWND hwnd;
    HWND hEditChat;
    HWND hEditInput;
    HWND hBtnSend;
    HWND hBtnConnect;
    HWND hEditServer;
    HWND hEditPort;
    HWND hEditUsername;
    HWND hStatus;
    HWND hBtnToggleMode;    // Toggle online/offline mode
    HWND hModeStatus;       // Display current mode status
    SOCKET client_socket;
    HANDLE receive_thread;
    int connected;
    int ai_mode_online;     // 1 = API , 0 = offline
    int is_typing;          // Track if user is typing
    char username[MAX_USERNAME];
} ChatWindow;

// Function prototypes
ChatWindow* create_chat_window(HINSTANCE hInstance);
void append_chat_text(HWND hEdit, const char* text, int type);
DWORD WINAPI receive_messages(LPVOID lpParam);

#endif // CHAT_WINDOW_H