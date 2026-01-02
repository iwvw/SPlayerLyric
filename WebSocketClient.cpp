/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * WebSocket Client Implementation
 */

#include "pch.h"
#include <afxwin.h>
#include "WebSocketClient.h"
#include <ws2tcpip.h>
#include "Config.h"
#include <sstream>
#include <random>
#include <algorithm>

// Use nlohmann/json for robust JSON parsing
#include "nlohmann_json.hpp"
using json = nlohmann::json;

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string Base64Encode(const unsigned char* data, size_t len)
{
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (len--)
    {
        char_array_3[i++] = *(data++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while (i++ < 3)
            ret += '=';
    }

    return ret;
}

static std::string GenerateWebSocketKey()
{
    unsigned char key[16];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < 16; i++)
        key[i] = static_cast<unsigned char>(dis(gen));

    return Base64Encode(key, 16);
}

static std::wstring Utf8ToWideLocal(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
    if (size <= 0) return std::wstring();
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &result[0], size);
    return result;
}

WebSocketClient& WebSocketClient::Instance()
{
    static WebSocketClient instance;
    return instance;
}

WebSocketClient::WebSocketClient()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

WebSocketClient::~WebSocketClient()
{
    Stop();
    WSACleanup();
}

void WebSocketClient::Start(int port)
{
    if (m_running)
        return;

    m_port = port;
    m_running = true;
    m_workerThread = std::thread(&WebSocketClient::WorkerThread, this);
}

void WebSocketClient::Stop()
{
    m_running = false;

    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    if (m_workerThread.joinable())
        m_workerThread.join();

    m_connected = false;
}

void WebSocketClient::SetCallbacks(const WebSocketCallbacks& callbacks)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks = callbacks;
}

void WebSocketClient::SendControl(SPlayerProtocol::ControlCommand cmd)
{
    if (!m_connected)
        return;

    std::string message = "{\"type\":\"control\",\"data\":{\"command\":\"";
    message += SPlayerProtocol::CommandToString(cmd);
    message += "\"}}";

    SendMessage(message);
}

// Read exactly n bytes from socket (blocking)
bool WebSocketClient::ReadBytes(char* buffer, size_t n)
{
    size_t totalRead = 0;
    while (totalRead < n && m_running && m_connected)
    {
        int result = recv(m_socket, buffer + totalRead, (int)(n - totalRead), 0);
        if (result > 0)
        {
            totalRead += result;
        }
        else if (result == 0)
        {
            return false; // Connection closed
        }
        else
        {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK)
            {
                Sleep(10);
                continue;
            }
            return false;
        }
    }
    return totalRead == n;
}

