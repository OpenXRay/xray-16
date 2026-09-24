#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace xray::render::essl
{
namespace detail
{
inline bool is_identifier_start(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
inline bool is_identifier_char(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

inline std::string_view strip_comment(std::string_view line)
{
    const size_t comment = line.find("//");
    return comment == std::string_view::npos ? line : line.substr(0, comment);
}

inline std::vector<std::string> tokenize(std::string_view line)
{
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < line.size())
    {
        const char c = line[i];
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            ++i;
            continue;
        }
        if (is_identifier_char(c))
        {
            size_t end = i;
            while (end < line.size() && is_identifier_char(line[end]))
                ++end;
            tokens.emplace_back(line.substr(i, end - i));
            i = end;
            continue;
        }
        tokens.emplace_back(1, c);
        ++i;
    }
    return tokens;
}

struct varying_declaration
{
    std::string location;
    std::string direction;
    std::string type;
    std::string name;
};

inline bool parse_varying(const std::vector<std::string>& tokens, varying_declaration& declaration)
{
    if (tokens.size() != 10)
        return false;
    if (tokens[0] != "layout" || tokens[1] != "(" || tokens[2] != "location" || tokens[3] != "=" || tokens[5] != ")")
        return false;
    if (tokens[6] != "in" && tokens[6] != "out")
        return false;
    if (tokens[9] != ";")
        return false;
    declaration = { tokens[4], tokens[6], tokens[7], tokens[8] };
    return true;
}

inline bool parse_numeric_define(const std::vector<std::string>& tokens, std::string& name, std::string& value)
{
    if (tokens.size() != 4 || tokens[0] != "#" || tokens[1] != "define")
        return false;
    if (!std::isdigit(static_cast<unsigned char>(tokens[3][0])))
        return false;
    name = tokens[2];
    value = tokens[3];
    return true;
}

inline bool parse_fragment_output(const std::vector<std::string>& tokens, std::string& location)
{
    if (tokens.size() != 4 || tokens[0] != "out" || tokens[3] != ";")
        return false;
    const std::string& name = tokens[2];
    if (name.rfind("SV_Target", 0) != 0)
        return false;
    location = name.size() > 9 ? name.substr(9) : "0";
    return true;
}

inline bool is_builtin_redeclaration(const std::vector<std::string>& tokens)
{
    if (tokens.size() < 3 || (tokens[0] != "in" && tokens[0] != "out"))
        return false;
    return tokens[2].rfind("gl_", 0) == 0;
}

inline bool guard_conditional(std::string_view line, const std::vector<std::string>& tokens, std::string& guarded)
{
    if (tokens.size() < 3 || tokens[0] != "#" || (tokens[1] != "if" && tokens[1] != "elif"))
        return false;

    std::vector<std::string> identifiers;
    for (size_t i = 2; i < tokens.size(); ++i)
    {
        const std::string& token = tokens[i];
        if (token == "defined")
            return false;
        if (!is_identifier_start(token[0]))
            continue;
        if (std::find(identifiers.begin(), identifiers.end(), token) == identifiers.end())
            identifiers.push_back(token);
    }
    if (identifiers.empty())
        return false;

    const std::string_view expression = strip_comment(line).substr(line.find(tokens[1]) + tokens[1].size());
    guarded = "#" + tokens[1] + " ";
    for (const std::string& identifier : identifiers)
        guarded += "defined(" + identifier + ") && ";
    guarded += "(";
    guarded.append(expression.data(), expression.size());
    guarded += ")";
    return true;
}

using conditional_stack = std::vector<std::vector<std::string>>;

inline void track_conditional(
    const std::vector<std::string>& tokens, const std::string& directive, conditional_stack& stack)
{
    if (tokens.size() < 2 || tokens[0] != "#")
        return;
    const std::string& keyword = tokens[1];
    if (keyword == "if" || keyword == "ifdef" || keyword == "ifndef")
        stack.push_back({ directive });
    else if ((keyword == "elif" || keyword == "else") && !stack.empty())
        stack.back().push_back(directive);
    else if (keyword == "endif" && !stack.empty())
        stack.pop_back();
}

inline std::string wrap_in_conditionals(const conditional_stack& stack, const std::string& statement)
{
    std::string result;
    for (const auto& frame : stack)
        for (const std::string& directive : frame)
            result += directive + "\n";
    result += statement + "\n";
    for (size_t i = 0; i < stack.size(); ++i)
        result += "#endif\n";
    return result;
}

struct stage_varying
{
    varying_declaration declaration;
    std::string interface_name;
    conditional_stack conditionals;
};

struct main_body
{
    size_t bodyStart; // the opening brace
    size_t bodyEnd;   // the closing brace
};

inline std::vector<main_body> find_main_bodies(const std::string& text)
{
    std::vector<main_body> bodies;
    size_t search = 0;
    while (true)
    {
        const size_t mainPos = text.find("void main", search);
        if (mainPos == std::string::npos)
            return bodies;
        search = mainPos + 9;

        const size_t bodyStart = text.find('{', mainPos);
        if (bodyStart == std::string::npos)
            return bodies;

        int depth = 0;
        for (size_t i = bodyStart; i < text.size(); ++i)
        {
            if (text[i] == '/' && i + 1 < text.size() && text[i + 1] == '/')
            {
                i = text.find('\n', i);
                if (i == std::string::npos)
                    break;
                continue;
            }
            if (text[i] == '{')
                ++depth;
            else if (text[i] == '}' && --depth == 0)
            {
                bodies.push_back({ bodyStart, i });
                search = i + 1;
                break;
            }
        }
        if (bodies.empty() || bodies.back().bodyStart != bodyStart)
            return bodies;
    }
}

constexpr std::string_view preamble =
    "precision highp float;\n"
    "precision highp int;\n"
    "precision highp sampler2D;\n"
    "precision highp sampler3D;\n"
    "precision highp samplerCube;\n"
    "precision highp sampler2DShadow;\n"
    "precision highp sampler2DArray;\n"
    "vec4 xr_widen(float v) { return vec4(v, 0.0, 0.0, 0.0); }\n"
    "vec4 xr_widen(vec2 v) { return vec4(v, 0.0, 0.0); }\n"
    "vec4 xr_widen(vec3 v) { return vec4(v, 0.0); }\n"
    "vec4 xr_widen(vec4 v) { return v; }\n";
} // namespace detail

inline std::string rewrite(const std::string& source, bool vertexStage)
{
    using namespace detail;

    std::unordered_map<std::string, std::string> defines;
    std::vector<stage_varying> varyings;
    conditional_stack conditionals;
    const std::string rewrittenDirection = vertexStage ? "out" : "in";

    std::string output;
    output.reserve(source.size() + preamble.size());

    bool skippingPerVertexBlock = false;
    bool preambleInserted = false;
    size_t lineStart = 0;
    while (lineStart < source.size())
    {
        size_t lineEnd = source.find('\n', lineStart);
        if (lineEnd == std::string::npos)
            lineEnd = source.size();
        const std::string_view line(source.data() + lineStart, lineEnd - lineStart);
        lineStart = lineEnd + 1;

        if (skippingPerVertexBlock)
        {
            if (line.find("};") != std::string_view::npos)
                skippingPerVertexBlock = false;
            output += '\n';
            continue;
        }
        if (line.find("gl_PerVertex") != std::string_view::npos)
        {
            skippingPerVertexBlock = line.find("};") == std::string_view::npos;
            output += '\n';
            continue;
        }
        if (line.find("#extension") != std::string_view::npos)
        {
            output += '\n';
            continue;
        }

        const auto tokens = tokenize(strip_comment(line));

        if (is_builtin_redeclaration(tokens))
        {
            output += '\n';
            continue;
        }

        std::string name, value;
        if (parse_numeric_define(tokens, name, value))
            defines.emplace(name, value);

        std::string guardedConditional;
        if (guard_conditional(line, tokens, guardedConditional))
        {
            track_conditional(tokens, guardedConditional, conditionals);
            output += guardedConditional + '\n';
            continue;
        }
        track_conditional(tokens, std::string(strip_comment(line)), conditionals);

        varying_declaration declaration;
        if (parse_varying(tokens, declaration) && declaration.direction == rewrittenDirection)
        {
            const auto resolved = defines.find(declaration.location);
            const std::string location = resolved == defines.end() ? declaration.location : resolved->second;
            stage_varying varying{ declaration, "xr_v2p_" + location, conditionals };
            output += declaration.direction + " vec4 " + varying.interface_name + "; "
                + declaration.type + " " + declaration.name + ";\n";
            varyings.push_back(std::move(varying));
            continue;
        }

        std::string outputLocation;
        if (!vertexStage && parse_fragment_output(tokens, outputLocation))
        {
            output += "layout(location = " + outputLocation + ") ";
            output.append(line.data(), line.size());
            output += '\n';
            continue;
        }

        output.append(line.data(), line.size());
        output += '\n';

        if (!preambleInserted && line.rfind("#version", 0) == 0)
        {
            output += preamble;
            preambleInserted = true;
        }
    }

    if (!preambleInserted)
        output.insert(0, preamble);

    const std::vector<main_body> bodies = find_main_bodies(output);
    if (bodies.empty() || varyings.empty())
        return output;

    std::string copies = "\n";
    for (const stage_varying& varying : varyings)
    {
        const std::string statement = vertexStage
            ? varying.interface_name + " = xr_widen(" + varying.declaration.name + ");"
            : varying.declaration.name + " = " + varying.declaration.type + "(" + varying.interface_name + ");";
        copies += wrap_in_conditionals(varying.conditionals, statement);
    }

    for (auto body = bodies.rbegin(); body != bodies.rend(); ++body)
        output.insert(vertexStage ? body->bodyEnd : body->bodyStart + 1, copies);
    return output;
}
} // namespace xray::render::essl
