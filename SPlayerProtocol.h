/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * SPlayer Protocol Definitions
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace SPlayerProtocol
{
    enum class MessageType
    {
        Unknown,
        Welcome,
        StatusChange,
        SongChange,
        ProgressChange,
        LyricChange,
        ControlResponse,
        Error
    };

    enum class ControlCommand
    {
        Toggle,
        Play,
        Pause,
        Next,
        Prev
    };

    struct SongInfo
    {
        std::wstring title;
        std::wstring name;
        std::wstring artist;
        std::wstring album;
        int64_t duration = 0;
    };

    struct ProgressInfo
    {
        int64_t currentTime = 0;
        int64_t duration = 0;
    };

    struct LrcLine
    {
        int64_t time = 0;
        std::wstring text;
        std::wstring translation;  // translatedLyric from SPlayer
    };

    struct YrcWord
    {
        int64_t startTime = 0;
        int64_t duration = 0;
        std::wstring text;
    };

    struct YrcLine
    {
        int64_t startTime = 0;
        int64_t endTime = 0;
        std::vector<YrcWord> words;
        std::wstring translation;  // translatedLyric from SPlayer
    };

    struct LyricData
    {
        std::vector<LrcLine> lrcData;
        std::vector<YrcLine> yrcData;
        std::vector<LrcLine> transData;
        bool hasYrc() const { return !yrcData.empty(); }
        bool hasLrc() const { return !lrcData.empty(); }
        bool empty() const { return lrcData.empty() && yrcData.empty(); }
    };

    inline const char* CommandToString(ControlCommand cmd)
    {
        switch (cmd)
        {
        case ControlCommand::Toggle: return "toggle";
        case ControlCommand::Play:   return "play";
        case ControlCommand::Pause:  return "pause";
        case ControlCommand::Next:   return "next";
        case ControlCommand::Prev:   return "prev";
        default: return "toggle";
        }
    }

    inline MessageType ParseMessageType(const std::string& type)
    {
        if (type == "welcome")          return MessageType::Welcome;
        if (type == "status-change")    return MessageType::StatusChange;
        if (type == "song-change")      return MessageType::SongChange;
        if (type == "progress-change")  return MessageType::ProgressChange;
        if (type == "lyric-change")     return MessageType::LyricChange;
        if (type == "control-response") return MessageType::ControlResponse;
        if (type == "error")            return MessageType::Error;
        return MessageType::Unknown;
    }
}
