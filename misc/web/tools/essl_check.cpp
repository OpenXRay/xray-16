#include "../../../src/Layers/xrRenderPC_GL/rgl_essl_web.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

static std::string read_file(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open " + path.string());
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

static std::vector<std::string> g_sourceFiles; // index = #line source-string number

static size_t line_of(const std::string& text, size_t pos)
{
    return 1 + std::count(text.begin(), text.begin() + pos, '\n');
}

static std::string expand_includes(const fs::path& root, const fs::path& file)
{
    const std::string text = read_file(file);
    const size_t index = g_sourceFiles.size();
    g_sourceFiles.push_back(file.string());

    std::string result = "#line 1 " + std::to_string(index) + "\n";
    size_t pos = 0;
    while (true)
    {
        const size_t directive = text.find("#include", pos);
        if (directive == std::string::npos)
        {
            result += text.substr(pos);
            return result;
        }
        result += text.substr(pos, directive - pos);
        const size_t open = text.find('"', directive);
        const size_t close = text.find('"', open + 1);
        std::string name = text.substr(open + 1, close - open - 1);
        std::replace(name.begin(), name.end(), '\\', '/');
        result += '\n';
        result += expand_includes(root, root / name);
        result += "\n#line " + std::to_string(line_of(text, close)) + " " + std::to_string(index) + "\n";
        pos = close + 1;
    }
}

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        std::cerr << "usage: essl_check <shader-root> <file> <vert|frag> <out> [DEFINE=VALUE...]\n";
        return 2;
    }
    const fs::path root = argv[1];
    const bool vertexStage = std::string(argv[3]) == "vert";

    std::string source = "#version 300 es\n";
    for (int i = 5; i < argc; ++i)
    {
        std::string define = argv[i];
        const size_t eq = define.find('=');
        source += "#define " + define.substr(0, eq) + "\t" + (eq == std::string::npos ? "1" : define.substr(eq + 1)) + "\n";
    }
    source += expand_includes(root, argv[2]);

    std::ofstream out(argv[4], std::ios::binary);
    out << xray::render::essl::rewrite(source, vertexStage);

    std::ofstream files(std::string(argv[4]) + ".files");
    for (const std::string& path : g_sourceFiles)
        files << path << '\n';
    return 0;
}
