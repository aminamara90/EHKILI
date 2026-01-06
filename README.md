# 🤖 Ehkili ChatBot: AI-Powered C Messenger

**Ehkili** is a multi-threaded, Client-Server chat application written in C. It features a unique hybrid intelligence system that utilizes the **Cohere AI API** for advanced conversations while maintaining an offline mode for instant, keyword-based responses in English and Tunisian Arabic.

---

## ✨ Features

* **Hybrid AI Logic:** * **Online Mode:** Uses `WinHTTP` to communicate with Cohere's Large Language Models.
    * **Offline Mode:** Uses local pattern matching for instant replies (e.g., "3asslema", "Marhbe").
* **Multi-threaded Architecture:** * **Server:** Handles multiple concurrent clients using `CreateThread` and `client_handler`.
    * **Client:** Maintains a responsive Win32 GUI while receiving messages in a background thread.
* **Bilingual Intelligence:** Specifically tuned to recognize Tunisian dialect (Tunsi) alongside English.
* **Thread Safety:** Implements `CRITICAL_SECTION` to ensure stable message broadcasting across multiple users.

---

## 🏗️ System Architecture

The project is split into two main Visual Studio projects that share a common protocol:



1.  **Server (`EHkili Server`):**
    * `server.c`: The core networking engine. Manages the `client_sockets` array and broadcasts messages.
    * `chatbot.c`: The "brain" that handles AI API requests and JSON parsing (`extract_json_value`).
2.  **Client (`EHkili Client`):**
    * `chat_window.c`: The Win32 GUI implementation (Window procedures, buttons, and layout).
    * `client.c`: Initializes Winsock and launches the application.
3.  **Shared:**
    * `common.h`: Defines the `ChatMessage` structure and shared configuration.

---

## 🛠️ Technologies & Tools

* **Language:** C (C11/C17)
* **IDE:** Visual Studio 2026
* **APIs & Libraries:**
    * `Winsock2.lib`: Low-level TCP/IP networking.
    * `WinHTTP.lib`: Synchronous/Asynchronous HTTPS requests for AI.
    * `Comctl32.lib` & `Gdi32.lib`: Native Windows UI components.
    * **AI Engine:** Cohere API (Command-R model).

---

## 🔑 API Configuration

To enable **Online Mode**, you must provide your own Cohere API key:

1.  Create an account at [Cohere.com](https://cohere.com/).
2.  Copy your API Key from the dashboard.
3.  Open `common.h` and update the following definition:

```c
#ifndef Cohere_APIKEY
#define Cohere_APIKEY "PASTE_YOUR_KEY_HERE"
#endif
