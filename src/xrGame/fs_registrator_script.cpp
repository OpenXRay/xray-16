#include "pch_script.h"
#include "xrCore/LocatorAPI.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

LPCSTR get_file_age_str(CLocatorAPI* fs, LPCSTR nm);
CLocatorAPI* getFS() { return &FS; }
LPCSTR update_path_script(CLocatorAPI* fs, LPCSTR initial, LPCSTR src)
{
    string_path temp;
    shared_str temp_2;
    fs->update_path(temp, initial, src);
    temp_2 = temp;
    return *temp_2;
}

//Alundaio: Set flag to rescan all files in path
void rescan_path_script(CLocatorAPI* fs, pcstr initial)
{
    fs->get_path(initial)->m_Flags.set(FS_Path::flNeedRescan, true);
}
//-Alundaio

class FS_file_list
{
    xr_vector<pstr>* m_p;

public:
    FS_file_list(xr_vector<pstr>* p) : m_p(p) {}
    u32 Size() { return m_p->size(); }
    LPCSTR GetAt(u32 idx) { return m_p->at(idx); }
    void Free() { FS.file_list_close(m_p); };
};

struct FS_item
{
    string_path name;
    u32 size;
    u32 modif;
    string256 buff;

    LPCSTR NameShort() { return name; }
    LPCSTR NameFull() { return name; }
    u32 Size() { return size; }
    LPCSTR Modif()
    {
        struct tm* newtime;
        time_t t = modif;
        newtime = localtime(&t);
        xr_strcpy(buff, asctime(newtime));
        return buff;
    }

    LPCSTR ModifDigitOnly()
    {
        struct tm* newtime;
        time_t t = modif;
        newtime = localtime(&t);
        xr_sprintf(buff, "%02d/%02d/%4d %02d:%02d", newtime->tm_mday, newtime->tm_mon + 1, newtime->tm_year + 1900,
            newtime->tm_hour, newtime->tm_min);
        return buff;
    }
};

template <bool b>
bool sizeSorter(const FS_item& itm1, const FS_item& itm2)
{
    if (b)
        return (itm1.size < itm2.size);
    return (itm2.size < itm1.size);
}
template <bool b>
bool modifSorter(const FS_item& itm1, const FS_item& itm2)
{
    if (b)
        return (itm1.modif < itm2.modif);
    return (itm2.modif < itm1.modif);
}
template <bool b>
bool nameSorter(const FS_item& itm1, const FS_item& itm2)
{
    if (b)
        return (xr_strcmp(itm1.name, itm2.name) < 0);
    return (xr_strcmp(itm2.name, itm1.name) < 0);
}

class FS_file_list_ex
{
    xr_vector<FS_item> m_file_items;

public:
    enum
    {
        eSortByNameUp = 0,
        eSortByNameDown,
        eSortBySizeUp,
        eSortBySizeDown,
        eSortByModifUp,
        eSortByModifDown
    };
    FS_file_list_ex(LPCSTR path, u32 flags, LPCSTR mask);

    u32 Size() { return m_file_items.size(); }
    FS_item GetAt(u32 idx) { return m_file_items[idx]; }
    void Sort(u32 flags);
};

FS_file_list_ex::FS_file_list_ex(LPCSTR path, u32 flags, LPCSTR mask)
{
    FS_Path* P = FS.get_path(path);
    P->m_Flags.set(FS_Path::flNeedRescan, TRUE);
    FS.m_Flags.set(CLocatorAPI::flNeedCheck, TRUE);
    FS.rescan_pathes();

    FS_FileSet files;
    FS.file_list(files, path, flags, mask);

    for (auto it = files.begin(); it != files.end(); ++it)
    {
        m_file_items.push_back(FS_item());
        FS_item& itm = m_file_items.back();
        ZeroMemory(itm.name, sizeof(itm.name));
        xr_strcat(itm.name, it->name.c_str());
        itm.modif = (u32)it->time_write;
        itm.size = it->size;
    }

    FS.m_Flags.set(CLocatorAPI::flNeedCheck, FALSE);
}