void WebSocketClient::WorkerThread()
{
    while (m_running)
    {
        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET)
        {
            Sleep(g_config.Data().reconnectInterval);
            continue;
        }

        sockaddr_in serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(static_cast<u_short>(m_port));
        inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

        if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            Sleep(g_config.Data().reconnectInterval);
            continue;
        }

        std::string wsKey = GenerateWebSocketKey();
        std::ostringstream request;
        request << "GET / HTTP/1.1\r\n"
            << "Host: 127.0.0.1:" << m_port << "\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << "Sec-WebSocket-Key: " << wsKey << "\r\n"
            << "Sec-WebSocket-Version: 13\r\n"
            << "\r\n";

        std::string reqStr = request.str();
        if (send(m_socket, reqStr.c_str(), (int)reqStr.size(), 0) == SOCKET_ERROR)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            Sleep(g_config.Data().reconnectInterval);
            continue;
        }

        char buffer[1024];
        int received = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            Sleep(g_config.Data().reconnectInterval);
            continue;
        }

        buffer[received] = '\0';
        std::string response(buffer);

        if (response.find("101") == std::string::npos)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            Sleep(g_config.Data().reconnectInterval);
            continue;
        }

        m_connected = true;
        OutputDebugStringW(L"[SPlayerLyric] Connected to SPlayer\n");
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            if (m_callbacks.onConnected)
                m_callbacks.onConnected();
        }

        // Set non-blocking with timeout
        u_long mode = 1;
        ioctlsocket(m_socket, FIONBIO, &mode);

        std::string msgAccumulator;

        while (m_running && m_connected)
        {
            // Read WebSocket frame header (2 bytes minimum)
            unsigned char header[2];
            
            int result = recv(m_socket, (char*)header, 2, MSG_PEEK);
            if (result < 2)
            {
                if (result == 0)
                {
                    m_connected = false;
                    break;
                }
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK)
                {
                    Sleep(50);
                    continue;
                }
                m_connected = false;
                break;
            }

            // Now read the full header
            recv(m_socket, (char*)header, 2, 0);

            bool fin = (header[0] & 0x80) != 0;
            int opcode = header[0] & 0x0F;
            bool masked = (header[1] & 0x80) != 0;
            uint64_t payloadLen = header[1] & 0x7F;

            // Read extended length if needed
            if (payloadLen == 126)
            {
                unsigned char extLen[2];
                if (!ReadBytes((char*)extLen, 2))
                {
                    m_connected = false;
                    break;
                }
                payloadLen = (extLen[0] << 8) | extLen[1];
            }
            else if (payloadLen == 127)
            {
                unsigned char extLen[8];
                if (!ReadBytes((char*)extLen, 8))
                {
                    m_connected = false;
                    break;
                }
                payloadLen = 0;
                for (int i = 0; i < 8; i++)
                    payloadLen = (payloadLen << 8) | extLen[i];
            }

            // Read mask key if present (should not be for server->client)
            unsigned char maskKey[4] = { 0 };
            if (masked)
            {
                if (!ReadBytes((char*)maskKey, 4))
                {
                    m_connected = false;
                    break;
                }
            }

            // Read payload
            std::string payload;
            if (payloadLen > 0)
            {
                payload.resize(payloadLen);
                size_t totalRead = 0;
                while (totalRead < payloadLen && m_running && m_connected)
                {
                    int toRead = (int)min(payloadLen - totalRead, (uint64_t)8192);
                    int r = recv(m_socket, &payload[totalRead], toRead, 0);
                    if (r > 0)
                    {
                        totalRead += r;
                    }
                    else if (r == 0)
                    {
                        m_connected = false;
                        break;
                    }
                    else
                    {
                        int err = WSAGetLastError();
                        if (err == WSAEWOULDBLOCK)
                        {
                            Sleep(10);
                            continue;
                        }
                        m_connected = false;
                        break;
                    }
                }

                if (totalRead < payloadLen)
                    break;

                // Unmask if needed
                if (masked)
                {
                    for (size_t i = 0; i < payloadLen; i++)
                        payload[i] ^= maskKey[i % 4];
                }
            }

            // Handle frame
            if (opcode == 0x1)  // Text frame
            {
                msgAccumulator = payload;
                if (fin)
                {
                    ParseMessage(msgAccumulator);
                    msgAccumulator.clear();
                }
            }
            else if (opcode == 0x0)  // Continuation frame
            {
                msgAccumulator += payload;
                if (fin)
                {
                    ParseMessage(msgAccumulator);
                    msgAccumulator.clear();
                }
            }
            else if (opcode == 0x8)  // Close
            {
                m_connected = false;
                break;
            }
            else if (opcode == 0x9)  // Ping
            {
                unsigned char pong[2] = { 0x8A, 0x00 };
                send(m_socket, (char*)pong, 2, 0);
            }
            // Ignore other opcodes
        }

        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        m_connected = false;
        OutputDebugStringW(L"[SPlayerLyric] Disconnected from SPlayer\n");

        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            if (m_callbacks.onDisconnected)
                m_callbacks.onDisconnected();
        }

        if (m_running)
            Sleep(g_config.Data().reconnectInterval);
    }
}

bool WebSocketClient::SendMessage(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_sendMutex);

    if (!m_connected || m_socket == INVALID_SOCKET)
        return false;

    std::vector<unsigned char> frame;

    frame.push_back(0x81);

    size_t len = msg.size();
    if (len < 126)
    {
        frame.push_back(static_cast<unsigned char>(0x80 | len));
    }
    else if (len < 65536)
    {
        frame.push_back(0xFE);
        frame.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<unsigned char>(len & 0xFF));
    }
    else
    {
        frame.push_back(0xFF);
        for (int i = 7; i >= 0; i--)
            frame.push_back(static_cast<unsigned char>((len >> (i * 8)) & 0xFF));
    }

    unsigned char mask[4];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (int i = 0; i < 4; i++)
        mask[i] = static_cast<unsigned char>(dis(gen));

    frame.insert(frame.end(), mask, mask + 4);

    for (size_t i = 0; i < len; i++)
        frame.push_back(msg[i] ^ mask[i % 4]);

    int sent = send(m_socket, (char*)frame.data(), (int)frame.size(), 0);
    return sent == (int)frame.size();
}

