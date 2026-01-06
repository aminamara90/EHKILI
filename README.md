# 🤖 Ehkili ChatBot

**Ehkili ChatBot** is a high-performance, multi-threaded C-based desktop application developed in **Visual Studio 2026**. It bridges low-level socket programming with modern generative AI, offering a hybrid communication system that works both online and offline.

---

## ✨ Features

* **Hybrid AI Engine:** Dynamically switches between:
    * **Online Mode:** Powered by **Cohere AI** (via `WinHTTP`) for intelligent, context-aware conversations.
    * **Offline Mode:** Instant, keyword-based responses with a local library of predefined replies.
* **Bilingual Support:** Includes built-in support for English and Tunisian Arabic (Tunsi) phrases (e.g., "3asslema!", "Marhbe bik!").
* **Multi-threaded Architecture:**
    * **Server:** Spawns a dedicated `client_handler` thread for every connection.
    * **Client:** Uses a background `receive_messages` thread to keep the Win32 GUI smooth and responsive.
* **Native Win32 Interface:** A fully functional Windows GUI featuring a real-time chat log, status indicators, and mode toggling.

---

## 🏗️ Project Architecture

The project follows a **Client–Server** model using TCP/IP Sockets:



1.  **Server (`EHkili Server`):**
    * `server.c`: Manages connections, multi-threading, and broadcasting.
    * `chatbot.c`: Contains the logic for the Cohere API integration and keyword matching.
2.  **Client (`EHkili Client`):**
    * `chat_window.c`: Manages the Win32 GUI elements and event loops.
    * `client.c`: Handles Winsock initialization and window launch.
3.  **Shared Logic:**
    * `common.h`: Defines the `ChatMessage` struct, port settings, and API keys.

---

## 🔑 API Configuration

To enable the AI features, you must configure your **Cohere API Key**.

1.  Get a free API key at [Cohere.com](https://cohere.com/).
2.  Open `common.h`.
3.  Locate and update the following line:

```c
#ifndef Cohere_APIKEY
#define Cohere_APIKEY "YOUR_ACTUAL_API_KEY_HERE"
#endif
