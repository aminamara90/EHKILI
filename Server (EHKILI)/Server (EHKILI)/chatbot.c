#define _SERVER
#include "common.h"
#include <ctype.h>

// JSON PARSING UTILITIES
static void unescape_json(char* str) {
    char* src = str;
    char* dst = str;
    while (*src) {
        if (*src == '\\' && src[1]) {
            src++;
            switch (*src) {
            case 'n': *dst++ = '\n'; break;
            case 't': *dst++ = '\t'; break;
            case 'r': *dst++ = '\r'; break;
            case '"': *dst++ = '"'; break;
            case '\\': *dst++ = '\\'; break;
            default: *dst++ = *src; break;
            }
            src++;
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static int extract_json_value(const char* json, const char* key, char* value, size_t value_size) {
    char search_pattern[128];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);

    const char* start = strstr(json, search_pattern);
    if (!start) return 0;

    start += strlen(search_pattern);
    while (*start && isspace(*start)) start++;

    if (*start == '"') {
        start++;
        const char* end = start;
        while (*end && *end != '"') {
            if (*end == '\\' && end[1]) end += 2;
            else end++;
        }
        if (*end != '"') return 0;

        size_t len = end - start;
        if (len >= value_size) len = value_size - 1;
        strncpy(value, start, len);
        value[len] = '\0';
        unescape_json(value);
        return 1;
    }

    return 0;
}

// ============================================================
// COHERE API - FREE TIER 
// ============================================================

int call_cohere_api(const char* prompt, char* response, size_t response_size) {
    if (!Cohere_APIKEY || strlen(Cohere_APIKEY) == 0) {
        printf("[API] No Cohere key configured\n");
        return -1;
    }

    printf("[API] Calling Cohere (FREE tier)...\n");

    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    char full_response[32768] = { 0 };
    DWORD bytesRead;
    BOOL bResults;
    int success = 0;

    hSession = WinHttpOpen(L"EhkiliChatBot/2.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) {
        printf("[API] Failed to init WinHTTP\n");
        return -1;
    }

    DWORD timeout = 15000; 
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);

    // Cohere API endpoint
    hConnect = WinHttpConnect(hSession, L"api.cohere.ai",
        INTERNET_DEFAULT_HTTPS_PORT, 0);

    if (!hConnect) {
        printf("[API] Failed to connect to Cohere\n");
        goto cleanup_cohere;
    }

    // Using chat endpoint
    hRequest = WinHttpOpenRequest(hConnect, L"POST",
        L"/v1/chat",
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest) {
        printf("[API] Failed to create request\n");
        goto cleanup_cohere;
    }

    // Escape prompt for JSON
    char escaped_prompt[1024];
    const char* src = prompt;
    char* dst = escaped_prompt;
    while (*src && (dst - escaped_prompt) < sizeof(escaped_prompt) - 2) {
        if (*src == '"') *dst++ = '\\';
        if (*src == '\\') *dst++ = '\\';
        if (*src == '\n') {
            *dst++ = '\\';
            *dst++ = 'n';
            src++;
            continue;
        }
        *dst++ = *src++;
    }
    *dst = '\0';

    // Build JSON body
    char json_body[2048];
    snprintf(json_body, sizeof(json_body),
        "{"
        "\"message\":\"%s\","
        "\"model\":\"command-r-08-2024\","
        "\"temperature\":0.7,"
        "\"max_tokens\":150"
        "}",
        escaped_prompt);

    // Build headers - Cohere
    wchar_t headers[1024];
    swprintf(headers, sizeof(headers) / sizeof(wchar_t),
        L"Content-Type: application/json\r\n"
        L"Authorization: Bearer %hs",
        Cohere_APIKEY);  

    printf("[API] Sending request to Cohere...\n");

    bResults = WinHttpSendRequest(hRequest, headers, (DWORD)-1L,
        json_body, (DWORD)strlen(json_body),
        (DWORD)strlen(json_body), 0);

    if (!bResults) {
        printf("[API] Send failed: %d\n", GetLastError());
        goto cleanup_cohere;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        printf("[API] Receive failed: %d\n", GetLastError());
        goto cleanup_cohere;
    }

    // Check status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        NULL, &statusCode, &statusCodeSize, NULL);

    printf("[API] Cohere status: %d\n", statusCode);

    // Read response
    while (WinHttpQueryDataAvailable(hRequest, &bytesRead) && bytesRead > 0) {
        if (strlen(full_response) + bytesRead >= sizeof(full_response)) break;

        char* buffer = malloc(bytesRead + 1);
        if (!buffer) break;

        DWORD dwRead = 0;
        if (WinHttpReadData(hRequest, buffer, bytesRead, &dwRead)) {
            buffer[dwRead] = '\0';
            strcat(full_response, buffer);
        }
        free(buffer);
    }

    if (statusCode != 200) {
        printf("[API] Error response: %.300s\n", full_response);
        goto cleanup_cohere;
    }

    printf("[API] Raw response: %.200s\n", full_response);

    // Parse Cohere response 
    if (extract_json_value(full_response, "text", response, response_size)) {
        // Clean up whitespace
        char* trimmed = response;
        while (*trimmed && isspace(*trimmed)) trimmed++;
        if (trimmed != response) {
            memmove(response, trimmed, strlen(trimmed) + 1);
        }

        if (strlen(response) > 0) {
            printf("[API] Cohere SUCCESS: %.50s...\n", response);
            success = 1;
        }
    }
    else {
        printf("[API] Failed to parse Cohere JSON\n");
    }

cleanup_cohere:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return success ? 0 : -1;
}

// OPENAI API

int call_openai_api(const char* prompt, char* response, size_t response_size) {
    if (!OPENAI_API_KEY || strlen(OPENAI_API_KEY) == 0) {
        return -1;
    }

    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    char full_response[16384] = { 0 };
    DWORD bytesRead;
    BOOL bResults;
    int success = 0;

    hSession = WinHttpOpen(L"EhkiliChatBot/2.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) return -1;

    DWORD timeout = API_TIMEOUT;
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);

    hConnect = WinHttpConnect(hSession, L"api.openai.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);

    if (!hConnect) goto cleanup_openai;

    hRequest = WinHttpOpenRequest(hConnect, L"POST",
        L"/v1/chat/completions",
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest) goto cleanup_openai;

    char escaped_prompt[1024];
    const char* src = prompt;
    char* dst = escaped_prompt;
    while (*src && (dst - escaped_prompt) < sizeof(escaped_prompt) - 2) {
        if (*src == '"' || *src == '\\') *dst++ = '\\';
        *dst++ = *src++;
    }
    *dst = '\0';

    char json_body[2048];
    snprintf(json_body, sizeof(json_body),
        "{"
        "\"model\":\"gpt-3.5-turbo\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"You are a friendly chatbot. Keep responses brief.\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}"
        "],"
        "\"max_tokens\":200,"
        "\"temperature\":0.8"
        "}",
        escaped_prompt);

    wchar_t headers[2048];
    swprintf(headers, sizeof(headers) / sizeof(wchar_t),
        L"Content-Type: application/json\r\n"
        L"Authorization: Bearer %hs",
        OPENAI_API_KEY);

    bResults = WinHttpSendRequest(hRequest, headers, (DWORD)-1L,
        json_body, (DWORD)strlen(json_body),
        (DWORD)strlen(json_body), 0);

    if (!bResults || !WinHttpReceiveResponse(hRequest, NULL)) {
        goto cleanup_openai;
    }

    while (WinHttpQueryDataAvailable(hRequest, &bytesRead) && bytesRead > 0) {
        if (strlen(full_response) + bytesRead >= sizeof(full_response)) break;
        char* buffer = malloc(bytesRead + 1);
        if (!buffer) break;
        DWORD dwRead = 0;
        if (WinHttpReadData(hRequest, buffer, bytesRead, &dwRead)) {
            buffer[dwRead] = '\0';
            strcat(full_response, buffer);
        }
        free(buffer);
    }

    if (extract_json_value(full_response, "content", response, response_size)) {
        printf("[API] OpenAI success\n");
        success = 1;
    }

cleanup_openai:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return success ? 0 : -1;
}

// ============================================================
// OFFLINE MODE
// ============================================================

void get_offline_response(const char* input, char* response) {
    char lower_input[256];
    strncpy(lower_input, input, sizeof(lower_input) - 1);
    lower_input[sizeof(lower_input) - 1] = '\0';
    for (char* p = lower_input; *p; p++) *p = tolower(*p);

    // Greetings (English + Tunisian)
    if (strstr(lower_input, "hello") || strstr(lower_input, "hi") || strstr(lower_input, "hey") ||
        strstr(lower_input, "3asslema") || strstr(lower_input, "slm") || strstr(lower_input, "ahla")) {
        const char* greetings[] = {
            "Hello! Marhbe bik! How can I help you?",
            "3asslema! Ahla w sahla! What can I do for you?",
            "Hey there! Kifeh, ch3amel cv?",
            "Hi! Labess? What's up?"
        };
        strcpy(response, greetings[rand() % 4]);
    }
    // How are you
    else if (strstr(lower_input, "how are you") || strstr(lower_input, "labess 3lik") || strstr(lower_input, "cv") ||
        strstr(lower_input, "labess") || strstr(lower_input, "chnahwalek")) {
        const char* status[] = {
            "I'm doing great, thanks! w enti?",
            "Labess el hamdulillah! How about you?",
            "I'm excellent! Behi behi, w enti chneya 7welik?",
            "All good! hmdlh!",
            "Jawna bhi w enti?"
        };
        strcpy(response, status[rand() % 5]);
    }
    // Identity
    else if (strstr(lower_input, "your name") || strstr(lower_input, "who are you") ||
        strstr(lower_input, "chkoun enti")) {
        strcpy(response, "Eni Ehkili ChatBot! Your friendly Tunisian-English assistant! Ana hne bech n3awnek!");
    }
    // Help
    else if (strstr(lower_input, "help") || strstr(lower_input, "3aweni")) {
        strcpy(response, "I can help you with: jokes (nokta), time (we9t), date (tarikh), weather, or just chat! Chbik?");
    }
    // Jokes
    else if (strstr(lower_input, "joke") || strstr(lower_input, "funny") ||
        strstr(lower_input, "nokta") || strstr(lower_input, "9oul nokta") || strstr(lower_input, "glegt")) {
        const char* jokes[] = {
            "Why don't programmers like nature? Too many bugs! Barcha 7acharat! ",
            "What's a computer's favorite snack? Microchips! Ya7ki 3la chips! ",
            "Why did the developer go broke? Used up all his cache! Khalles el cache mte3ou! ",
            "Kifech tfahem developer? Ta9oulo console.log! "
        };
        strcpy(response, jokes[rand() % 4]);
    }
    // Time
    else if (strstr(lower_input, "time") || strstr(lower_input, "wa9t") || strstr(lower_input, "9adech lwa9t taw")) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        sprintf(response, "The time is %02d:%02d:%02d - El we9t taw : %02d:%02d! ",
            t->tm_hour, t->tm_min, t->tm_sec, t->tm_hour, t->tm_min);
    }
    // Date
    else if (strstr(lower_input, "date") || strstr(lower_input, "today") ||
        strstr(lower_input, "tarikh")) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        const char* days[] = { "Sunday/El 7ad", "Monday/Ethnin", "Tuesday/Tletha",
                             "Wednesday/Larb3a", "Thursday/Khamis", "Friday/Jom3a", "Saturday/Sebt" };
        sprintf(response, "Today is %s ", days[t->tm_wday]);
    }
    // Weather
    else if (strstr(lower_input, "weather") || strstr(lower_input, "ta9s")) {
        strcpy(response, "I can't check real weather, but I hope it's nice! Inchallah ta9s behi 3andkom! ");
    }
    // Thanks
    else if (strstr(lower_input, "thank") || strstr(lower_input, "merci") ||
        strstr(lower_input, "aaychek") || strstr(lower_input, "merci 3lik")) {
        const char* thanks[] = {
            "You're welcome! B seha! ",
            "No problem! Mefama 7ata mochkla!",
            "Anytime! Wa9t ma t7eb! <3",
            "Far7an li ena 3awentek! :3"
        };
        strcpy(response, thanks[rand() % 4]);
    }
    // Goodbye
    else if (strstr(lower_input, "bye") || strstr(lower_input, "goodbye") ||
        strstr(lower_input, "beselema") || strstr(lower_input, "yatik saha") || strstr(lower_input, "nchoufouk ay")) {
        const char* byes[] = {
            "Goodbye! Beselema! ;)",
            "See you! Yatik essaha enti zeda!",
            "Take care! Rabi ma3ak! <3",
            "Bye! Beselema w besseha! :)",
            "Nchoufek bkhir nchlh !"
        };
        strcpy(response, byes[rand() % 4]);
    }
    // Compliments
    else if (strstr(lower_input, "love") || strstr(lower_input, "like") || strstr(lower_input, "3ejbetni")) {
        const char* positive[] = {
            "That's wonderful! 7aja behya barcha! ",
            "Awesome! Mouch b6al! ",
            "I'm glad! Fara7t barcha! ",
            "That's great! Haja m9a9sa! "
        };
        strcpy(response, positive[rand() % 4]);
    }
    else if (strstr(lower_input, "mregel") || strstr(lower_input, "mreglin") ||
        strstr(lower_input, "behi") || strstr(lower_input, "jawek bhi")) {
        const char* positive[] = {
            "mregel sahbi ! test7a9 7eja okhra? ",
            "Awesome! You need something else?"
        };
        strcpy(response, positive[rand() % 2]);
    }
    else if (strstr(lower_input, "netawaa") || strstr(lower_input, "future visions") || strstr(lower_input, "netawaamall")) {
        const char* positive[] = {
             "Netawaa Mall is a youth-led event focused on volunteering and social impact, scheduled for December 17, 2025, at the Salle de fêtes The Queen in Sousse. It is designed as a public gathering to showcase projects, foster connections, and inspire community change through various interactive spaces including workshops, innovation displays, and networking circles. The event expects to host over 300 attendees, feature more than 20 initiatives, and include presentations from 10+ speakers."
        };
        strcpy(response, positive[0]);
    }
    // Generic responses
    else {
        const char* generic[] = {
            "That's interesting! Tell me more! 9olli akther!",
            "I see! Fehemt! Can you elaborate?",
            "Fascinating! Haja interissente! What else?",
            "Cool! Ya3tik! I'd love to hear more!",
            "Interesting point! 7aja behi! Continue!",
            "Ma fhemtech kan tzid tfasserli akther !"
        };
        strcpy(response, generic[rand() % 5]);
    }
}

// ============================================================
// MAIN CHATBOT RESPONSE FUNCTION
// ============================================================

void chatbot_response(const char* username, const char* input, char* response) {
    printf("[ChatBot] Message from %s: %.50s\n", username, input);

    // Try OpenAI first (if configured)
    if (call_openai_api(input, response, MAX_MESSAGE) == 0) {
        printf("[EHKILI] Using OpenAI\n");
        return;
    }

    // Try Cohere 
    if (call_cohere_api(input, response, MAX_MESSAGE) == 0) {
        printf("[EHKILI] Using Cohere (FREE)\n");
        return;
    }

    // Fallback to offline mode
    printf("[EHKILI] Using offline mode\n");
    get_offline_response(input, response);
}