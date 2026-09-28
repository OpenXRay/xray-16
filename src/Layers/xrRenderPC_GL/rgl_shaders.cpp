#include "stdafx.h"
#include "r2.h"

#include "Layers/xrRender/ShaderResourceTraits.h"
#include "xrCore/FileCRC32.h"

namespace xray::render::RENDER_NAMESPACE
{
#if defined(XR_PLATFORM_ANDROID)
namespace
{
constexpr cpcstr AndroidVaryingPrefix = "_xray_gles_varying_";

struct AndroidGlslLineRewrite
{
    cpcstr path;
    cpcstr source;
    cpcstr replacement;
};

constexpr AndroidGlslLineRewrite AndroidGlslLineRewrites[] =
{
#define XR_GLES_LINE_REWRITE(path, source, replacement) { path, source, replacement },
#include "AndroidGlslCompatRules.inl"
#undef XR_GLES_LINE_REWRITE
};

bool is_glsl_space(char value)
{
    return value == ' ' || value == '\t' || value == '\r';
}

size_t skip_glsl_space(const xr_string& value, size_t position)
{
    while (position < value.size() && is_glsl_space(value[position]))
        ++position;
    return position;
}

bool read_glsl_identifier(const xr_string& value, size_t& position, size_t& begin, size_t& end)
{
    position = skip_glsl_space(value, position);
    if (position >= value.size())
        return false;

    const char first = value[position];
    if (!((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') || first == '_'))
        return false;

    begin = position++;
    while (position < value.size())
    {
        const char current = value[position];
        if (!((current >= 'A' && current <= 'Z') || (current >= 'a' && current <= 'z') ||
                (current >= '0' && current <= '9') || current == '_'))
            break;
        ++position;
    }
    end = position;
    return true;
}

bool is_glsl_qualifier(const xr_string& token)
{
    return token == "flat" || token == "smooth" || token == "noperspective" || token == "centroid" ||
        token == "sample" || token == "invariant" || token == "precise";
}

bool glsl_line_matches_ignoring_space(const xr_string& line, size_t first, size_t last, cpcstr source)
{
    size_t linePosition = first;
    size_t sourcePosition = 0;
    while (true)
    {
        while (linePosition < last && is_glsl_space(line[linePosition]))
            ++linePosition;
        while (source[sourcePosition] && is_glsl_space(source[sourcePosition]))
            ++sourcePosition;

        if (linePosition == last || !source[sourcePosition])
            return linePosition == last && !source[sourcePosition];
        if (line[linePosition++] != source[sourcePosition++])
            return false;
    }
}

xr_string normalize_android_shader_path(cpcstr sourcePath)
{
    xr_string normalized = sourcePath ? sourcePath : "";
    for (char& character : normalized)
    {
        if (character == '\\')
            character = '/';
        else if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character - 'A' + 'a');
    }
    return normalized;
}

bool android_shader_path_matches(const xr_string& sourcePath, cpcstr rulePath)
{
    const size_t ruleLength = xr_strlen(rulePath);
    if (sourcePath.size() < ruleLength ||
        sourcePath.compare(sourcePath.size() - ruleLength, ruleLength, rulePath) != 0)
        return false;

    return sourcePath.size() == ruleLength || sourcePath[sourcePath.size() - ruleLength - 1] == '/';
}

bool apply_android_line_rewrite(
    xr_string& line, size_t first, size_t last, const xr_string& sourcePath)
{
    const size_t length = last - first;
    for (const auto& rewrite : AndroidGlslLineRewrites)
    {
        if (!android_shader_path_matches(sourcePath, rewrite.path))
            continue;

        const bool exact = xr_strlen(rewrite.source) == length &&
            line.compare(first, length, rewrite.source) == 0;
        if (exact || glsl_line_matches_ignoring_space(line, first, last, rewrite.source))
        {
            line.replace(first, length, rewrite.replacement);
            return true;
        }
    }
    return false;
}

bool get_android_varying_type(const xr_string& type, xr_string& physicalType, pcstr& swizzle)
{
    xr_string family;
    int width = 0;

    if (type == "float" || type == "half")
    {
        family = "float";
        width = 1;
    }
    else if ((type.size() == 6 && type.compare(0, 5, "float") == 0) ||
        (type.size() == 5 && type.compare(0, 4, "half") == 0))
    {
        family = "float";
        width = type.back() - '0';
    }
    else if (type.size() == 4 && type.compare(0, 3, "vec") == 0)
    {
        family = "float";
        width = type.back() - '0';
    }
    else if (type == "int")
    {
        family = "int";
        width = 1;
    }
    else if (type.size() == 4 && type.compare(0, 3, "int") == 0)
    {
        family = "int";
        width = type.back() - '0';
    }
    else if (type.size() == 5 && type.compare(0, 4, "ivec") == 0)
    {
        family = "int";
        width = type.back() - '0';
    }
    else if (type == "uint")
    {
        family = "uint";
        width = 1;
    }
    else if (type.size() == 5 && type.compare(0, 4, "uint") == 0)
    {
        family = "uint";
        width = type.back() - '0';
    }
    else if (type.size() == 5 && type.compare(0, 4, "uvec") == 0)
    {
        family = "uint";
        width = type.back() - '0';
    }

    if (width < 1 || width > 4)
        return false;

    physicalType = family == "float" ? "vec4" : family == "int" ? "ivec4" : "uvec4";
    static constexpr cpcstr Swizzles[] = { nullptr, ".x", ".xy", ".xyz", "" };
    swizzle = Swizzles[width];
    return true;
}

bool add_android_fragment_output_location(xr_string& line)
{
    if (line.find("layout") != xr_string::npos)
        return false;

    size_t position = 0;
    size_t begin = 0;
    size_t end = 0;
    if (!read_glsl_identifier(line, position, begin, end) || line.compare(begin, end - begin, "out") != 0)
        return false;
    if (!read_glsl_identifier(line, position, begin, end))
        return false;
    const xr_string type = line.substr(begin, end - begin);
    if (type != "vec4" && type != "float4")
        return false;
    if (!read_glsl_identifier(line, position, begin, end))
        return false;

    const xr_string name = line.substr(begin, end - begin);
    constexpr cpcstr Target = "SV_Target";
    if (name.compare(0, xr_strlen(Target), Target) != 0)
        return false;

    const xr_string suffix = name.substr(xr_strlen(Target));
    u32 location = 0;
    if (!suffix.empty())
    {
        for (const char character : suffix)
        {
            if (character < '0' || character > '9')
                return false;
            location = location * 10 + u32(character - '0');
        }
    }

    const size_t indentation = skip_glsl_space(line, 0);
    string64 layout;
    xr_sprintf(layout, "layout(location = %u) ", location);
    line.insert(indentation, layout);
    return true;
}

bool normalize_android_stage_varying(xr_string& line, char stage)
{
    const cpcstr wantedStorage = stage == 'v' ? "out" : stage == 'p' ? "in" : nullptr;
    if (!wantedStorage)
        return false;

    const size_t layoutPosition = line.find("layout");
    if (layoutPosition == xr_string::npos)
        return false;
    const size_t open = line.find('(', layoutPosition + 6);
    const size_t close = open == xr_string::npos ? xr_string::npos : line.find(')', open + 1);
    const size_t location = line.find("location", open + 1);
    if (close == xr_string::npos || location > close)
        return false;

    const size_t equals = line.find('=', location + xr_strlen("location"));
    if (equals == xr_string::npos || equals > close)
        return false;
    xr_string locationKey;
    for (size_t cursor = equals + 1; cursor < close && line[cursor] != ','; ++cursor)
    {
        const char value = line[cursor];
        if ((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') ||
            (value >= '0' && value <= '9') || value == '_')
            locationKey += value;
        else if (!is_glsl_space(value))
            locationKey += '_';
    }
    if (locationKey.empty())
        return false;

    size_t position = close + 1;
    size_t begin = 0;
    size_t end = 0;
    xr_string storage;
    while (read_glsl_identifier(line, position, begin, end))
    {
        const xr_string token = line.substr(begin, end - begin);
        if (token == "in" || token == "out")
        {
            storage = token;
            break;
        }
        if (!is_glsl_qualifier(token))
            return false;
    }
    if (storage != wantedStorage)
        return false;

    size_t typeBegin = 0;
    size_t typeEnd = 0;
    if (!read_glsl_identifier(line, position, typeBegin, typeEnd))
        return false;
    xr_string type = line.substr(typeBegin, typeEnd - typeBegin);
    if (type == "lowp" || type == "mediump" || type == "highp")
    {
        if (!read_glsl_identifier(line, position, typeBegin, typeEnd))
            return false;
        type = line.substr(typeBegin, typeEnd - typeBegin);
    }

    size_t nameBegin = 0;
    size_t nameEnd = 0;
    if (!read_glsl_identifier(line, position, nameBegin, nameEnd))
        return false;
    const xr_string name = line.substr(nameBegin, nameEnd - nameBegin);
    if (name.compare(0, 3, "gl_") == 0 ||
        name.compare(0, xr_strlen(AndroidVaryingPrefix), AndroidVaryingPrefix) == 0)
        return false;

    xr_string physicalType;
    pcstr swizzle = nullptr;
    if (!get_android_varying_type(type, physicalType, swizzle))
        return false;

    // Some mobile linkers still require matching varying identifiers even
    // when both stages use explicit locations. Derive the physical name from
    // the location expression (for example TEXCOORD1) so independently named
    // PC shader inputs/outputs link without any per-resource rename rule.
    const xr_string physicalName = xr_string(AndroidVaryingPrefix) + locationKey;
    line.replace(nameBegin, nameEnd - nameBegin, physicalName);
    line.replace(typeBegin, typeEnd - typeBegin, physicalType);
    line += "\n#define ";
    line += name;
    line += " ";
    line += physicalName;
    line += swizzle;
    return true;
}

xr_string transform_android_glsl_source(cpcstr source, size_t length, char stage, cpcstr sourceName)
{
    xr_string transformed;
    transformed.reserve(length + 256);
    const xr_string sourcePath = normalize_android_shader_path(sourceName);

    size_t offset = 0;
    while (offset < length)
    {
        const cpcstr newline = static_cast<cpcstr>(memchr(source + offset, '\n', length - offset));
        const size_t lineEnd = newline ? size_t(newline - source) : length;
        xr_string line(source + offset, lineEnd - offset);

        const size_t first = skip_glsl_space(line, 0);
        size_t last = line.size();
        while (last > first && is_glsl_space(line[last - 1]))
            --last;
        const xr_string trimmed = line.substr(first, last - first);

        // The PC renderer's HLSL-like GLSL accepts a handful of implicit
        // conversions that strict GLSL ES drivers reject. Keep the original
        // resource tree byte-for-byte intact and normalize only the source
        // copy passed to the Android compiler.
        apply_android_line_rewrite(line, first, last, sourcePath);

        // These are built-ins in GLSL ES.  Redeclaring them is accepted by
        // desktop GLSL but rejected by Adreno as a reserved-name violation.
        if (stage == 'p' && (trimmed == "in vec4 gl_FragCoord;" || trimmed == "in int gl_SampleID;"))
            line.assign(line.size(), ' ');
        else
        {
            if (stage == 'p')
                add_android_fragment_output_location(line);
            normalize_android_stage_varying(line, stage);
        }

        transformed += line;
        transformed += '\n';
        offset = newline ? lineEnd + 1 : length;
    }

    return transformed;
}
} // namespace
#endif

void CRender::addShaderOption(const char* name, const char* value)
{
    m_ShaderOptions += "#define ";
    m_ShaderOptions += name;
    m_ShaderOptions += " ";
    m_ShaderOptions += value;
    m_ShaderOptions += "\n";
}

template <typename T>
static GLuint create_shader(pcstr* buffer, size_t const buffer_size, cpcstr filename,
    T*& result, const GLenum* format)
{
    auto [type, output] = ShaderTypeTraits<T>::CreateHWShader(buffer, buffer_size, result->sh, format, filename);

    // Parse constant, texture, sampler binding
    if (output != 0)
    {
        result->sh = output;
        if (type == 'p')
            result->constants.parse(&output, ShaderTypeTraits<T>::GetShaderDest());
    }
    return output;
}

static GLuint create_shader(cpcstr pTarget, pcstr* buffer, size_t const buffer_size,
    cpcstr filename, void*& result, const GLenum* format)
{
    switch (pTarget[0])
    {
    case 'p':
        return create_shader(buffer, buffer_size, filename, (SPS*&)result, format);
    case 'v':
        return create_shader(buffer, buffer_size, filename, (SVS*&)result, format);
    case 'g':
        return create_shader(buffer, buffer_size, filename, (SGS*&)result, format);
    case 'c':
        return create_shader(buffer, buffer_size, filename, (SCS*&)result, format);
    case 'h':
        return create_shader(buffer, buffer_size, filename, (SHS*&)result, format);
    case 'd':
        return create_shader(buffer, buffer_size, filename, (SDS*&)result, format);
    default:
        NODEFAULT;
        return 0;
    }
}

class shader_name_holder
{
    size_t pos{};
    string_path name;

public:
    void append(cpcstr string)
    {
        const size_t size = xr_strlen(string);
        for (size_t i = 0; i < size; ++i)
        {
            name[pos] = string[i];
            ++pos;
        }
    }

    void append(u32 value)
    {
        name[pos] = '0' + char(value); // NOLINT
        ++pos;
    }

    void finish()
    {
        name[pos] = '\0';
    }

    pcstr c_str() const { return name; }
};

class shader_options_holder
{
    size_t pos{};
    string512 m_options[128];

public:
    void add(cpcstr string)
    {
        strconcat(m_options[pos++], string, "\n");
    }

    void add(cpcstr name, cpcstr value)
    {
        // It's important to have postfix increment!
        strconcat(m_options[pos++], "#define ", name, "\t", value, "\n");
    }

    void finish()
    {
        m_options[pos][0] = '\0';
    }

    [[nodiscard]] size_t size() const { return pos; }
    string512& operator[](size_t idx) { return m_options[idx]; }
};

class shader_sources_manager
{
    pcstr* m_sources{};
    size_t m_sources_lines{};
    xr_vector<pstr> m_source, m_includes;

public:
    ~shader_sources_manager()
    {
        // Free string resources
        xr_free(m_sources);
        for (pstr include : m_includes)
            xr_free(include);
        m_source.clear();
        m_includes.clear();
    }

    [[nodiscard]] auto get() const { return m_sources; }
    [[nodiscard]] auto length() const { return m_sources_lines; }

    void compile(IReader* file, shader_options_holder& options, cpcstr target, cpcstr sourceName)
    {
        load_includes(file, target[0], sourceName);
        apply_options(options);
    }

private:
    // TODO: OGL: make ignore commented includes
    void load_includes(IReader* file, char stage, cpcstr sourceName)
    {
        cpcstr sourceData = static_cast<cpcstr>(file->pointer());
        const size_t dataLength = file->length();

        // Copy source file data into a null-terminated buffer
#if defined(XR_PLATFORM_ANDROID)
        const xr_string transformed = transform_android_glsl_source(sourceData, dataLength, stage, sourceName);
        cpstr data = xr_alloc<char>(transformed.size() + 1);
        CopyMemory(data, transformed.c_str(), transformed.size() + 1);
#else
        cpstr data = xr_alloc<char>(dataLength + 2);
        CopyMemory(data, sourceData, dataLength);
        data[dataLength] = '\n';
        data[dataLength + 1] = '\0';
#endif
        m_includes.push_back(data);
        m_source.push_back(data);

        string_path path;
        pstr str = data;
        while (strstr(str, "#include") != nullptr)
        {
            // Get filename of include directive
            str = strstr(str, "#include"); // Find the include directive
            char* fn = strchr(str, '"') + 1; // Get filename, skip quotation
            *str = '\0'; // Terminate previous source
            str = strchr(fn, '"'); // Get end of filename path
            *str = '\0'; // Terminate filename path

            // Create path to included shader
            strconcat(path, RImplementation.getShaderPath(), fn);
            FS.update_path(path, _game_shaders_, path);
            while (cpstr sep = strchr(path, '/'))
                *sep = '\\';

            // Open and read file, recursively load includes
            IReader* R = FS.r_open(path);
            R_ASSERT2(R, path);
            load_includes(R, stage, fn);
            FS.r_close(R);

            // Add next source, skip quotation
            ++str;
            m_source.push_back(str);
        }
    }

    void apply_options(shader_options_holder& options)
    {
        // Compile sources list
        m_sources_lines = m_source.size() + options.size();
        m_sources = xr_alloc<pcstr>(m_sources_lines);

        // Make define lines
        for (size_t i = 0; i < options.size(); ++i)
        {
            m_sources[i] = options[i];
        }
        CopyMemory(m_sources + options.size(), m_source.data(), m_source.size() * sizeof(pstr));
    }
};

HRESULT CRender::shader_compile(pcstr name, IReader* fs, pcstr pFunctionName,
    pcstr pTarget, u32 Flags, void*& result)
{
    shader_options_holder options;
    shader_name_holder sh_name;

    // Don't move these variables to lower scope!
    string64 c_name;
    string32 c_smapsize;
    string32 c_gloss;
    string32 c_sun_shafts;
    string32 c_ssao;
    string32 c_sun_quality;
    string32 c_isample;
    string32 c_water_reflection;

    // TODO: OGL: Implement these parameters.
    UNUSED(pFunctionName);
    UNUSED(Flags);

    // options:
    const auto appendShaderOption = [&](u32 option, cpcstr macro, cpcstr value)
    {
        if (option)
            options.add(macro, value);

        sh_name.append(option);
    };

#if defined(XR_PLATFORM_ANDROID)
    // Prefer GLSL ES 3.20 when the context exposes it and retain 3.10 for
    // GLES 3.1 devices. The source compatibility layer below is still needed:
    // mixed integer/float operators remain stricter than desktop GLSL.
    options.add(GLAD_GL_ES_VERSION_3_2 ? "#version 320 es" : "#version 310 es");

    // Some GLES 3.1 drivers expose the shader I/O blocks as an extension even
    // though the feature is also available in the 3.10 language version.
    if (GLAD_GL_EXT_shader_io_blocks)
        options.add("#extension GL_EXT_shader_io_blocks : enable");
    else if (GLAD_GL_OES_shader_io_blocks)
        options.add("#extension GL_OES_shader_io_blocks : enable");

    // v_volumetric.h redeclares gl_ClipDistance in the same built-in block.
    if (GLAD_GL_EXT_clip_cull_distance)
        options.add("#extension GL_EXT_clip_cull_distance : enable");
    if (GLAD_GL_EXT_gpu_shader5)
        options.add("#extension GL_EXT_gpu_shader5 : enable");
    else if (GLAD_GL_OES_gpu_shader5)
        options.add("#extension GL_OES_gpu_shader5 : enable");
    if (GLAD_GL_EXT_shader_implicit_conversions)
        options.add("#extension GL_EXT_shader_implicit_conversions : enable");
    options.add("precision highp float;");
    options.add("precision highp int;");
    // GLSL ES 3.10 has no implicit precision for the sampler types that
    // OpenXRay's deferred and MSAA shader headers use.  Desktop GLSL accepts
    // these declarations without a precision qualifier, while Adreno rejects
    // them and the engine later reports the misleading video-card message.
    options.add("precision lowp sampler3D;");
    options.add("precision lowp sampler2DMS;");
    options.add("precision lowp sampler2DShadow;");
#else
    options.add("#version 410");
    options.add("#extension GL_ARB_separate_shader_objects : enable");
#endif

#ifdef DEBUG
#if !defined(XR_PLATFORM_ANDROID)
    options.add("#pragma optimize (off)");
#endif
    sh_name.append(0u);
#else
#if !defined(XR_PLATFORM_ANDROID)
    options.add("#pragma optimize (on)");
#endif
    sh_name.append(1u);
#endif

    xr_sprintf(c_name, "// %s.%s", name, pTarget);
    options.add(c_name);

    // Shadow map size
    {
        xr_itoa(m_SMAPSize, c_smapsize, 10);
        options.add("SMAP_size", c_smapsize);
        sh_name.append(c_smapsize);
    }

    // FP16 Filter
    appendShaderOption(o.fp16_filter, "FP16_FILTER", "1");

    // FP16 Blend
    appendShaderOption(o.fp16_blend, "FP16_BLEND", "1");

    // HW smap
    appendShaderOption(o.HW_smap, "USE_HWSMAP", "1");

    // HW smap PCF
    appendShaderOption(o.HW_smap_PCF, "USE_HWSMAP_PCF", "1");

    // Fetch4
    appendShaderOption(o.HW_smap_FETCH4, "USE_FETCH4", "1");

    // SJitter
    appendShaderOption(o.sjitter, "USE_SJITTER", "1");

    // Branching
    appendShaderOption(HW.Caps.raster_major >= 3, "USE_BRANCHING", "1");

    // Vertex texture fetch
    appendShaderOption(HW.Caps.geometry.bVTF, "USE_VTF", "1");

    // Tshadows
    appendShaderOption(o.Tshadows, "USE_TSHADOWS", "1");

    // Motion blur
    appendShaderOption(o.mblur, "USE_MBLUR", "1");

    // Sun filter
    appendShaderOption(o.sunfilter, "USE_SUNFILTER", "1");

    // Static sun on R2 and higher
    appendShaderOption(o.sunstatic, "USE_R2_STATIC_SUN", "1");

    // Force gloss
    {
        xr_sprintf(c_gloss, "%f", o.forcegloss_v);
        appendShaderOption(o.forcegloss, "FORCE_GLOSS", c_gloss);
    }

    // Force skinw
    appendShaderOption(o.forceskinw, "SKIN_COLOR", "1");

    // SSAO Blur
    appendShaderOption(o.ssao_blur_on, "USE_SSAO_BLUR", "1");

    // SSAO HDAO
    if (o.ssao_hdao)
    {
        options.add("HDAO", "1");
        sh_name.append(static_cast<u32>(1)); // HDAO on
        sh_name.append(static_cast<u32>(0)); // HBAO off
        sh_name.append(static_cast<u32>(0)); // HBAO vectorized off
    }
    else // SSAO HBAO
    {
        sh_name.append(static_cast<u32>(0)); // HDAO off
        sh_name.append(o.ssao_hbao);         // HBAO on/off

        appendShaderOption(o.ssao_hbao, "USE_HBAO", "1");
        appendShaderOption(o.hbao_vectorized, "VECTORIZED_CODE", "1");
    }

    if (o.ssao_opt_data)
    {
        if (o.ssao_half_data)
            options.add("SSAO_OPT_DATA", "2");
        else
            options.add("SSAO_OPT_DATA", "1");
    }
    sh_name.append(o.ssao_opt_data ? (o.ssao_half_data ? u32(2) : u32(1)) : u32(0));

    // skinning
    // SKIN_NONE
    appendShaderOption(m_skinning < 0, "SKIN_NONE", "1");

    // SKIN_0
    appendShaderOption(0 == m_skinning, "SKIN_0", "1");

    // SKIN_1
    appendShaderOption(1 == m_skinning, "SKIN_1", "1");

    // SKIN_2
    appendShaderOption(2 == m_skinning, "SKIN_2", "1");

    // SKIN_3
    appendShaderOption(3 == m_skinning, "SKIN_3", "1");

    // SKIN_4
    appendShaderOption(4 == m_skinning, "SKIN_4", "1");

    //	Igor: need restart options
    // Soft water
    {
        const bool softWater = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_WATER);
        appendShaderOption(softWater, "USE_SOFT_WATER", "1");
    }

    // Water reflections
    if (RImplementation.o.advancedpp && ps_r_water_reflection)
    {
        xr_sprintf(c_water_reflection, "%d", ps_r_water_reflection);
        options.add("SSR_QUALITY", c_water_reflection);
        sh_name.append(ps_r_water_reflection);
        const bool sshHalfDepth = ps_r2_ls_flags_ext.test(R3FLAGEXT_SSR_HALF_DEPTH);
        appendShaderOption(sshHalfDepth, "SSR_HALF_DEPTH", "1");
        const bool ssrJitter = ps_r2_ls_flags_ext.test(R3FLAGEXT_SSR_JITTER);
        appendShaderOption(ssrJitter, "SSR_JITTER", "1");
    }
    else
    {
#if defined(XR_PLATFORM_ANDROID)
        // Some GLES preprocessors reject an undefined identifier used in a
        // numeric #if expression.  A zero value preserves the disabled path.
        options.add("SSR_QUALITY", "0");
#endif
        sh_name.append(static_cast<u32>(0));
    }

    // Soft particles
    {
        const bool useSoftParticles = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_PARTICLES);
        appendShaderOption(useSoftParticles, "USE_SOFT_PARTICLES", "1");
    }

    // Depth of field
    {
        const bool dof = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DOF);
        appendShaderOption(dof, "USE_DOF", "1");
    }

