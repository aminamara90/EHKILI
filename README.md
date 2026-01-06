🤖 Ehkili ChatBot - AI Desktop Messenger
Ehkili ChatBot is a high-performance, multi-threaded C-based desktop application that bridges low-level socket programming with modern generative AI. It features a hybrid intelligence system that provides smart responses via the Cohere API when online and instant, predefined replies when offline.

✨ Key Features
Hybrid AI Engine: Dynamically switches between Online Mode (powered by Cohere/OpenAI) and Offline Mode (fast, local keyword-based responses).

Bilingual Intelligence: Capable of understanding and responding in both English and Tunisian Arabic (e.g., "3asslema!", "Marhbe bik!").

Multi-threaded Architecture: * Server: Spawns a dedicated client_handler thread for every connected user to ensure zero-lag message routing.

Client: Uses a background receive_messages thread to keep the Win32 GUI responsive during active communication.

Dynamic Win32 Interface: A fully resizable Windows GUI built with native C, featuring real-time status updates and smooth UI scaling.

Private & Global Messaging: Support for broadcasting messages to all users or targeting the ChatBot specifically.

🛠️ Environment & Prerequisites
Operating System: Windows (uses Win32 API and Winsock2).

IDE: Visual Studio 2026 (or newer).

Required Libraries: * ws2_32.lib (Networking).

winhttp.lib (API communication).

comctl32.lib (UI Controls).

🔑 API Configuration Guide
The project is designed to use the Cohere API for its primary online intelligence. Follow these steps to configure it:

1. Obtain Your API Key
Sign up at Cohere.com.

Navigate to your dashboard and generate a free "Trial" API Key.

2. Configure the Source Code
Open the common.h file in your project. You will find a section dedicated to API configuration:

C

// Locate this section in common.h
#ifndef Cohere_APIKEY
#define Cohere_APIKEY "YOUR_COHERE_API_KEY_HERE"
#endif

// Optional: Configure OpenAI
#ifndef OPENAI_API_KEY
#define OPENAI_API_KEY "YOUR_OPENAI_KEY_HERE"
#endif
Replace YOUR_COHERE_API_KEY_HERE with the key you obtained from Cohere.

Security Note: If you plan to make your repository public, do not hardcode your actual key. Instead, use environment variables or remind users to fill this field locally before compiling.

3. How the API Works
Request: The server uses WinHTTP to send a secure POST request to api.cohere.ai/v1/chat.

Model: By default, it uses the command-r-08-2024 model for conversational accuracy.

Parsing: The project includes a custom JSON utility (extract_json_value) in chatbot.c to parse AI responses without needing external libraries like cJSON.

🚀 How to Run
Build the Solution: Open the project in Visual Studio and build both the Server and Client projects.

Start the Server: Run server.exe. It will listen on port 8888.

Launch the Client: Run client.exe. Enter your username and the server IP (default: 127.0.0.1).

Chat: Toggle between Online and Offline modes using the UI button to see the different response behaviors.
