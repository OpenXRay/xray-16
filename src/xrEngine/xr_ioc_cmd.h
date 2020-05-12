#pragma once

#define CMD0(cls)                     \
    {                                 \
        static cls x##cls();          \
        Console->AddCommand(&x##cls); \
    }
#define CMD1(cls, p1)                 \
    {                                 \
        static cls x##cls(p1);        \
        Console->AddCommand(&x##cls); \
    }
#define CMD2(cls, p1, p2)             \
    {                                 \
        static cls x##cls(p1, p2);    \
        Console->AddCommand(&x##cls); \
    }
#define CMD3(cls, p1, p2, p3)          \
    {                                  \
        static cls x##cls(p1, p2, p3); \
        Console->AddCommand(&x##cls);  \
    }
#define CMD4(cls, p1, p2, p3, p4)          \
    {                                      \
        static cls x##cls(p1, p2, p3, p4); \
        Console->AddCommand(&x##cls);      \
    }

#include "xrCore/xrCore_benchmark_macros.h"
#include "xrCore/xr_token.h"

extern ENGINE_API bool renderer_allow_override; // allows to change renderer setting

class ENGINE_API IConsole_Command
{
public:
    friend class CConsole;
    typedef char TInfo[256];
    typedef char TStatus[256];
    typedef xr_vector<shared_str> vecTips;
    typedef xr_vector<shared_str> vecLRU;

protected:
    pcstr cName;
    bool bEnabled;
    bool bLowerCaseArgs;
    bool bEmptyArgsHandled;

    vecLRU m_LRU;

    enum
    {
        LRU_MAX_COUNT = 10
    };

    IC bool EQ(pcstr S1, pcstr S2) { return xr_strcmp(S1, S2) == 0; }
public:
    IConsole_Command(pcstr N BENCH_SEC_SIGN) : cName(N), bEnabled(true), bLowerCaseArgs(false), bEmptyArgsHandled(false)
    {
        m_LRU.reserve(LRU_MAX_COUNT + 1);
        m_LRU.clear();
    }
    virtual ~IConsole_Command()
    {
        if (Console)
            Console->RemoveCommand(this);
    };

    BENCH_SEC_SCRAMBLEVTBL3

    pcstr Name() { return cName; }
    void InvalidSyntax();
    virtual void Execute(pcstr args) = 0;
    virtual void GetStatus(TStatus& S) { S[0] = 0; }
    virtual void Info(TInfo& I) { xr_strcpy(I, "(no arguments)"); }
    virtual void Save(IWriter* F)
    {
        TStatus S;
        GetStatus(S);
        if (S[0])
            F->w_printf("%s %s\r\n", cName, S);
    }

    BENCH_SEC_SCRAMBLEVTBL2

    virtual void fill_tips(vecTips& tips, u32 /*mode*/) { add_LRU_to_tips(tips); }
    // vecLRU& LRU () { return m_LRU; }
    virtual void add_to_LRU(shared_str const& arg);
    void add_LRU_to_tips(vecTips& tips);

}; // class IConsole_Command

class ENGINE_API CCC_Mask : public IConsole_Command
{
protected:
    Flags32* value;
    u32 mask;

public:
    CCC_Mask(pcstr N, Flags32* V, u32 M) : IConsole_Command(N), value(V), mask(M){};
    const bool GetValue() const { return value->test(mask); }
    virtual void Execute(pcstr args)
    {
        if (EQ(args, "on"))
            value->set(mask, true);
        else if (EQ(args, "off"))
            value->set(mask, false);
        else if (EQ(args, "1"))
            value->set(mask, true);
        else if (EQ(args, "0"))
            value->set(mask, false);
        else
            InvalidSyntax();
    }
    virtual void GetStatus(TStatus& S) { xr_strcpy(S, value->test(mask) ? "on" : "off"); }
    virtual void Info(TInfo& I) { xr_strcpy(I, "'on/off' or '1/0'"); }
    virtual void fill_tips(vecTips& tips, u32 /*mode*/)
    {
        TStatus str;
        xr_sprintf(str, sizeof(str), "%s (current) [on/off]", value->test(mask) ? "on" : "off");
        tips.push_back(str);
    }
};

class ENGINE_API CCC_ToggleMask : public IConsole_Command
{
protected:
    Flags32* value;
    u32 mask;

public:
    CCC_ToggleMask(pcstr N, Flags32* V, u32 M) : IConsole_Command(N), value(V), mask(M) { bEmptyArgsHandled = true; }
    const bool GetValue() const { return value->test(mask); }
    virtual void Execute(pcstr /*args*/)
    {
        value->set(mask, !GetValue());
        TStatus S;
        strconcat(sizeof(S), S, cName, " is ", value->test(mask) ? "on" : "off");
        Log(S);
    }
    virtual void GetStatus(TStatus& S) { xr_strcpy(S, value->test(mask) ? "on" : "off"); }
    virtual void Info(TInfo& I) { xr_strcpy(I, "'on/off' or '1/0'"); }
    virtual void fill_tips(vecTips& tips, u32 /*mode*/)
    {
        TStatus str;
        xr_sprintf(str, sizeof(str), "%s (current) [on/off]", value->test(mask) ? "on" : "off");
        tips.push_back(str);
    }
};

class ENGINE_API CCC_Token : public IConsole_Command
{
protected:
    u32* value;
    const xr_token* tokens;

public:
    CCC_Token(pcstr N, u32* V, const xr_token* T) : IConsole_Command(N), value(V), tokens(T){}

    virtual void Execute(pcstr args)
    {
        const xr_token* tok = GetToken();
        if (!tok)
        {
            Msg("! token [%s] is null", cName);
            return;
        }
        while (tok->name)
        {
            if (xr_stricmp(tok->name, args) == 0)
            {
                *value = tok->id;
                break;
            }
            tok++;
        }
        if (!tok->name)
            InvalidSyntax();
    }
    virtual void GetStatus(TStatus& S)
    {
        const xr_token* tok = GetToken();
        while (tok->name)
        {
            if (tok->id == (int)(*value))
            {
                xr_strcpy(S, tok->name);
                return;
            }
            tok++;
        }
        xr_strcpy(S, "?");
        return;
    }
    virtual void Info(TInfo& I)
    {
        I[0] = 0;
        const xr_token* tok = GetToken();
        while (tok->name)
        {
            if (I[0])
                xr_strcat(I, "/");
            xr_strcat(I, tok->name);
            tok++;
        }
    }
    virtual const xr_token* GetToken() noexcept { return tokens; }
    virtual void fill_tips(vecTips& tips, u32 /*mode*/)
    {
        TStatus str;
        bool res = false;
        const xr_token* tok = GetToken();
        while (tok->name && !res)
        {
            if (tok->id == (int)(*value))
            {
                xr_sprintf(str, sizeof(str), "%s (current)", tok->name);
                tips.push_back(str);
                res = true;
            }
            tok++;
        }
        if (!res)
        {
            tips.push_back("--- (current)");
        }
        tok = GetToken();
        while (tok->name)
        {
            tips.push_back(tok->name);
            tok++;
        }
    }
};

class ENGINE_API CCC_Float : public IConsole_Command
{
protected:
    float* value;
    float min, max;

public:
    CCC_Float(pcstr N, float* V, float _min = 0, float _max = 1)
        : IConsole_Command(N), value(V), min(_min), max(_max){}
    const float GetValue() const { return *value; }
    void GetBounds(float& fmin, float& fmax) const
    {
        fmin = min;
        fmax = max;
    }

    virtual void Execute(pcstr args)
    {
        float v = float(atof(args));
        if (v < (min - EPS) || v > (max + EPS))
            InvalidSyntax();
        else
            *value = v;
    }
    virtual void GetStatus(TStatus& S)
    {
        xr_sprintf(S, sizeof(S), "%3.5f", *value);
        while (xr_strlen(S) && ('0' == S[xr_strlen(S) - 1]))
            S[xr_strlen(S) - 1] = 0;
    }
    virtual void Info(TInfo& I) { xr_sprintf(I, sizeof(I), "float value in range [%3.3f,%3.3f]", min, max); }
    virtual void fill_tips(vecTips& tips, u32 mode)
    {
        TStatus str;
        xr_sprintf(str, sizeof(str), "%3.5f (current) [%3.3f,%3.3f]", *value, min, max);
        tips.push_back(str);
        IConsole_Command::fill_tips(tips, mode);
    }
};

class ENGINE_API CCC_Vector3 : public IConsole_Command
{
protected:
    Fvector* value;
    Fvector min, max;

public:
    CCC_Vector3(pcstr N, Fvector* V, const Fvector _min, const Fvector _max) : IConsole_Command(N), value(V)
    {
        min.set(_min);
        max.set(_max);
    };
    const Fvector GetValue() const { return *value; }
    Fvector* GetValuePtr() const { return value; }
    virtual void Execute(pcstr args)
    {
        Fvector v;
        if (3 != sscanf(args, "%f,%f,%f", &v.x, &v.y, &v.z))
        {
            if (3 != sscanf(args, "(%f,%f,%f)", &v.x, &v.y, &v.z))
            {
                InvalidSyntax();
                return;
            }
        }
        if (v.x < min.x || v.y < min.y || v.z < min.z)
        {
            InvalidSyntax();
            return;
        }
        if (v.x > max.x || v.y > max.y || v.z > max.z)
        {
            InvalidSyntax();
            return;
        }
        value->set(v);
    }
    virtual void GetStatus(TStatus& S) { xr_sprintf(S, sizeof(S), "(%f, %f, %f)", value->x, value->y, value->z); }
    virtual void Info(TInfo& I)
    {
        xr_sprintf(I, sizeof(I), "vector3 in range [%e,%e,%e]-[%e,%e,%e]", min.x, min.y, min.z, max.x, max.y, max.z);
    }
    virtual void fill_tips(vecTips& tips, u32 mode)
    {
        TStatus str;
        xr_sprintf(str, sizeof(str), "(%e, %e, %e) (current) [(%e,%e,%e)-(%e,%e,%e)]", value->x, value->y, value->z,
            min.x, min.y, min.z, max.x, max.y, max.z);
        tips.push_back(str);
        IConsole_Command::fill_tips(tips, mode);
    }
};

class ENGINE_API CCC_Integer : public IConsole_Command
{
protected:
    int* value;
    int min, max;

public:
    const int GetValue() const { return *value; }
    void GetBounds(int& imin, int& imax) const
    {
        imin = min;
        imax = max;
    }

    CCC_Integer(pcstr N, int* V, int _min = 0, int _max = 999) : IConsole_Command(N), value(V), min(_min), max(_max){}

    virtual void Execute(pcstr args)
    {
        int v = atoi(args);
        if (v < min || v > max)
            InvalidSyntax();
        else
            *value = v;
    }
    virtual void GetStatus(TStatus& S) { xr_itoa(*value, S, 10); }
    virtual void Info(TInfo& I) { xr_sprintf(I, sizeof(I), "integer value in range [%d,%d]", min, max); }
    virtual void fill_tips(vecTips& tips, u32 mode)
    {
        TStatus str;
        xr_sprintf(str, sizeof(str), "%d (current) [%d,%d]", *value, min, max);
        tips.push_back(str);
        IConsole_Command::fill_tips(tips, mode);
    }
};

class ENGINE_API CCC_String : public IConsole_Command
{
protected:
    pstr value;
    int size;

public:
    CCC_String(pcstr N, pstr V, int _size = 2) : IConsole_Command(N), value(V), size(_size)
    {
        bLowerCaseArgs = false;
        R_ASSERT(V);
        R_ASSERT(size > 1);
    };

    virtual void Execute(pcstr args) { strncpy_s(value, size, args, size - 1); }
    virtual void GetStatus(TStatus& S) { xr_strcpy(S, value); }
    virtual void Info(TInfo& I) { xr_sprintf(I, sizeof(I), "string with up to %d characters", size); }
    virtual void fill_tips(vecTips& tips, u32 mode)
    {
        tips.push_back((pcstr)value);
        IConsole_Command::fill_tips(tips, mode);
    }
};

class ENGINE_API CCC_LoadCFG : public IConsole_Command
{
public:
    virtual bool allow(pcstr /*cmd*/) { return true; }
    CCC_LoadCFG(pcstr N);
    virtual void Execute(pcstr args);
};

class ENGINE_API CCC_LoadCFG_custom : public CCC_LoadCFG
{
    string64 m_cmd;

public:
    CCC_LoadCFG_custom(pcstr cmd);
    virtual bool allow(pcstr cmd);
};
