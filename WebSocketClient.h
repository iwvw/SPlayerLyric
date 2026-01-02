/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * WebSocket Client Interface
 */

#pragma once

#include "SPlayerProtocol.h"
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

struct WebSocketCallbacks
{
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(bool isPlaying)> onStatusChange;
    std::function<void(const SPlayerProtocol::SongInfo&)> onSongChange;
    std::function<void(const SPlayerProtocol::ProgressInfo&)> onProgressChange;
    std::function<void(const SPlayerProtocol::LyricData&)> onLyricChange;
    std::function<void(const std::string&)> onError;
};

class WebSocketClient
{
public:
    static WebSocketClient& Instance();

    void Start(int port = 25885);
    void Stop();

    bool IsConnected() const { return m_connected; }

    void SendControl(SPlayerProtocol::ControlCommand cmd);
    void SetCallbacks(const WebSocketCallbacks& callbacks);

private:
    WebSocketClient();
    ~WebSocketClient();

    void WorkerThread();
    void ParseMessage(const std::string& message);
    bool SendMessage(const std::string& msg);
    bool ReadBytes(char* buffer, size_t n);

    std::thread m_workerThread;
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_connected{ false };

    int m_port = 25885;
    SOCKET m_socket = INVALID_SOCKET;

    std::mutex m_callbackMutex;
    WebSocketCallbacks m_callbacks;

    std::mutex m_sendMutex;
};

#define g_wsClient WebSocketClient::Instance()