void FS_file_list_ex::Sort(u32 flags)
{
    if (flags == eSortByNameUp)
        std::sort(m_file_items.begin(), m_file_items.end(), nameSorter<true>);
    else if (flags == eSortByNameDown)
        std::sort(m_file_items.begin(), m_file_items.end(), nameSorter<false>);
    else if (flags == eSortBySizeUp)
        std::sort(m_file_items.begin(), m_file_items.end(), sizeSorter<true>);
    else if (flags == eSortBySizeDown)
        std::sort(m_file_items.begin(), m_file_items.end(), sizeSorter<false>);
    else if (flags == eSortByModifUp)
        std::sort(m_file_items.begin(), m_file_items.end(), modifSorter<true>);
    else if (flags == eSortByModifDown)
        std::sort(m_file_items.begin(), m_file_items.end(), modifSorter<false>);
}

FS_file_list_ex file_list_open_ex(CLocatorAPI* fs, LPCSTR path, u32 flags, LPCSTR mask)
{
    return FS_file_list_ex(path, flags, mask);
}

FS_file_list file_list_open_script(CLocatorAPI* fs, LPCSTR initial, u32 flags)
{
    return FS_file_list(fs->file_list_open(initial, flags));
}

FS_file_list file_list_open_script_2(CLocatorAPI* fs, LPCSTR initial, LPCSTR folder, u32 flags)
{
    return FS_file_list(fs->file_list_open(initial, folder, flags));
}

void dir_delete_script_2(CLocatorAPI* fs, LPCSTR path, LPCSTR nm, int remove_files)
{
    fs->dir_delete(path, nm, remove_files);
}

void dir_delete_script(CLocatorAPI* fs, LPCSTR full_path, int remove_files) { fs->dir_delete(full_path, remove_files); }
LPCSTR get_file_age_str(CLocatorAPI* fs, LPCSTR nm)
{
    time_t t = fs->get_file_age(nm);
    struct tm* newtime;
    newtime = localtime(&t);
    return asctime(newtime);
}

static const CLocatorAPI::file* ExistS(CLocatorAPI* fs, const char* path, const char* name)
{
    string_path temp;
    fs->update_path(temp, path, name);
    return fs->GetFileDesc(temp);
}

