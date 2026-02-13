#pragma once

#include "xrCore/xrCore.h"

namespace xray::render
{

struct JsonValue
{
    enum Type { Null, Bool, Number, String, Array, Object };
    Type type = Null;
    bool boolVal = false;
    double numVal = 0.0;
    xr_string strVal;
    xr_vector<JsonValue> arrVal;
    xr_map<xr_string, JsonValue> objVal;

    bool is_null() const { return type == Null; }
    bool is_string() const { return type == String; }
    bool is_number() const { return type == Number; }
    bool is_bool() const { return type == Bool; }
    bool is_object() const { return type == Object; }
    bool is_array() const { return type == Array; }

    const JsonValue& operator[](const char* key) const;
    const JsonValue& operator[](size_t index) const;

    float as_float(float def = 0.f) const;
    int as_int(int def = 0) const;
    u32 as_u32(u32 def = 0) const;
    bool as_bool(bool def = false) const;
    const char* as_string(const char* def = "") const;

    bool has(const char* key) const;
    size_t size() const;
};

bool ParseJson(const char* text, JsonValue& out);

} // namespace xray::render
