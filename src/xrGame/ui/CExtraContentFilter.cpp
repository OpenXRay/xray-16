#include "StdAfx.h"
#include "CExtraContentFilter.h"
#include "../RegistryFuncs.h"
#include "object_broker.h"

#define EXTRA_CONTENT_SECTION "extracontent_packs"

bool CExtraContentFilter::CheckPackKey(LPCSTR KeyName)
{
    if (!KeyName)
        return false;

    DWORD KeyValue = 0;
    ReadRegistry_DWValue(KeyName, KeyValue);

    return KeyValue == 1;
}

CExtraContentFilter::CExtraContentFilter()
{
    if (pSettings->section_exist(EXTRA_CONTENT_SECTION))
    {
        u32 PacksCount = pSettings->line_count(EXTRA_CONTENT_SECTION);
        for (u32 i = 0; i < PacksCount; i++)
        {
            PackData* pNewPack = xr_new<PackData>();
            LPCSTR PackName, KeyName;
            pSettings->r_line(EXTRA_CONTENT_SECTION, i, &PackName, &KeyName);
            pNewPack->sPackName = PackName;
            pNewPack->bEnabled = CheckPackKey(KeyName);

            if (pSettings->section_exist(pNewPack->sPackName))
            {
                u32 ContentCount = pSettings->line_count(pNewPack->sPackName);
                for (u32 c = 0; c < ContentCount; c++)
                {
                    LPCSTR ContName, V;
                    pSettings->r_line(pNewPack->sPackName, c, &ContName, &V);
                    pNewPack->aContent.push_back(ContName);
                }
            }

            aDataPacks.push_back(pNewPack);
        }
    }
}

CExtraContentFilter::~CExtraContentFilter()
{
    for (u32 i = 0; i < aDataPacks.size(); i++)
    {
        delete_data(aDataPacks[i]);
    }
    delete_data(aDataPacks);
}

bool CExtraContentFilter::IsDataEnabled(LPCSTR pData)
{
    for (xr_vector<PackData*>::iterator it = aDataPacks.begin(); it != aDataPacks.end(); it++)
    {
        PackData* pPackData = *it;
        xr_vector<shared_str>::const_iterator i =
            std::find(pPackData->aContent.begin(), pPackData->aContent.end(), pData);
        if (i != pPackData->aContent.end())
        {
            return pPackData->bEnabled;
        }
    }
    return true;
}
