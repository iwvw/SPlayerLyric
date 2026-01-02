/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * Simple JSON Parser
 */

#pragma once

#include <string>
#include <map>
#include <vector>

class JsonValue
{
public:
    using Object = std::map<std::string, JsonValue>;
    using Array = std::vector<JsonValue>;

    JsonValue() : m_type(Type::Null) {}
    JsonValue(bool b) : m_type(Type::Bool), m_bool(b) {}
    JsonValue(int64_t i) : m_type(Type::Int), m_int(i) {}
    JsonValue(double d) : m_type(Type::Double), m_double(d) {}
    JsonValue(const std::string& s) : m_type(Type::String), m_string(s) {}
    JsonValue(std::string&& s) : m_type(Type::String), m_string(std::move(s)) {}
    JsonValue(const Object& o) : m_type(Type::Object), m_object(o) {}
    JsonValue(const Array& a) : m_type(Type::Array), m_array(a) {}

    bool isNull() const { return m_type == Type::Null; }
    bool isBool() const { return m_type == Type::Bool; }
    bool isInt() const { return m_type == Type::Int; }
    bool isDouble() const { return m_type == Type::Double; }
    bool isNumber() const { return m_type == Type::Int || m_type == Type::Double; }
    bool isString() const { return m_type == Type::String; }
    bool isObject() const { return m_type == Type::Object; }
    bool isArray() const { return m_type == Type::Array; }

    bool asBool(bool def = false) const { return m_type == Type::Bool ? m_bool : def; }
    int64_t asInt(int64_t def = 0) const 
    { 
        if (m_type == Type::Int) return m_int;
        if (m_type == Type::Double) return static_cast<int64_t>(m_double);
        return def;
    }
    double asDouble(double def = 0.0) const
    {
        if (m_type == Type::Double) return m_double;
        if (m_type == Type::Int) return static_cast<double>(m_int);
        return def;
    }
    const std::string& asString() const { return m_string; }
    const Object& asObject() const { return m_object; }
    const Array& asArray() const { return m_array; }

    const JsonValue& operator[](const std::string& key) const
    {
        static JsonValue nullValue;
        if (m_type != Type::Object) return nullValue;
        auto it = m_object.find(key);
        return it != m_object.end() ? it->second : nullValue;
    }

    const JsonValue& operator[](size_t index) const
    {
        static JsonValue nullValue;
        if (m_type != Type::Array || index >= m_array.size()) return nullValue;
        return m_array[index];
    }

    size_t size() const
    {
        if (m_type == Type::Array) return m_array.size();
        if (m_type == Type::Object) return m_object.size();
        return 0;
    }

    bool contains(const std::string& key) const
    {
        return m_type == Type::Object && m_object.find(key) != m_object.end();
    }

private:
    enum class Type { Null, Bool, Int, Double, String, Object, Array };

    Type m_type;
    bool m_bool = false;
    int64_t m_int = 0;
    double m_double = 0.0;
    std::string m_string;
    Object m_object;
    Array m_array;
};

class JsonParser
{
public:
    static JsonValue Parse(const std::string& json);

private:
    JsonParser(const std::string& json) : m_json(json), m_pos(0) {}

    JsonValue parseValue();
    JsonValue parseObject();
    JsonValue parseArray();
    JsonValue parseString();
    JsonValue parseNumber();
    JsonValue parseKeyword();

    void skipWhitespace();
    char peek() const { return m_pos < m_json.size() ? m_json[m_pos] : '\0'; }
    char get() { return m_pos < m_json.size() ? m_json[m_pos++] : '\0'; }
    bool match(char c) { if (peek() == c) { ++m_pos; return true; } return false; }

    const std::string& m_json;
    size_t m_pos;
};

inline std::wstring Utf8ToWide(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();

    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
    if (size <= 0) return std::wstring();

    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &result[0], size);
    return result;
}

inline std::string WideToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return std::string();

    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return std::string();

    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), &result[0], size, nullptr, nullptr);
    return result;
}