    // Sun shafts
    if (RImplementation.o.advancedpp && ps_r_sun_shafts)
    {
        xr_sprintf(c_sun_shafts, "%d", ps_r_sun_shafts);
        options.add("SUN_SHAFTS_QUALITY", c_sun_shafts);
        sh_name.append(ps_r_sun_shafts);
    }
    else
    {
        // Do not define SUN_SHAFTS_QUALITY to zero. The volumetric shader uses
        // #ifdef to remove the ray-march entirely; defining zero entered that
        // branch without defining RAY_SAMPLES and broke shader compilation.
        sh_name.append(static_cast<u32>(0));
    }

    if (RImplementation.o.advancedpp && ps_r_ssao)
    {
        xr_sprintf(c_ssao, "%d", ps_r_ssao);
        options.add("SSAO_QUALITY", c_ssao);
        sh_name.append(ps_r_ssao);
    }
    else
    {
#if defined(XR_PLATFORM_ANDROID)
        options.add("SSAO_QUALITY", "0");
#endif
        sh_name.append(static_cast<u32>(0));
    }

    // Sun quality
    if (RImplementation.o.advancedpp && ps_r_sun_quality)
    {
        xr_sprintf(c_sun_quality, "%d", ps_r_sun_quality);
        options.add("SUN_QUALITY", c_sun_quality);
        sh_name.append(ps_r_sun_quality);
    }
    else
    {
#if defined(XR_PLATFORM_ANDROID)
        options.add("SUN_QUALITY", "0");
#endif
        sh_name.append(static_cast<u32>(0));
    }

