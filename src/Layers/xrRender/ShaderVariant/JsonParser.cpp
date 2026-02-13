#include "stdafx.h"
#include "JsonParser.h"

namespace xray::render
{

static const JsonValue s_nullValue;

const JsonValue& JsonValue::operator[](const char* key) const
{
    if (type != Object)
        return s_nullValue;
    auto it = objVal.find(key);
    return it != objVal.end() ? it->second : s_nullValue;
}

const JsonValue& JsonValue::operator[](size_t index) const
{
    if (type != Array || index >= arrVal.size())
        return s_nullValue;
    return arrVal[index];
}

float JsonValue::as_float(float def) const { return type == Number ? static_cast<float>(numVal) : def; }
int JsonValue::as_int(int def) const { return type == Number ? static_cast<int>(numVal) : def; }
u32 JsonValue::as_u32(u32 def) const { return type == Number ? static_cast<u32>(numVal) : def; }
bool JsonValue::as_bool(bool def) const { return type == Bool ? boolVal : def; }
const char* JsonValue::as_string(const char* def) const { return type == String ? strVal.c_str() : def; }

bool JsonValue::has(const char* key) const
{
    return type == Object && objVal.contains(key);
}

size_t JsonValue::size() const
{
    if (type == Array) return arrVal.size();
    if (type == Object) return objVal.size();
    return 0;
}

struct JsonParser
{
    const char* p;
    const char* end;

    void skipWhitespace()
    {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
            ++p;
    }

    bool expect(char c)
    {
        skipWhitespace();
        if (p < end && *p == c) { ++p; return true; }
        return false;
    }

    bool parseValue(JsonValue& out)
    {
        skipWhitespace();
        if (p >= end) return false;

        switch (*p)
        {
        case '"': return parseString(out);
        case '{': return parseObject(out);
        case '[': return parseArray(out);
        case 't': return parseLiteral("true", out, JsonValue::Bool, true);
        case 'f': return parseLiteral("false", out, JsonValue::Bool, false);
        case 'n': return parseLiteral("null", out, JsonValue::Null, false);
        default:
            if (*p == '-' || (*p >= '0' && *p <= '9'))
                return parseNumber(out);
            return false;
        }
    }

    bool parseStringRaw(xr_string& s)
    {
        if (*p != '"') return false;
        ++p;
        s.clear();
        while (p < end && *p != '"')
        {
            if (*p == '\\')
            {
                ++p;
                if (p >= end) return false;
                switch (*p)
                {
                case '"': s += '"'; break;
                case '\\': s += '\\'; break;
                case '/': s += '/'; break;
                case 'n': s += '\n'; break;
                case 't': s += '\t'; break;
                case 'r': s += '\r'; break;
                default: s += *p; break;
                }
            }
            else
            {
                s += *p;
            }
            ++p;
        }
        if (p >= end) return false;
        ++p;
        return true;
    }

    bool parseString(JsonValue& out)
    {
        out.type = JsonValue::String;
        return parseStringRaw(out.strVal);
    }

    bool parseNumber(JsonValue& out)
    {
        const char* start = p;
        if (*p == '-') ++p;
        while (p < end && *p >= '0' && *p <= '9') ++p;
        if (p < end && *p == '.')
        {
            ++p;
            while (p < end && *p >= '0' && *p <= '9') ++p;
        }
        if (p == start || (p == start + 1 && *start == '-'))
            return false;
        out.type = JsonValue::Number;
        double result = 0.0;
        bool negative = (*start == '-');
        const char* s = negative ? start + 1 : start;
        while (s < p && *s != '.')
            result = result * 10.0 + (*s++ - '0');
        if (s < p && *s == '.')
        {
            ++s;
            double frac = 0.1;
            while (s < p)
            {
                result += (*s++ - '0') * frac;
                frac *= 0.1;
            }
        }
        out.numVal = negative ? -result : result;
        return true;
    }

    bool parseLiteral(const char* lit, JsonValue& out, JsonValue::Type t, bool bval)
    {
        size_t len = strlen(lit);
        if (static_cast<size_t>(end - p) < len || memcmp(p, lit, len) != 0)
            return false;
        p += len;
        out.type = t;
        out.boolVal = bval;
        return true;
    }

    bool parseObject(JsonValue& out)
    {
        if (*p != '{') return false;
        ++p;
        out.type = JsonValue::Object;
        out.objVal.clear();
        skipWhitespace();
        if (p < end && *p == '}') { ++p; return true; }
        for (;;)
        {
            skipWhitespace();
            if (p >= end) return false;
            xr_string key;
            if (!parseStringRaw(key)) return false;
            skipWhitespace();
            if (!expect(':')) return false;
            JsonValue val;
            if (!parseValue(val)) return false;
            out.objVal[std::move(key)] = std::move(val);
            skipWhitespace();
            if (p < end && *p == ',') { ++p; continue; }
            if (p < end && *p == '}') { ++p; return true; }
            return false;
        }
    }

    bool parseArray(JsonValue& out)
    {
        if (*p != '[') return false;
        ++p;
        out.type = JsonValue::Array;
        out.arrVal.clear();
        skipWhitespace();
        if (p < end && *p == ']') { ++p; return true; }
        for (;;)
        {
            JsonValue val;
            if (!parseValue(val)) return false;
            out.arrVal.push_back(std::move(val));
            skipWhitespace();
            if (p < end && *p == ',') { ++p; continue; }
            if (p < end && *p == ']') { ++p; return true; }
            return false;
        }
    }
};

bool ParseJson(const char* text, JsonValue& out)
{
    if (!text) return false;
    JsonParser parser;
    parser.p = text;
    parser.end = text + strlen(text);
    parser.skipWhitespace();
    if (!parser.parseValue(out)) return false;
    parser.skipWhitespace();
    return true;
}

} // namespace xray::render
