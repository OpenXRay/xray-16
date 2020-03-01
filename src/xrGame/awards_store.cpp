#include "StdAfx.h"
#include "awards_store.h"
#include "xrGameSpy/GameSpy_Full.h"
#include "xrGameSpy/GameSpy_SAKE.h"

namespace gamespy_profile
{
awards_store::awards_store(CGameSpy_Full* fullgs_obj)
{
    VERIFY(fullgs_obj && fullgs_obj->GetGameSpySAKE());
    m_fullgs_obj = fullgs_obj;
    m_sake_obj = fullgs_obj->GetGameSpySAKE();

    init_field_names();
    m_get_records_input.mTableId = (char*)profile_table_name;
    m_get_records_input.mFieldNames = m_field_names_store;
    m_get_records_input.mNumFields = fields_count;
}

awards_store::~awards_store() {}
void awards_store::init_field_names()
{
    for (int i = 0; i < at_awards_count; ++i)
    {
        int findex = i * ap_award_params_count;
        m_field_names_store[findex + ap_award_id] =
            ATLAS_GET_STAT_NAME(get_award_id_stat(static_cast<enum_awards_t>(i)));
        m_field_names_store[findex + ap_award_rdate] =
            ATLAS_GET_STAT_NAME(get_award_reward_date_stat(static_cast<enum_awards_t>(i)));
    }
}

void awards_store::reset_awards()
{
    m_awards_result.clear();
    m_ltx_awards_result.clear();

    for (int aidx = 0; aidx < at_awards_count; ++aidx)
    {
        m_awards_result.insert(std::make_pair(enum_awards_t(aidx), award_data(u16(0), u32(0))));
    };
}

void awards_store::load_awards(store_operation_cb& opcb)
{
    m_award_operation_cb = opcb;

    SAKERequest reqres = m_sake_obj->GetMyRecords(&m_get_records_input, &awards_store::get_my_awards_cb, this);

    if (!reqres)
    {
        SAKEStartRequestResult tmp_result = m_sake_obj->GetRequestResult();
        m_award_operation_cb(false, CGameSpy_SAKE::TryToTranslate(tmp_result).c_str());
        m_award_operation_cb.clear();
    }
}

void awards_store::load_awards_from_ltx(CInifile& ini)
{
    for (int i = 0; i < at_awards_count; ++i)
    {
        enum_awards_t tmp_awid = static_cast<enum_awards_t>(i);
        LPCSTR tmp_award_name = get_award_name(tmp_awid);
        u16 tmp_count = ini.r_u16(tmp_award_name, award_count_line);
        u32 tmp_rdate = ini.r_u32(tmp_award_name, award_rdate_line);
        m_ltx_awards_result.insert(std::make_pair(tmp_awid, award_data(tmp_count, tmp_rdate)));
    }
}
void awards_store::merge_sake_to_ltx_awards()
{
    for (all_awards_t::iterator i = m_ltx_awards_result.begin(), ie = m_ltx_awards_result.end(); i != ie; ++i)
    {
        all_awards_t::const_iterator tmp_awi = m_awards_result.find(i->first);
        if (tmp_awi != m_awards_result.end())
        {
            u16 tmp_count = std::max(i->second.m_count, tmp_awi->second.m_count);
            u32 tmp_rdate = std::max(i->second.m_last_reward_date, tmp_awi->second.m_last_reward_date);
            i->second = award_data(tmp_count, tmp_rdate);
        }
    }
}

all_awards_t& awards_store::get_player_awards()
{
    if (m_ltx_awards_result.empty())
        return m_awards_result;
    return m_ltx_awards_result;
}

bool awards_store::is_sake_equal_to_file() const
{
    VERIFY(!m_ltx_awards_result.empty());
    if (m_ltx_awards_result.empty())
        return true; // unknown

    for (all_awards_t::const_iterator i = m_ltx_awards_result.begin(), ie = m_ltx_awards_result.end(); i != ie; ++i)
    {
        all_awards_t::const_iterator tmp_iter = m_awards_result.find(i->first);
        R_ASSERT(tmp_iter != m_awards_result.end());

        if (i->second.m_count != tmp_iter->second.m_count)
            return false;
    }
    return true;
}

void awards_store::process_award(SAKEField* award_params)
{
    enum_awards_t awid = get_award_by_stat_name(award_params[ap_award_id].mName);
    VERIFY(awid != at_awards_count);

    int rdate_statid_real = get_award_reward_date_stat(awid);
    int rdate_statid_from_table = ATLAS_GET_STAT(award_params[ap_award_rdate].mName);
    VERIFY(rdate_statid_real == rdate_statid_from_table);

    u16 awards_count = award_params[ap_award_id].mValue.mShort;
    u32 award_rdate = award_params[ap_award_rdate].mValue.mInt;
    m_awards_result.insert(std::make_pair(awid, award_data(awards_count, award_rdate)));
}

void awards_store::process_aw_out_response(SAKEGetMyRecordsOutput* tmp_out, int const out_fields_count)
{
    VERIFY(tmp_out->mNumRecords <= 1); // one raw
    if (tmp_out->mNumRecords == 0)
        return;

    for (int i = 0; i < out_fields_count; ++i)
    {
        if (get_award_by_stat_name(tmp_out->mRecords[0][i].mName) != at_awards_count)
        {
            process_award(&tmp_out->mRecords[0][i]);
        }
    }
}

void __cdecl awards_store::get_my_awards_cb(
    SAKE sake, SAKERequest request, SAKERequestResult result, void* inputData, void* outputData, void* userData)
{
    awards_store* my_inst = static_cast<awards_store*>(userData);
    VERIFY(my_inst && my_inst->m_award_operation_cb);
    if (result != SAKERequestResult_SUCCESS)
    {
        my_inst->m_award_operation_cb(false, CGameSpy_SAKE::TryToTranslate(result).c_str());
    }
    else
    {
        SAKEGetMyRecordsOutput* tmp_out = static_cast<SAKEGetMyRecordsOutput*>(outputData);
        VERIFY(tmp_out);
        my_inst->process_aw_out_response(tmp_out, fields_count);
        my_inst->m_award_operation_cb(true, "");
    }
    my_inst->m_award_operation_cb.clear();
}

} // namespace gamespy_profile