    // Steep parallax
    {
        const bool steepParallax = RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_STEEP_PARALLAX);
        appendShaderOption(steepParallax, "ALLOW_STEEPPARALLAX", "1");
    }

    // Geometry buffer optimization
    appendShaderOption(o.gbuffer_opt, "GBUFFER_OPTIMIZATION", "1");

    // Shader Model 4.1
#ifndef XR_PLATFORM_APPLE
    appendShaderOption(o.dx11_sm4_1, "SM_4_1", "1");
    // Despite the fact that glsl 4.1 is claimed to be supported on macOS,
    // the issue is that gatherTextureOffset requires compile-time constant offset argument.
    // So it is more handy to disable its use for mac as for now.
#endif

    // Minmax SM
    appendShaderOption(o.minmax_sm, "USE_MINMAX_SM", "1");

    // Shadow of Chernobyl compatibility
    appendShaderOption(ShadowOfChernobylMode, "USE_SHOC_RESOURCES", "1");

    // add a #define for DX10_1 MSAA support
    if (o.msaa)
    {
        appendShaderOption(o.msaa, "USE_MSAA", "1");

        {
            static char samples[2];
            samples[0] = char(o.msaa_samples) + '0';
            samples[1] = 0;
            appendShaderOption(o.msaa_samples, "MSAA_SAMPLES", samples);
        }

        xr_sprintf(c_isample, "uint(%d)", m_MSAASample);
        options.add("ISAMPLE", c_isample);
        sh_name.append(static_cast<u32>(0));

        appendShaderOption(o.msaa_opt, "MSAA_OPTIMIZATION", "1");

        switch (o.msaa_alphatest)
        {
        case MSAA_ATEST_DX10_0_ATOC:
            options.add("MSAA_ALPHATEST_DX10_0_ATOC", "1");

            sh_name.append(static_cast<u32>(1)); // DX10_0_ATOC   on
            sh_name.append(static_cast<u32>(0)); // DX10_1_ATOC   off
            sh_name.append(static_cast<u32>(0)); // DX10_1_NATIVE off
            break;
        case MSAA_ATEST_DX10_1_ATOC:
            options.add("MSAA_ALPHATEST_DX10_1_ATOC", "1");

            sh_name.append(static_cast<u32>(0)); // DX10_0_ATOC   off
            sh_name.append(static_cast<u32>(1)); // DX10_1_ATOC   on
            sh_name.append(static_cast<u32>(0)); // DX10_1_NATIVE off
            break;
        case MSAA_ATEST_DX10_1_NATIVE:
            options.add("MSAA_ALPHATEST_DX10_1", "1");

            sh_name.append(static_cast<u32>(0)); // DX10_0_ATOC   off
            sh_name.append(static_cast<u32>(0)); // DX10_1_ATOC   off
            sh_name.append(static_cast<u32>(1)); // DX10_1_NATIVE on
            break;
        default:
            sh_name.append(static_cast<u32>(0)); // DX10_0_ATOC   off
            sh_name.append(static_cast<u32>(0)); // DX10_1_ATOC   off
            sh_name.append(static_cast<u32>(0)); // DX10_1_NATIVE off
        }
    }
    else
    {
        sh_name.append(static_cast<u32>(0)); // MSAA off
        sh_name.append(static_cast<u32>(0)); // No MSAA samples
        sh_name.append(static_cast<u32>(0)); // No MSAA ISAMPLE
        sh_name.append(static_cast<u32>(0)); // No MSAA optimization
        sh_name.append(static_cast<u32>(0)); // DX10_0_ATOC   off
        sh_name.append(static_cast<u32>(0)); // DX10_1_ATOC   off
        sh_name.append(static_cast<u32>(0)); // DX10_1_NATIVE off
    }

#if defined(XR_PLATFORM_ANDROID)
    // This suffix is part of the on-disk shader cache key. Bump it whenever
    // the Android source compatibility pass changes: the source-file CRC
    // alone cannot detect translator changes and would otherwise mix old
    // program binaries with newly translated stages after an APK update.
    sh_name.append("glescompat4");

