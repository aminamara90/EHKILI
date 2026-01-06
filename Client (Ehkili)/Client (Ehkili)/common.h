#ifndef COMMON_H
#define COMMON_H

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

// CRITICAL ORDER
#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commctrl.h>
#include <time.h>

// Link required libraries
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")

// FREE API CONFIGURATION
#ifndef Cohere_APIKEY
#define Cohere_APIKEY ""
#endif

// OPTION 2: OpenAI 
#ifndef OPENAI_API_KEY
#define OPENAI_API_KEY ""
#endif

// Server settings
#define SERVER_PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_USERNAME 50
#define MAX_MESSAGE 256

// Message types
#define MSG_NORMAL  0
#define MSG_SYSTEM  1
#define MSG_PRIVATE 2
#define MSG_JOIN   3
#define MSG_LEAVE   4

// AI Settings
#define MAX_AI_RESPONSE 512
#define API_TIMEOUT 15000  

typedef struct {
    int type;
    char username[MAX_USERNAME];
    char message[MAX_MESSAGE];
    char target[MAX_USERNAME];
} ChatMessage;

#ifdef _SERVER
void chatbot_response(const char* username, const char* input, char* response);
int call_openai_api(const char* prompt, char* response, size_t response_size);
int call_cohere_api(const char* prompt, char* response, size_t response_size);
void get_offline_response(const char* input, char* response);
#endif

#endif