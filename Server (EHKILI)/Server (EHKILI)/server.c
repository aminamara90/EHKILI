#define _SERVER
#include "common.h"
#include <time.h>
#include <ctype.h>

SOCKET client_sockets[MAX_CLIENTS];
char client_usernames[MAX_CLIENTS][MAX_USERNAME];
int client_count = 0;
CRITICAL_SECTION cs;

void broadcast_message(ChatMessage* msg, SOCKET exclude_socket) {
    EnterCriticalSection(&cs);

    for (int i = 0; i < client_count; i++) {
        if (client_sockets[i] != exclude_socket && client_sockets[i] != INVALID_SOCKET) {
            send(client_sockets[i], (char*)msg, sizeof(ChatMessage), 0);
        }
    }

    LeaveCriticalSection(&cs);
}

DWORD WINAPI client_handler(LPVOID lpParam) {
    SOCKET client_socket = *(SOCKET*)lpParam;
    ChatMessage msg;
    int bytes_received;

    // Receive username
    bytes_received = recv(client_socket, (char*)&msg, sizeof(ChatMessage), 0);
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return 0;
    }

    // Add client to list
    EnterCriticalSection(&cs);
    strcpy(client_usernames[client_count], msg.username);
    client_sockets[client_count] = client_socket;
    int client_index = client_count;
    client_count++;
    LeaveCriticalSection(&cs);

    // Broadcast join message
    msg.type = MSG_SYSTEM;
    sprintf(msg.message, "%s has joined the chat", msg.username);
    broadcast_message(&msg, client_socket);

    printf("[Server] %s connected\n", msg.username);

    // Send welcome message from chatbot
    ChatMessage bot_msg;
    bot_msg.type = MSG_NORMAL;
    strcpy(bot_msg.username, "ChatBot");

    char welcome_msg[MAX_MESSAGE];
    sprintf(welcome_msg, "Welcome %s! I'm your AI assistant. Ask me anything!", msg.username);
    strcpy(bot_msg.message, welcome_msg);
    strcpy(bot_msg.target, msg.username);

    send(client_socket, (char*)&bot_msg, sizeof(ChatMessage), 0);

    // Handle messages from client
    while (1) {
        bytes_received = recv(client_socket, (char*)&msg, sizeof(ChatMessage), 0);

        if (bytes_received <= 0) {
            // Client disconnected
            EnterCriticalSection(&cs);
            client_sockets[client_index] = INVALID_SOCKET;

            // Broadcast leave message
            msg.type = MSG_SYSTEM;
            strcpy(msg.username, client_usernames[client_index]);
            sprintf(msg.message, "%s has left the chat", client_usernames[client_index]);
            broadcast_message(&msg, 0);

            printf("[Server] %s disconnected\n", client_usernames[client_index]);
            LeaveCriticalSection(&cs);

            closesocket(client_socket);
            break;
        }

        // Always broadcast the user's message to everyone
        broadcast_message(&msg, client_socket);

        // Check if message is for chatbot
        int should_respond = 1;

        // Don't respond to private messages unless addressed to ChatBot
        if (msg.type == MSG_PRIVATE && strcmp(msg.target, "ChatBot") != 0) {
            should_respond = 0;
        }

        if (should_respond) {
            // Generate chatbot response
            char bot_response[MAX_MESSAGE];
            chatbot_response(msg.username, msg.message, bot_response);

            // Send response back to sender
            ChatMessage response_msg;
            response_msg.type = MSG_NORMAL;
            strcpy(response_msg.username, "ChatBot");
            strcpy(response_msg.message, bot_response);
            strcpy(response_msg.target, msg.username);

            send(client_socket, (char*)&response_msg, sizeof(ChatMessage), 0);

            // Also broadcast to all if not private
            if (msg.type != MSG_PRIVATE) {
                // Send to everyone except the original sender
                EnterCriticalSection(&cs);
                for (int i = 0; i < client_count; i++) {
                    if (client_sockets[i] != INVALID_SOCKET &&
                        client_sockets[i] != client_socket &&
                        strcmp(client_usernames[i], msg.username) != 0) {
                        send(client_sockets[i], (char*)&response_msg, sizeof(ChatMessage), 0);
                    }
                }
                LeaveCriticalSection(&cs);
            }
        }
    }

    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, new_socket;
    struct sockaddr_in server, client;
    int c;

    InitializeCriticalSection(&cs);

    // Seed random number generator for chatbot
    srand((unsigned int)time(NULL));

    // Display ChatBot mode with better detection
    printf("----------------------------------------------------------\n");
    printf("           EHKILI CHATBOT SERVER - Starting...         \n");
    printf("_________________________________________________________\n\n");

    // Check API configuration
    int has_openai = (OPENAI_API_KEY && strlen(OPENAI_API_KEY) > 0 && strcmp(OPENAI_API_KEY, "") != 0);
    int has_cohere = (Cohere_APIKEY && strlen(Cohere_APIKEY) > 0 && strcmp(Cohere_APIKEY, "") != 0);

    printf("ChatBot Mode Detection:\n");
    printf("_________________________________________________________\n");

    if (has_openai) {
        printf(" OpenAI API       : ENABLED \n");
    }
    else {
        printf("  OpenAI API       : Not configured\n");
    }

    if (has_cohere) {
        printf("  Cohere API       : ENABLED \n");
    }
    else {
        printf("  Cohere API       : Not configured\n");
    }

    printf("  Offline Mode       : ENABLED \n");
    printf("----------------------------------------------------\n\n");

    if (has_openai || has_cohere) {
        printf("AI Mode: ONLINE - Using intelligent AI responses\n");
        if (has_openai) {
            printf("   Priority: OpenAI --> Cohere --> Offline\n");
        }
        else {
            printf("   Priority: Cohere --> Offline\n");
        }
    }
    else {
        printf("AI Mode: OFFLINE - Using rule-based responses\n");
        printf("\n");
    }

    printf("\n");

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("  WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
    printf("  Winsock initialized\n");

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf(" Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("  Socket created\n");

    // Prepare sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(SERVER_PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("  Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    printf("  Bind successful on port %d\n", SERVER_PORT);

    // Listen
    listen(server_socket, MAX_CLIENTS);
    printf("  Server listening...\n");
    printf("\n");
    printf("========================================================\n");
    printf("     EHKILI Server Ready! Waiting for clients...   \n");
    printf("=========================================================\n\n");

    // Initialize client sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = INVALID_SOCKET;
    }

    c = sizeof(struct sockaddr_in);

    while (1) {
        // Accept connection
        new_socket = accept(server_socket, (struct sockaddr*)&client, &c);
        if (new_socket == INVALID_SOCKET) {
            printf("  Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        printf("[Server] Connection accepted from %s:%d\n",
            inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        // Create thread for client
        HANDLE thread = CreateThread(NULL, 0, client_handler, &new_socket, 0, NULL);
        if (thread == NULL) {
            printf("  Thread creation failed\n");
            closesocket(new_socket);
        }
        else {
            CloseHandle(thread);
        }
    }

    DeleteCriticalSection(&cs);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}