    // Vertex declarations use different physical widths for the same logical
    // attributes in the skinning variants. The original resources rely on
    // desktop GLSL's permissive vector narrowing; make it explicit after the
    // SKIN_* option has been selected, without changing game or mod files.
    options.add(
        "#if defined(SKIN_NONE) || defined(SKIN_0)\n"
        "#define skin_input_normal(value) value.xyz\n"
        "#else\n"
        "#define skin_input_normal(value) value\n"
        "#endif\n"
        "#if defined(SKIN_NONE) || defined(SKIN_0) || defined(SKIN_1) || defined(SKIN_2)\n"
        "#define skin_input_tangent(value) value.xyz\n"
        "#else\n"
        "#define skin_input_tangent(value) value\n"
        "#endif\n"
        "#if defined(SKIN_2) || defined(SKIN_3)\n"
        "#define skin_input_tc(value) value\n"
        "#else\n"
        "#define skin_input_tc(value) value.xy\n"
        "#endif");
#endif

    // finish
    options.finish();
    sh_name.finish();

    char extension[3];
    strncpy_s(extension, pTarget, 2);

    u32 fileCrc = 0;
    string_path filename, full_path{};
    strconcat(sizeof(filename), filename, "gl" DELIMITER, name, ".", extension, DELIMITER, sh_name.c_str());
    if (GLAD_GL_ARB_get_program_binary && GLAD_GL_ARB_separate_shader_objects)
    {
        string_path file;
        strconcat(sizeof(file), file, "shaders_cache_oxr" DELIMITER, filename);
        FS.update_path(full_path, "$app_data_root$", file);

        string_path shadersFolder;
        FS.update_path(shadersFolder, "$game_shaders$", RImplementation.getShaderPath());

        getFileCrc32(fs, shadersFolder, fileCrc);
        fs->seek(0);
    }

