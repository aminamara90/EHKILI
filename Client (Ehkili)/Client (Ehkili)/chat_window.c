#include "chat_window.h"

#define ID_EDIT_CHAT 101
#define ID_EDIT_INPUT 102
#define ID_BTN_SEND 103
#define ID_BTN_CONNECT 104
#define ID_EDIT_SERVER 105
#define ID_EDIT_PORT 106
#define ID_EDIT_USERNAME 107
#define ID_STATUS 108
#define ID_BTN_TOGGLE_MODE 109
#define ID_MODE_STATUS 110
#define ID_TIMER_TYPING 111

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void connect_to_server(ChatWindow* cw);
void send_chat_message(ChatWindow* cw);
void disconnect_from_server(ChatWindow* cw);
void toggle_ai_mode(ChatWindow* cw);
void resize_controls(ChatWindow* cw, int width, int height);
void update_send_button(ChatWindow* cw);

ChatWindow* create_chat_window(HINSTANCE hInstance) {
    ChatWindow* cw = (ChatWindow*)malloc(sizeof(ChatWindow));
    if (!cw) return NULL;

    memset(cw, 0, sizeof(ChatWindow));
    cw->client_socket = INVALID_SOCKET;
    cw->connected = 0;
    cw->ai_mode_online = 1;
    cw->is_typing = 0;

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"EhkiliChatWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassW(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            free(cw);
            return NULL;
        }
    }

    // Create RESIZABLE window
    cw->hwnd = CreateWindowExW(
        0,
        L"EhkiliChatWindow",
        L"Ehkili ChatBot - AI Desktop Messenger",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,  // Added WS_CLIPCHILDREN for smooth resize
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        NULL, NULL, hInstance, cw
    );

    if (!cw->hwnd) {
        free(cw);
        return NULL;
    }

    // Create fonts
    HFONT hFontNormal = CreateFontW(
        -15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    HFONT hFontBold = CreateFontW(
        -16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    HFONT hFontTitle = CreateFontW(
        -18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    // === HEADER SECTION ===
    HWND hTitle = CreateWindowW(L"STATIC", L"🤖 EHKILI CHATBOT",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 10, 860, 35, cw->hwnd, NULL, hInstance, NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

    cw->hStatus = CreateWindowExW(
        0, L"EDIT", L"● Disconnected - Configure settings below",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
        10, 50, 860, 25, cw->hwnd, (HMENU)ID_STATUS, hInstance, NULL);
    SendMessageW(cw->hStatus, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // === CONNECTION SECTION ===
    CreateWindowW(L"STATIC", L"Server:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        10, 85, 70, 25, cw->hwnd, NULL, hInstance, NULL);
    cw->hEditServer = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"127.0.0.1",
        WS_CHILD | WS_VISIBLE | ES_LEFT,
        90, 85, 140, 25, cw->hwnd, (HMENU)ID_EDIT_SERVER, hInstance, NULL);

    CreateWindowW(L"STATIC", L"Port:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        240, 85, 50, 25, cw->hwnd, NULL, hInstance, NULL);
    cw->hEditPort = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"8888",
        WS_CHILD | WS_VISIBLE | ES_LEFT,
        300, 85, 70, 25, cw->hwnd, (HMENU)ID_EDIT_PORT, hInstance, NULL);

    CreateWindowW(L"STATIC", L"Username:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        380, 85, 80, 25, cw->hwnd, NULL, hInstance, NULL);
    cw->hEditUsername = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"User",
        WS_CHILD | WS_VISIBLE | ES_LEFT,
        470, 85, 140, 25, cw->hwnd, (HMENU)ID_EDIT_USERNAME, hInstance, NULL);

    cw->hBtnConnect = CreateWindowW(L"BUTTON", L"🔌 Connect",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        620, 85, 120, 25, cw->hwnd, (HMENU)ID_BTN_CONNECT, hInstance, NULL);

    // === AI MODE TOGGLE ===
    cw->hModeStatus = CreateWindowW(L"STATIC", L"🌐 AI Mode: Online (API Enabled)",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 120, 350, 25, cw->hwnd, (HMENU)ID_MODE_STATUS, hInstance, NULL);
    SendMessageW(cw->hModeStatus, WM_SETFONT, (WPARAM)hFontBold, TRUE);

    cw->hBtnToggleMode = CreateWindowW(L"BUTTON", L"↔ Switch to Offline",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        370, 120, 180, 25, cw->hwnd, (HMENU)ID_BTN_TOGGLE_MODE, hInstance, NULL);

    HWND hModeInfo = CreateWindowW(L"STATIC",
        L"💡 Online: AI responses | Offline: Fast local",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        560, 120, 310, 25, cw->hwnd, NULL, hInstance, NULL);

    // === CHAT DISPLAY (will resize dynamically) ===
    cw->hEditChat = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
        WS_VSCROLL | ES_AUTOVSCROLL,
        10, 160, 860, 420,
        cw->hwnd, (HMENU)ID_EDIT_CHAT, hInstance, NULL);

    // === INPUT AREA (will resize dynamically) ===
    cw->hEditInput = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        10, 590, 750, 60,
        cw->hwnd, (HMENU)ID_EDIT_INPUT, hInstance, NULL);

    cw->hBtnSend = CreateWindowW(L"BUTTON", L"📤 Send",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        770, 590, 100, 60,
        cw->hwnd, (HMENU)ID_BTN_SEND, hInstance, NULL);
    EnableWindow(cw->hBtnSend, FALSE);

    // Apply fonts to all controls
    HWND controls[] = {
        cw->hEditChat, cw->hEditInput, cw->hEditServer, cw->hEditPort,
        cw->hEditUsername, cw->hBtnSend, cw->hBtnConnect, cw->hBtnToggleMode, hModeInfo
    };
    for (int i = 0; i < sizeof(controls) / sizeof(HWND); i++) {
        SendMessageW(controls[i], WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    }

    // Welcome message
    append_chat_text(cw->hEditChat, "|=========================================================================|", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "|                      WELCOME TO EHKILI CHATBOT                             |", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "|              Your Intelligent AI-Powered Desktop Messenger                 |", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "|=========================================================================|", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "Features:", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "   - Online Mode: Smart AI responses (Cohere/OpenAI)", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "   - Offline Mode: Instant local responses (no internet)", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "   - ilingual: English & Tunisian Arabic (3asslema!)", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "   - Dynamic: Window resizes smoothly", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "Ready to connect! Enter your details and click 'Connect'", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "", MSG_SYSTEM);

    ShowWindow(cw->hwnd, SW_SHOW);
    UpdateWindow(cw->hwnd);

    return cw;
}

void append_chat_text(HWND hEdit, const char* text, int type) {
    if (!hEdit || !text) return;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (wlen <= 0) return;

    wchar_t* wtext = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (!wtext) return;

    MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, wlen);

    int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)wtext);
    SendMessageW(hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
    SendMessageW(hEdit, EM_LINESCROLL, 0, 0xFFFF);

    free(wtext);
}

DWORD WINAPI receive_messages(LPVOID lpParam) {
    ChatWindow* cw = (ChatWindow*)lpParam;
    ChatMessage msg;
    int bytes_received;
    char display[BUFFER_SIZE * 2];

    while (cw->connected) {
        bytes_received = recv(cw->client_socket, (char*)&msg, sizeof(ChatMessage), 0);

        if (bytes_received <= 0) {
            cw->connected = 0;
            EnableWindow(cw->hBtnSend, FALSE);
            EnableWindow(cw->hBtnConnect, TRUE);
            SetWindowTextW(cw->hStatus, L"Disconnected - Connection lost");
            InvalidateRect(cw->hStatus, NULL, TRUE);
            UpdateWindow(cw->hStatus);
            SetWindowTextW(cw->hBtnConnect, L"🔌 Connect");

            append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
            append_chat_text(cw->hEditChat, "=========================================================================", MSG_SYSTEM);
            append_chat_text(cw->hEditChat, "  !! CONNECTION LOST - Server disconnected !!", MSG_SYSTEM);
            append_chat_text(cw->hEditChat, "=========================================================================", MSG_SYSTEM);
            break;
        }

        // Format messages
        switch (msg.type) {
        case MSG_SYSTEM:
            sprintf(display, "%s", msg.message);
            break;
        case MSG_JOIN:
            sprintf(display, "%s joined the chat", msg.username);
            break;
        case MSG_LEAVE:
            sprintf(display, "%s left the chat", msg.username);
            break;
        case MSG_PRIVATE:
            if (strcmp(msg.target, cw->username) == 0) {
                sprintf(display, "[PRIVATE from %s]: %s", msg.username, msg.message);
            }
            break;
        default:
            if (strcmp(msg.username, "Ehkili") == 0) {
                append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
                append_chat_text(cw->hEditChat, "=========================================================================", MSG_SYSTEM);
                sprintf(display, "Ehkili:");
                append_chat_text(cw->hEditChat, display, msg.type);

                // Word wrap for long messages
                char* line = msg.message;
                char wrapped[MAX_MESSAGE + 20];
                while (strlen(line) > 65) {
                    int break_pos = 65;
                    while (break_pos > 0 && line[break_pos] != ' ') break_pos--;
                    if (break_pos == 0) break_pos = 65;

                    snprintf(wrapped, sizeof(wrapped), " %.*s", break_pos, line);
                    append_chat_text(cw->hEditChat, wrapped, msg.type);
                    line += break_pos;
                    while (*line == ' ') line++;
                }
                sprintf(wrapped, "%s", line);
                append_chat_text(cw->hEditChat, wrapped, msg.type);

                append_chat_text(cw->hEditChat, "=========================================================================", MSG_SYSTEM);
                append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
                continue;
            }
            else {
                sprintf(display, "%s: %s", msg.username, msg.message);
            }
            break;
        }

        append_chat_text(cw->hEditChat, display, msg.type);
    }

    return 0;
}

void resize_controls(ChatWindow* cw, int width, int height) {
    if (width < 400 || height < 300) return; // Minimum size

    // Calculate margins
    int margin = 10;
    int usable_width = width - (margin * 2) - 20; // Account for window borders
    int usable_height = height - margin - 40; // Account for title bar

    // Fixed heights
    int title_height = 35;
    int status_height = 25;
    int connection_height = 25;
    int mode_height = 25;
    int input_height = 60;
    int button_width = 100;

    int y_pos = margin;

    // Title (spans full width)
    SetWindowPos(GetWindow(cw->hwnd, GW_CHILD), NULL,
        margin, y_pos, usable_width, title_height,
        SWP_NOZORDER);
    y_pos += title_height + 5;

    // Status
    SetWindowPos(cw->hStatus, NULL,
        margin, y_pos, usable_width, status_height,
        SWP_NOZORDER);
    y_pos += status_height + 10;

    // Connection controls (dynamically positioned)
    SetWindowPos(cw->hEditServer, NULL,
        90, y_pos, 140, connection_height,
        SWP_NOZORDER);
    SetWindowPos(cw->hEditPort, NULL,
        300, y_pos, 70, connection_height,
        SWP_NOZORDER);
    SetWindowPos(cw->hEditUsername, NULL,
        470, y_pos, usable_width - 600, connection_height,
        SWP_NOZORDER);
    SetWindowPos(cw->hBtnConnect, NULL,
        usable_width - 110, y_pos, 120, connection_height,
        SWP_NOZORDER);
    y_pos += connection_height + 10;

    // Mode controls
    SetWindowPos(cw->hModeStatus, NULL,
        margin, y_pos, 350, mode_height,
        SWP_NOZORDER);
    SetWindowPos(cw->hBtnToggleMode, NULL,
        370, y_pos, 180, mode_height,
        SWP_NOZORDER);
    y_pos += mode_height + 10;

    // Calculate remaining space for chat and input
    int remaining_height = usable_height - y_pos - input_height - 15;

    // Chat area (takes most of remaining space)
    SetWindowPos(cw->hEditChat, NULL,
        margin, y_pos, usable_width, remaining_height,
        SWP_NOZORDER);
    y_pos += remaining_height + 10;

    // Input area and send button
    SetWindowPos(cw->hEditInput, NULL,
        margin, y_pos, usable_width - button_width - 10, input_height,
        SWP_NOZORDER);
    SetWindowPos(cw->hBtnSend, NULL,
        usable_width - button_width + margin, y_pos, button_width, input_height,
        SWP_NOZORDER);

    // Redraw all controls
    InvalidateRect(cw->hwnd, NULL, TRUE);
}

void update_send_button(ChatWindow* cw) {
    if (!cw->connected) {
        EnableWindow(cw->hBtnSend, FALSE);
        SetWindowTextW(cw->hBtnSend, L"📤 Send");
        return;
    }

    char text[10];
    GetWindowTextA(cw->hEditInput, text, sizeof(text));

    // Check if there's actual text (not just whitespace)
    int has_text = 0;
    for (char* p = text; *p; p++) {
        if (!isspace(*p)) {
            has_text = 1;
            break;
        }
    }

    if (has_text) {
        EnableWindow(cw->hBtnSend, TRUE);
        SetWindowTextW(cw->hBtnSend, L"📤 Send");
    }
    else {
        EnableWindow(cw->hBtnSend, FALSE);
        SetWindowTextW(cw->hBtnSend, L"📤 Send");
    }
}

void toggle_ai_mode(ChatWindow* cw) {
    cw->ai_mode_online = !cw->ai_mode_online;

    if (cw->ai_mode_online) {
        SetWindowTextW(cw->hModeStatus, L"AI Mode: Online (API Enabled)");
        SetWindowTextW(cw->hBtnToggleMode, L"↔ Switch to Offline");
        append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
        append_chat_text(cw->hEditChat, "--> Switched to ONLINE mode - Using AI API (Cohere/OpenAI)", MSG_SYSTEM);
        append_chat_text(cw->hEditChat, "   Next response will use intelligent AI!", MSG_SYSTEM);
    }
    else {
        SetWindowTextW(cw->hModeStatus, L"AI Mode: Offline (Local Only)");
        SetWindowTextW(cw->hBtnToggleMode, L"↔ Switch to Online");
        append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
        append_chat_text(cw->hEditChat, "--> Switched to OFFLINE mode - Using local responses", MSG_SYSTEM);
        append_chat_text(cw->hEditChat, "   Responses will be instant (no internet needed)!", MSG_SYSTEM);
    }
}

void connect_to_server(ChatWindow* cw) {
    if (cw->connected) {
        disconnect_from_server(cw);
        return;
    }

    WSADATA wsa;
    struct sockaddr_in server;
    char server_ip[100];
    char port_str[20];
    char username[MAX_USERNAME];
    int port;

    GetWindowTextA(cw->hEditServer, server_ip, sizeof(server_ip));
    GetWindowTextA(cw->hEditPort, port_str, sizeof(port_str));
    GetWindowTextA(cw->hEditUsername, username, sizeof(username));

    if (strlen(username) == 0) {
        MessageBoxA(cw->hwnd, "Please enter a username!", "Username Required",
            MB_OK | MB_ICONWARNING);
        SetFocus(cw->hEditUsername);
        return;
    }

    port = atoi(port_str);
    strcpy(cw->username, username);

    SetWindowTextW(cw->hStatus, L"Connecting...");
    InvalidateRect(cw->hStatus, NULL, TRUE);
    UpdateWindow(cw->hStatus);

    cw->client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cw->client_socket == INVALID_SOCKET) {
        MessageBoxA(cw->hwnd, "Failed to create socket!", "Error", MB_OK | MB_ICONERROR);
        SetWindowTextW(cw->hStatus, L"Connection failed");
        InvalidateRect(cw->hStatus, NULL, TRUE);
        UpdateWindow(cw->hStatus);
        return;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_port = htons(port);

    if (connect(cw->client_socket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        char error_msg[200];
        sprintf(error_msg, "Could not connect to %s:%d\n\nMake sure the server is running!",
            server_ip, port);
        MessageBoxA(cw->hwnd, error_msg, "Connection Failed", MB_OK | MB_ICONERROR);
        closesocket(cw->client_socket);
        SetWindowTextW(cw->hStatus, L"Connection failed");
        InvalidateRect(cw->hStatus, NULL, TRUE);
        UpdateWindow(cw->hStatus);
        return;
    }

    ChatMessage join_msg;
    join_msg.type = MSG_JOIN;
    strcpy(join_msg.username, username);
    strcpy(join_msg.message, "joined");
    join_msg.target[0] = '\0';
    send(cw->client_socket, (char*)&join_msg, sizeof(ChatMessage), 0);

    cw->connected = 1;
    EnableWindow(cw->hBtnSend, FALSE);
    EnableWindow(cw->hEditServer, FALSE);
    EnableWindow(cw->hEditPort, FALSE);
    EnableWindow(cw->hEditUsername, FALSE);
    SetWindowTextW(cw->hStatus, L"Connected - Chat away!");
    InvalidateRect(cw->hStatus, NULL, TRUE);
    UpdateWindow(cw->hStatus);
    SetWindowTextW(cw->hBtnConnect, L"🔌 Disconnect");
    SetFocus(cw->hEditInput);

    cw->receive_thread = CreateThread(NULL, 0, receive_messages, cw, 0, NULL);

    append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "=========================================================================", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "        SUCCESSFULLY CONNECTED TO SERVER", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "    ChatBot is ready! Start your conversation!", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "=========================================================================", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
}

void disconnect_from_server(ChatWindow* cw) {
    if (!cw->connected) return;

    ChatMessage leave_msg;
    leave_msg.type = MSG_LEAVE;
    strcpy(leave_msg.username, cw->username);
    strcpy(leave_msg.message, "left");
    send(cw->client_socket, (char*)&leave_msg, sizeof(ChatMessage), 0);

    cw->connected = 0;
    closesocket(cw->client_socket);

    EnableWindow(cw->hBtnSend, FALSE);
    EnableWindow(cw->hEditServer, TRUE);
    EnableWindow(cw->hEditPort, TRUE);
    EnableWindow(cw->hEditUsername, TRUE);
    SetWindowTextW(cw->hStatus, L"Disconnected");
    InvalidateRect(cw->hStatus, NULL, TRUE);
    UpdateWindow(cw->hStatus);
    SetWindowTextW(cw->hBtnConnect, L"🔌 Connect");

    append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "=========================================================================", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "            DISCONNECTED FROM SERVER", MSG_SYSTEM);
    append_chat_text(cw->hEditChat, "=========================================================================", MSG_SYSTEM);
}

void send_chat_message(ChatWindow* cw) {
    char message[MAX_MESSAGE];
    GetWindowTextA(cw->hEditInput, message, sizeof(message));

    char* start = message;
    while (*start && isspace(*start)) start++;
    if (strlen(start) == 0) return;

    ChatMessage msg;
    msg.type = MSG_NORMAL;
    strcpy(msg.username, cw->username);
    strncpy(msg.message, start, MAX_MESSAGE - 1);
    msg.message[MAX_MESSAGE - 1] = '\0';
    msg.target[0] = '\0';

    if (send(cw->client_socket, (char*)&msg, sizeof(ChatMessage), 0) == SOCKET_ERROR) {
        MessageBoxA(cw->hwnd, "Failed to send message!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    SetWindowTextA(cw->hEditInput, "");
    SetFocus(cw->hEditInput);
    update_send_button(cw);

    char display[BUFFER_SIZE];
    append_chat_text(cw->hEditChat, "", MSG_SYSTEM);
    sprintf(display, "You: %s", msg.message);
    append_chat_text(cw->hEditChat, display, MSG_NORMAL);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static ChatWindow* cw = NULL;

    switch (uMsg) {
    case WM_CREATE:
        cw = (ChatWindow*)((CREATESTRUCT*)lParam)->lpCreateParams;
        break;

    case WM_SIZE: {
        if (cw) {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            resize_controls(cw, width, height);
        }
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_CONNECT) {
            connect_to_server(cw);
        }
        else if (LOWORD(wParam) == ID_BTN_SEND) {
            send_chat_message(cw);
        }
        else if (LOWORD(wParam) == ID_BTN_TOGGLE_MODE) {
            toggle_ai_mode(cw);
        }
        else if (LOWORD(wParam) == ID_EDIT_INPUT && HIWORD(wParam) == EN_CHANGE) {
            update_send_button(cw);
        }
        break;

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 700; // Minimum width
        mmi->ptMinTrackSize.y = 500; // Minimum height
        break;
    }

    case WM_CLOSE:
        if (cw) {
            if (cw->connected) {
                int result = MessageBoxA(hwnd,
                    "You are still connected to the server.\n\nDo you want to disconnect and exit?",
                    "Confirm Exit",
                    MB_YESNO | MB_ICONQUESTION);
                if (result == IDNO) {
                    return 0; // Don't close
                }
                disconnect_from_server(cw);
                Sleep(100); // Give time for clean disconnect
            }
            free(cw);
        }
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_CTLCOLORSTATIC: {
        // Custom colors for status labels
        HDC hdcStatic = (HDC)wParam;
        HWND hStatic = (HWND)lParam;

        if (hStatic == cw->hStatus) {
            SetBkMode(hdcStatic, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        break;
    }

    case WM_ERASEBKGND: {
        // Prevent flicker during resize
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Custom background
        RECT rect;
        GetClientRect(hwnd, &rect);
        HBRUSH hBrush = CreateSolidBrush(RGB(240, 242, 245));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_KEYDOWN: {
        // Handle keyboard shortcuts
        if (wParam == VK_RETURN) {
            // Check if input has focus
            HWND hFocused = GetFocus();
            if (hFocused == cw->hEditInput) {
                // Check if Shift is held (for multiline)
                if (!(GetKeyState(VK_SHIFT) & 0x8000)) {
                    // Send message on Enter (without Shift)
                    if (cw->connected) {
                        send_chat_message(cw);
                    }
                    return 0;
                }
            }
        }
        else if (wParam == VK_ESCAPE) {
            // Clear input on Escape
            if (GetFocus() == cw->hEditInput) {
                SetWindowTextA(cw->hEditInput, "");
                update_send_button(cw);
            }
        }
        break;
    }

    case WM_ACTIVATE: {
        // Refresh UI when window becomes active
        if (LOWORD(wParam) != WA_INACTIVE) {
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}