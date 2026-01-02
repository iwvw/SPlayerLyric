/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * JSON Parser Implementation
 */

#include "pch.h"
#include "JsonParser.h"
#include <cctype>
#include <cstdlib>

JsonValue JsonParser::Parse(const std::string& json)
{
    JsonParser parser(json);
    return parser.parseValue();
}

JsonValue JsonParser::parseValue()
{
    skipWhitespace();
    char c = peek();

    if (c == '{') return parseObject();
    if (c == '[') return parseArray();
    if (c == '"') return parseString();
    if (c == '-' || std::isdigit(c)) return parseNumber();
    if (std::isalpha(c)) return parseKeyword();

    return JsonValue();
}

JsonValue JsonParser::parseObject()
{
    JsonValue::Object obj;

    get();
    skipWhitespace();

    if (peek() == '}')
    {
        get();
        return JsonValue(obj);
    }

    while (true)
    {
        skipWhitespace();
        if (peek() != '"') break;

        JsonValue keyVal = parseString();
        std::string key = keyVal.asString();

        skipWhitespace();
        if (!match(':')) break;

        JsonValue value = parseValue();
        obj[key] = value;

        skipWhitespace();
        if (match(',')) continue;
        if (match('}')) break;
        break;
    }

    return JsonValue(obj);
}

JsonValue JsonParser::parseArray()
{
    JsonValue::Array arr;

    get();
    skipWhitespace();

    if (peek() == ']')
    {
        get();
        return JsonValue(arr);
    }

    while (true)
    {
        JsonValue value = parseValue();
        arr.push_back(value);

        skipWhitespace();
        if (match(',')) continue;
        if (match(']')) break;
        break;
    }

    return JsonValue(arr);
}

JsonValue JsonParser::parseString()
{
    get();

    std::string result;
    while (peek() != '"' && peek() != '\0')
    {
        char c = get();
        if (c == '\\')
        {
            c = get();
            switch (c)
            {
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            case '/': result += '/'; break;
            case 'u':
            {
                std::string hex;
                for (int i = 0; i < 4 && m_pos < m_json.size(); ++i)
                    hex += get();
                if (hex.size() == 4)
                {
                    wchar_t wc = static_cast<wchar_t>(std::strtol(hex.c_str(), nullptr, 16));
                    char buf[4] = { 0 };
                    int len = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, buf, 4, nullptr, nullptr);
                    result.append(buf, len);
                }
                break;
            }
            default: result += c; break;
            }
        }
        else
        {
            result += c;
        }
    }

    get();
    return JsonValue(result);
}

JsonValue JsonParser::parseNumber()
{
    size_t start = m_pos;
    bool isDouble = false;

    if (peek() == '-') get();

    while (std::isdigit(peek())) get();

    if (peek() == '.')
    {
        isDouble = true;
        get();
        while (std::isdigit(peek())) get();
    }

    if (peek() == 'e' || peek() == 'E')
    {
        isDouble = true;
        get();
        if (peek() == '+' || peek() == '-') get();
        while (std::isdigit(peek())) get();
    }

    std::string numStr = m_json.substr(start, m_pos - start);

    if (isDouble)
        return JsonValue(std::stod(numStr));
    else
        return JsonValue(static_cast<int64_t>(std::stoll(numStr)));
}

JsonValue JsonParser::parseKeyword()
{
    size_t start = m_pos;
    while (std::isalpha(peek())) get();

    std::string keyword = m_json.substr(start, m_pos - start);

    if (keyword == "true") return JsonValue(true);
    if (keyword == "false") return JsonValue(false);
    if (keyword == "null") return JsonValue();

    return JsonValue();
}

void JsonParser::skipWhitespace()
{
    while (m_pos < m_json.size() && std::isspace(m_json[m_pos]))
        ++m_pos;
}