    GLuint program = 0;
    if (GLAD_GL_ARB_get_program_binary && GLAD_GL_ARB_separate_shader_objects && FS.exist(full_path))
    {
        IReader* file = FS.r_open(full_path);
        if (file->length() > 8)
        {
            xr_string renderer, glVer, shadingVer;
            file->r_string(renderer);
            file->r_string(glVer);
            file->r_string(shadingVer);

            if (0 == xr_strcmp(renderer.c_str(), HW.AdapterName) &&
                0 == xr_strcmp(glVer.c_str(), HW.OpenGLVersionString) &&
                0 == xr_strcmp(shadingVer.c_str(), HW.ShadingVersion))
            {
                const GLenum binaryFormat = file->r_u32();

                const u32 savedFileCrc = file->r_u32();
                if (savedFileCrc == fileCrc)
                {
                    const u32 savedBytecodeCrc = file->r_u32();
                    const u32 bytecodeCrc = crc32(file->pointer(), file->elapsed());
                    if (bytecodeCrc == savedBytecodeCrc)
                    {
#ifdef DEBUG
                        Log("* Loading shader:", full_path);
#endif
                        program = create_shader(pTarget, (pcstr*)file->pointer(), file->elapsed(), filename, result, &binaryFormat);
                    }
                }
            }
        }
        file->close();
    }

