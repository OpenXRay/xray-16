#include "src/xrCore/Text/LegacyFilename.h"
#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
int main()
{
    using namespace xray::text;
    const std::string utf8 = u8"Скадовск Ёж 42.scop";
    const std::string cp = "\xd1\xea\xe0\xe4\xee\xe2\xf1\xea \xa8\xe6 42.scop";
    assert(filename_from_utf8(utf8) == cp);
    assert(filename_to_utf8(cp) == utf8);
    assert(filename_to_utf8(utf8) == utf8);
    assert(filename_from_utf8("slot_42.scop") == "slot_42.scop");
    assert(filename_to_utf8(u8"保存.scop") == u8"保存.scop");
    assert(!valid_utf8("\xc0\xaf"));
    assert(!valid_utf8("\xed\xa0\x80"));
    assert(!valid_utf8("\xf4\x90\x80\x80"));
    auto dir = std::filesystem::temp_directory_path() / "xray-filename-regression";
    std::filesystem::create_directories(dir);
    auto file = dir / filename_to_utf8(cp);
    { std::ofstream out(file); out << "save"; }
    std::string listed = filename_from_utf8(file.filename().string());
    // ui_load_dialog uses a wildcard for the list and an exact filename
    // mask after selection. Both must match the UTF-8 filesystem index.
    assert(file.filename().string() != listed);
    const std::string guillemets = u8"u0_a694 - прибытие на «скадовск».scop";
    std::array masks{listed, filename_from_utf8(guillemets), std::string("*.scop"), std::string("quicksave.sav")};
    encode_save_file_masks(masks);
    assert(masks[0] == file.filename().string());
    assert(masks[1] == guillemets);
    assert(masks[2] == "*.scop");
    assert(masks[3] == "quicksave.sav");
    std::ifstream in(dir / filename_to_utf8(listed));
    std::string contents;
    in >> contents;
    assert(contents == "save");
    in.close();
    assert(std::filesystem::remove(dir / filename_to_utf8(listed)));
    std::filesystem::remove(dir);
}