SCRIPT_EXPORT(fs_registrator, (), {
    module(luaState)[class_<FS_item>("FS_item")
                         .def("NameFull", &FS_item::NameFull)
                         .def("NameShort", &FS_item::NameShort)
                         .def("Size", &FS_item::Size)
                         .def("ModifDigitOnly", &FS_item::ModifDigitOnly)
                         .def("Modif", &FS_item::Modif),

        class_<FS_file_list_ex>("FS_file_list_ex")
            .def("Size", &FS_file_list_ex::Size)
            .def("GetAt", &FS_file_list_ex::GetAt)
            .def("Sort", &FS_file_list_ex::Sort),

        class_<FS_file_list>("FS_file_list")
            .def("Size", &FS_file_list::Size)
            .def("GetAt", &FS_file_list::GetAt)
            .def("Free", &FS_file_list::Free),

        // XXX: uncomment
        /*		class_<FS_Path>("FS_Path")
                    .def_readonly("m_Path",						&FS_Path::m_Path)
                    .def_readonly("m_Root",						&FS_Path::m_Root)
                    .def_readonly("m_Add",						&FS_Path::m_Add)
                    .def_readonly("m_DefExt",					&FS_Path::m_DefExt)
                    .def_readonly("m_FilterCaption",			&FS_Path::m_FilterCaption),
        */
        class_<CLocatorAPI::file>("fs_file")
            .def_readonly("name", &CLocatorAPI::file::name)
            .def_readonly("vfs", &CLocatorAPI::file::vfs)
            .def_readonly("ptr", &CLocatorAPI::file::ptr)
            .def_readonly("size_real", &CLocatorAPI::file::size_real)
            .def_readonly("size_compressed", &CLocatorAPI::file::size_compressed)
            .def_readonly("modif", &CLocatorAPI::file::modif),

        class_<FileStatus>("FileStatus")
            .def_readonly("Exists", &FileStatus::Exists)
            .def_readonly("External", &FileStatus::External),

        class_<CLocatorAPI>("FS")
            .enum_("FS_sort_mode")[value("FS_sort_by_name_up", int(FS_file_list_ex::eSortByNameUp)),
                value("FS_sort_by_name_down", int(FS_file_list_ex::eSortByNameDown)),
                value("FS_sort_by_size_up", int(FS_file_list_ex::eSortBySizeUp)),
                value("FS_sort_by_size_down", int(FS_file_list_ex::eSortBySizeDown)),
                value("FS_sort_by_modif_up", int(FS_file_list_ex::eSortByModifUp)),
                value("FS_sort_by_modif_down", int(FS_file_list_ex::eSortByModifDown))]
            .enum_("FS_List")[value("FS_ListFiles", int(FS_ListFiles)), value("FS_ListFolders", int(FS_ListFolders)),
                value("FS_ClampExt", int(FS_ClampExt)), value("FS_RootOnly", int(FS_RootOnly))]
            .enum_("FSType")[value("FSType_Virtual", int(FSType::Virtual)),
                value("FSType_External", int(FSType::External)), value("FSType_Any", int(FSType::Any))]
            .def("path_exist", &CLocatorAPI::path_exist)
            .def("update_path", &update_path_script)
            .def("get_path", static_cast<FS_Path*(CLocatorAPI::*)(pcstr)>(&CLocatorAPI::get_path))
            .def("get_path", static_cast<bool(CLocatorAPI::*)(pcstr, FS_Path**)>(&CLocatorAPI::get_path))
            .def("append_path", &CLocatorAPI::append_path)
            .def("rescan_path", &rescan_path_script) //Alundaio

            .def("file_delete", (void (CLocatorAPI::*)(LPCSTR, LPCSTR))(&CLocatorAPI::file_delete))
            .def("file_delete", (void (CLocatorAPI::*)(LPCSTR))(&CLocatorAPI::file_delete))

            .def("dir_delete", &dir_delete_script)
            .def("dir_delete", &dir_delete_script_2)

            .def("file_rename", &CLocatorAPI::file_rename)
            .def("file_length", &CLocatorAPI::file_length)
            .def("file_copy", &CLocatorAPI::file_copy)

            .def("exist", &ExistS)
            .def("exist", (FileStatus(CLocatorAPI::*)(LPCSTR, FSType))(&CLocatorAPI::exist))
            .def("exist", (FileStatus(CLocatorAPI::*)(LPCSTR, LPCSTR, FSType))(&CLocatorAPI::exist))

            .def("get_file_age", &CLocatorAPI::get_file_age)
            .def("get_file_age_str", &get_file_age_str)
            .def("r_open", (IReader * (CLocatorAPI::*)(LPCSTR, LPCSTR))(&CLocatorAPI::r_open))
            .def("r_open", (IReader * (CLocatorAPI::*)(LPCSTR))(&CLocatorAPI::r_open))
            .def("r_close", (void (CLocatorAPI::*)(IReader*&))(&CLocatorAPI::r_close))

            .def("w_open", (IWriter * (CLocatorAPI::*)(LPCSTR, LPCSTR))(&CLocatorAPI::w_open))
            .def("w_open", (IWriter * (CLocatorAPI::*)(LPCSTR))(&CLocatorAPI::w_close))
            .def("w_close", &CLocatorAPI::w_close)

            .def("file_list_open", &file_list_open_script)
            .def("file_list_open", &file_list_open_script_2)
            .def("file_list_open_ex", &file_list_open_ex),

        def("getFS", getFS)];
});
