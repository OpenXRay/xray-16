#include "StdAfx.h"
#include "best_scores_store.h"
#include "xrGameSpy/GameSpy_Full.h"
#include "xrGameSpy/GameSpy_SAKE.h"

namespace gamespy_profile
{
best_scores_store::best_scores_store(CGameSpy_Full* fullgs_obj)
{
    VERIFY(fullgs_obj && fullgs_obj->GetGameSpySAKE());
    m_fullgs_obj = fullgs_obj;
    m_sake_obj = fullgs_obj->GetGameSpySAKE();

    init_field_names();

    m_get_records_input.mTableId = (char*)profile_table_name;
    m_get_records_input.mFieldNames = m_field_names_store;
    m_get_records_input.mNumFields = fields_count;
}

best_scores_store::~best_scores_store()
{
    // VERIFY(!m_scores_operation_cb);
}

void best_scores_store::init_field_names()
{
    for (int i = 0; i < bst_score_types_count; ++i)
    {
        m_field_names_store[i] = ATLAS_GET_STAT_NAME(get_best_score_id_stat(static_cast<enum_best_score_type>(i)));
    }
}

void best_scores_store::reset_scores()
{
    m_result_scores.clear();
    m_ltx_result_scores.clear();

    for (int bsidx = 0; bsidx < bst_score_types_count; ++bsidx)
    {
        m_result_scores.insert(std::make_pair(enum_best_score_type(bsidx), 0));
    };
}

void best_scores_store::load_best_scores(store_operation_cb& opcb)
{
    m_scores_operation_cb = opcb;

    SAKERequest reqres =
        m_sake_obj->GetMyRecords(&m_get_records_input, &best_scores_store::get_my_player_scores_cb, this);

    if (!reqres)
    {
        SAKEStartRequestResult tmp_result = m_sake_obj->GetRequestResult();
        m_scores_operation_cb(false, CGameSpy_SAKE::TryToTranslate(tmp_result).c_str());
        m_scores_operation_cb.clear();
    }
}

void best_scores_store::load_best_scores_from_ltx(CInifile& ini)
{
    for (int i = 0; i != bst_score_types_count; ++i)
    {
        enum_best_score_type bstype = static_cast<enum_best_score_type>(i);
        m_ltx_result_scores.insert(
            std::make_pair(bstype, ini.r_u32(get_best_score_name(bstype), best_score_value_line)));
    }
}

void best_scores_store::merge_sake_to_ltx_best_scores()
{
    for (all_best_scores_t::iterator i = m_ltx_result_scores.begin(), ie = m_ltx_result_scores.end(); i != ie; ++i)
    {
        all_best_scores_t::const_iterator tmp_bi = m_result_scores.find(i->first);
        if (tmp_bi != m_result_scores.end())
        {
            u32 tmp_value = std::max(i->second, tmp_bi->second);
            i->second = tmp_value;
        }
    }
}

all_best_scores_t& best_scores_store::get_player_best_scores()
{
    if (m_ltx_result_scores.empty())
        return m_result_scores;

    return m_ltx_result_scores;
}

bool best_scores_store::is_sake_equal_to_file() const
{
    VERIFY(!m_ltx_result_scores.empty());
    if (m_ltx_result_scores.empty())
        return true;

    for (all_best_scores_t::const_iterator i = m_ltx_result_scores.begin(), ie = m_ltx_result_scores.end(); i != ie;
         ++i)
    {
        all_best_scores_t::const_iterator tmp_iter = m_result_scores.find(i->first);
        R_ASSERT(tmp_iter != m_result_scores.end());

        if (i->second != tmp_iter->second)
            return false;
    }
    return true;
}

void __cdecl best_scores_store::get_my_player_scores_cb(
    SAKE sake, SAKERequest request, SAKERequestResult result, void* inputData, void* outputData, void* userData)
{
    best_scores_store* my_inst = static_cast<best_scores_store*>(userData);
    VERIFY(my_inst && my_inst->m_scores_operation_cb);
    if (result != SAKERequestResult_SUCCESS)
    {
        my_inst->m_scores_operation_cb(false, CGameSpy_SAKE::TryToTranslate(result).c_str());
    }
    else
    {
        SAKEGetMyRecordsOutput* tmp_out = static_cast<SAKEGetMyRecordsOutput*>(outputData);
        VERIFY(tmp_out);
        my_inst->process_scores_out_response(tmp_out, fields_count);
        my_inst->m_scores_operation_cb(true, "mp_load_best_scores_complete");
    }
    my_inst->m_scores_operation_cb.clear();
}

void best_scores_store::process_scores_out_response(SAKEGetMyRecordsOutput* tmp_out, int const out_fields_count)
{
    VERIFY(tmp_out->mNumRecords <= 1); // one raw
    if (tmp_out->mNumRecords == 0)
    {
        for (int i = 0; i < bst_score_types_count; ++i)
        {
            m_result_scores.insert(std::make_pair(static_cast<gamespy_profile::enum_best_score_type>(i), 0));
        }
        return;
    }

    for (int i = 0; i < out_fields_count; ++i)
    {
        enum_best_score_type bst = get_best_score_type_by_sname(tmp_out->mRecords[0][i].mName);
        if (bst == bst_score_types_count)
            continue;
        s32 bs_value = tmp_out->mRecords[0][i].mValue.mInt; // one raw
        m_result_scores.insert(std::make_pair(bst, bs_value));
    };
}

} // namespace gamespy_profile