    // Failed to use cached shader, then:
    if (!program)
    {
#ifdef DEBUG
        Log("- Compile shader:", filename);
#endif
        // Compile sources list
        shader_sources_manager sources;
        string_path sourceFileName;
        strconcat(sizeof(sourceFileName), sourceFileName, name, ".", extension);
        sources.compile(fs, options, pTarget, sourceFileName);

        // Compile the shader from sources
        program = create_shader(pTarget, sources.get(), sources.length(), filename, result, nullptr);

        if (GLAD_GL_ARB_get_program_binary && GLAD_GL_ARB_separate_shader_objects && program)
        {
            GLint binaryLength{};
            GLenum binaryFormat{};
            CHK_GL(glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binaryLength));

            GLvoid* binary = binaryLength ? xr_malloc(binaryLength) : nullptr;
            if (binary)
            {
                CHK_GL(glGetProgramBinary(program, binaryLength, nullptr, &binaryFormat, binary));
                IWriter* file = FS.w_open(full_path);

                file->w_string(HW.AdapterName);
                file->w_string(HW.OpenGLVersionString);
                file->w_string(HW.ShadingVersion);

                file->w_u32(binaryFormat);
                file->w_u32(fileCrc);

                const u32 bytecodeCrc = crc32(binary, binaryLength);
                file->w_u32(bytecodeCrc); // Do not write anything below this line, take a look at reading (crc32)

                file->w(binary, binaryLength);
                FS.w_close(file);
                xr_free(binary);
            }
        }
    }

    if (program)
        return S_OK;

    return E_FAIL;
}
} // namespace xray::render::RENDER_NAMESPACE
