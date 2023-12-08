#include <doctest/doctest.h>

#include <Common/Platform.hpp>
#include <xrCore/xrCore.h>
#include <xrCore/xr_types.h>

#include <xrCore/xr_ini.h>

CInifile read_from_string(pcstr str, CInifile::allow_include_func_t allow_include = nullptr)
{
    IReader reader = IReader(const_cast<pstr>(str), xr_strlen(str));
    return CInifile(&reader, "test.ini", allow_include);
}

TEST_CASE("parse empty file")
{
    CInifile ini = read_from_string("");

    CHECK_EQ(ini.section_count(), 0);
}

TEST_CASE("parse empty section")
{
    CInifile ini = read_from_string("[a]");

    CHECK_EQ(ini.section_count(), 1);
    CHECK_UNARY(ini.section_exist("a"));
}

TEST_CASE("parse simple section")
{
    CInifile ini = read_from_string(
        R"ini(
[a]
key = value
)ini");

    CHECK_UNARY(ini.section_exist("a"));
    CHECK_UNARY(ini.line_exist("a", "key"));
    CHECK_EQ(ini.read<pcstr>("a", "key"), "value");
}

TEST_CASE("parse integer value")
{
    CInifile ini = read_from_string(
        R"ini(
[a]
key = 123
)ini");

    CHECK_UNARY(ini.section_exist("a"));
    CHECK_UNARY(ini.line_exist("a", "key"));
    CHECK_EQ(ini.read<u32>("a", "key"), 123);
}

TEST_CASE("Parse float value")
{
    CInifile ini = read_from_string(
        R"ini(
[a]
key = 123.456
)ini");

    CHECK_UNARY(ini.section_exist("a"));
    CHECK_UNARY(ini.line_exist("a", "key"));
    CHECK_EQ(ini.read<f32>("a", "key"), 123.456f);
}

TEST_CASE("Parse quoted value")
{
    CInifile ini = read_from_string(
        R"ini(
[a]
key = "value"
)ini");

    CHECK_UNARY(ini.section_exist("a"));
    CHECK_UNARY(ini.line_exist("a", "key"));
    CHECK_EQ(ini.read<pcstr>("a", "key"), "\"value\"");
}

TEST_CASE("Parse multiline value")
{
    CInifile ini = read_from_string(
        R"ini(
[a]
key = "multiline
value"
)ini");

    CHECK_UNARY(ini.section_exist("a"));
    CHECK_UNARY(ini.line_exist("a", "key"));
    CHECK_EQ(ini.read<pcstr>("a", "key"), "\"multiline\r\nvalue\"");
}