void WebSocketClient::ParseMessage(const std::string& message)
{
    try
    {
        json j = json::parse(message);
        
        std::string type = j.value("type", "");
        
        OutputDebugStringA("[SPlayerLyric] Message type: ");
        OutputDebugStringA(type.c_str());
        OutputDebugStringA("\n");

        auto msgType = SPlayerProtocol::ParseMessageType(type);

        std::lock_guard<std::mutex> lock(m_callbackMutex);

        switch (msgType)
        {
        case SPlayerProtocol::MessageType::StatusChange:
        {
            if (m_callbacks.onStatusChange)
            {
                bool isPlaying = j["data"].value("status", false);
                m_callbacks.onStatusChange(isPlaying);
            }
            break;
        }

        case SPlayerProtocol::MessageType::SongChange:
        {
            if (m_callbacks.onSongChange)
            {
                SPlayerProtocol::SongInfo info;
                auto& data = j["data"];
                info.title = Utf8ToWideLocal(data.value("title", ""));
                info.name = Utf8ToWideLocal(data.value("name", ""));
                info.artist = Utf8ToWideLocal(data.value("artist", ""));
                info.album = Utf8ToWideLocal(data.value("album", ""));
                info.duration = data.value("duration", 0);
                m_callbacks.onSongChange(info);
            }
            break;
        }

        case SPlayerProtocol::MessageType::ProgressChange:
        {
            if (m_callbacks.onProgressChange)
            {
                SPlayerProtocol::ProgressInfo info;
                auto& data = j["data"];
                info.currentTime = data.value("currentTime", 0);
                info.duration = data.value("duration", 0);
                m_callbacks.onProgressChange(info);
            }
            break;
        }

        case SPlayerProtocol::MessageType::LyricChange:
        {
            if (m_callbacks.onLyricChange)
            {
                SPlayerProtocol::LyricData lyricData;
                auto& data = j["data"];

                OutputDebugStringW(L"[SPlayerLyric] Processing lyric-change\n");

                // Parse lrcData - SPlayer format has words array inside each line
                if (data.contains("lrcData") && data["lrcData"].is_array())
                {
                    for (auto& item : data["lrcData"])
                    {
                        SPlayerProtocol::LrcLine line;
                        line.time = item.value("startTime", (int64_t)0);
                        
                        // Get translation from translatedLyric field
                        std::string transText = item.value("translatedLyric", "");
                        if (!transText.empty())
                            line.translation = Utf8ToWideLocal(transText);
                        
                        // Get text from words array
                        if (item.contains("words") && item["words"].is_array() && !item["words"].empty())
                        {
                            std::wstring combined;
                            for (auto& wordItem : item["words"])
                            {
                                std::string word = wordItem.value("word", "");
                                if (!word.empty())
                                    combined += Utf8ToWideLocal(word);
                            }
                            line.text = combined;
                        }
                        
                        if (!line.text.empty())
                            lyricData.lrcData.push_back(line);
                    }
                }

                // Parse yrcData - similar structure
                if (data.contains("yrcData") && data["yrcData"].is_array())
                {
                    for (auto& item : data["yrcData"])
                    {
                        SPlayerProtocol::YrcLine line;
                        line.startTime = item.value("startTime", (int64_t)0);
                        line.endTime = item.value("endTime", (int64_t)0);
                        
                        // Get translation from translatedLyric field
                        std::string transText = item.value("translatedLyric", "");
                        if (!transText.empty())
                            line.translation = Utf8ToWideLocal(transText);

                        if (item.contains("words") && item["words"].is_array())
                        {
                            for (auto& wordItem : item["words"])
                            {
                                SPlayerProtocol::YrcWord word;
                                word.startTime = wordItem.value("startTime", (int64_t)0);
                                int64_t endTime = wordItem.value("endTime", (int64_t)0);
                                word.duration = endTime - word.startTime;
                                word.text = Utf8ToWideLocal(wordItem.value("word", ""));
                                if (!word.text.empty())
                                    line.words.push_back(word);
                            }
                        }

                        if (!line.words.empty())
                            lyricData.yrcData.push_back(line);
                    }
                }

                // Parse transData
                if (data.contains("transData") && data["transData"].is_array())
                {
                    for (auto& item : data["transData"])
                    {
                        SPlayerProtocol::LrcLine line;
                        line.time = item.value("startTime", (int64_t)0);
                        line.text = Utf8ToWideLocal(item.value("word", ""));
                        if (!line.text.empty())
                            lyricData.transData.push_back(line);
                    }
                }

                wchar_t buf[128];
                swprintf_s(buf, L"[SPlayerLyric] Parsed LRC=%zu, YRC=%zu, TRANS=%zu\n",
                    lyricData.lrcData.size(), lyricData.yrcData.size(), lyricData.transData.size());
                OutputDebugStringW(buf);

                m_callbacks.onLyricChange(lyricData);
            }
            break;
        }

        case SPlayerProtocol::MessageType::Error:
        {
            if (m_callbacks.onError)
            {
                std::string errorMsg = j["data"].value("message", "Unknown error");
                m_callbacks.onError(errorMsg);
            }
            break;
        }

        default:
            break;
        }
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA("[SPlayerLyric] JSON parse error: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
    }
}
