///////////////////////////////////////////////////////////////
// ini_id_loader.h
// темплейтовый класс, который загружает из ini файла
// строку с текстовыми id, потом присваивает каждому текстовому
// id уникальный index
///////////////////////////////////////////////////////////////

#pragma once

// T_ID, T_INDEX -	тип индекса и id

// ITEM_DATA		-	структура с полями id и index типа T_ID и T_INDEX,
//					обязательно имеет конструктор с параметрами (T_INDEX index, T_ID id, LPCSTR r1, ..., LPCSTR rN)
//					N = ITEM_REC_NUM - число доп. параметров в ITEM_DATA

// T_INIT		-	класс где определена статическая InitIdToIndex
//					функция инициализации section_name и line_name

#define TEMPLATE_SPECIALIZATION \
    template <bool ITEM_REC_NUM, typename ITEM_DATA, typename T_ID, typename T_INDEX, typename T_INIT>
#define CSINI_IdToIndex CIni_IdToIndex<ITEM_REC_NUM, ITEM_DATA, T_ID, T_INDEX, T_INIT>

TEMPLATE_SPECIALIZATION
class CIni_IdToIndex
{
public:
    typedef T_INDEX index_type;
    typedef T_ID id_type;

protected:
    typedef xr_vector<ITEM_DATA> T_VECTOR;
    static T_VECTOR* m_pItemDataVector;

    template <bool isNum>
    static void LoadItemData(u32 count, LPCSTR cfgRecord)
    {
        for (u32 k = 0; k < count; k += 1)
        {
            string64 buf;
            LPCSTR id_str = _GetItem(cfgRecord, k, buf);
            char* id_str_lwr = xr_strdup(id_str); // not used?
            xr_strlwr(id_str_lwr);

            if constexpr (!isNum)
            {
                ITEM_DATA item_data(T_INDEX(m_pItemDataVector->size()), T_ID(id_str));
                m_pItemDataVector->push_back(item_data);
            }
            else
            {
                string64 buf1;
                LPCSTR rec1 = _GetItem(cfgRecord, ++k, buf1);
                ITEM_DATA item_data(T_INDEX(m_pItemDataVector->size()), T_ID(id_str), rec1);
                m_pItemDataVector->push_back(item_data);
            }
            xr_free(id_str_lwr);
        }
    }

    //имя секции и линии откуда будут загружаться id
    static LPCSTR section_name;
    static LPCSTR line_name;

public:
    CIni_IdToIndex();
    virtual ~CIni_IdToIndex();

    static void InitInternal();
    static const ITEM_DATA* GetById(const T_ID& str_id, bool no_assert = false);
    static const ITEM_DATA* GetByIndex(T_INDEX index, bool no_assert = false);

    static const T_INDEX IdToIndex(const T_ID& str_id, T_INDEX default_index = T_INDEX(-1), bool no_assert = false)
    {
        const ITEM_DATA* item = GetById(str_id, no_assert);
        return item ? item->index : default_index;
    }
    static const T_ID IndexToId(T_INDEX index, T_ID default_id = NULL, bool no_assert = false)
    {
        const ITEM_DATA* item = GetByIndex(index, no_assert);
        return item ? item->id : default_id;
    }

    static const T_INDEX GetMaxIndex() { return m_pItemDataVector->size() - 1; }
    //удаление статичекого массива
    static void DeleteIdToIndexData();
};

TEMPLATE_SPECIALIZATION
typename CSINI_IdToIndex::T_VECTOR* CSINI_IdToIndex::m_pItemDataVector = NULL;

TEMPLATE_SPECIALIZATION
LPCSTR CSINI_IdToIndex::section_name = NULL;
TEMPLATE_SPECIALIZATION
LPCSTR CSINI_IdToIndex::line_name = NULL;

TEMPLATE_SPECIALIZATION
CSINI_IdToIndex::CIni_IdToIndex() {}
TEMPLATE_SPECIALIZATION
CSINI_IdToIndex::~CIni_IdToIndex() {}
TEMPLATE_SPECIALIZATION
const ITEM_DATA* CSINI_IdToIndex::GetById(const T_ID& str_id, bool no_assert)
{
    typename T_VECTOR::iterator it;
    for (it = m_pItemDataVector->begin(); m_pItemDataVector->end() != it; it++)
    {
        if (!xr_strcmp((*it).id, str_id))
            break;
    }

    if (m_pItemDataVector->end() == it)
    {
        R_ASSERT3(no_assert, "item not found, id", *str_id);
        return NULL;
    }

    return &(*it);
}

TEMPLATE_SPECIALIZATION
const ITEM_DATA* CSINI_IdToIndex::GetByIndex(T_INDEX index, bool no_assert)
{
    if ((size_t)index >= m_pItemDataVector->size())
    {
        if (!no_assert)
            xrDebug::Fatal(DEBUG_INFO, "item by index not found in section %s, line %s", section_name, line_name);
        return NULL;
    }
    return &(m_pItemDataVector->at(index));
}

TEMPLATE_SPECIALIZATION
void CSINI_IdToIndex::DeleteIdToIndexData() { xr_delete(m_pItemDataVector); }
TEMPLATE_SPECIALIZATION
void CSINI_IdToIndex::InitInternal()
{
    VERIFY(!m_pItemDataVector);
    T_INIT::InitIdToIndex();
    {
        m_pItemDataVector = new T_VECTOR();

        VERIFY(section_name);
        VERIFY(line_name);

        LPCSTR cfgRecord = pSettings->r_string(section_name, line_name);
        VERIFY(cfgRecord);
        u32 count = _GetItemCount(cfgRecord);
        LoadItemData<ITEM_REC_NUM>(count, cfgRecord);
    }
}

#undef TEMPLATE_SPECIALIZATION
