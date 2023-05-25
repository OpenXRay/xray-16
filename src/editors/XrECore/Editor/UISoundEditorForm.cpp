//---------------------------------------------------------------------------
#include "stdafx.h"
#include "UISoundEditorForm.h"
#include "SoundManager.h"
#include "../../../xrSound/stdafx.h"
#include "../../../xrSound/soundrender_source.h"
UISoundEditorForm *UISoundEditorForm::Form = nullptr;

UISoundEditorForm::UISoundEditorForm()
{
    m_ItemProps = xr_new<UIPropertiesForm>();
    m_ItemList = xr_new<UIItemListForm>();
    m_ItemList->SetOnItemFocusedEvent(TOnILItemFocused(this, &UISoundEditorForm::OnItemsFocused));
    modif_map.clear();
    m_Flags.zero();
    InitItemList();
    if (!FS.can_write_to_alias(_sounds_))
    {
        Log("#!You don't have permisions to modify sounds.");
        m_ItemProps->SetReadOnly(TRUE);
        m_Flags.set(flReadOnly, TRUE);
        bAutoPlay = TRUE;
    }
}

UISoundEditorForm::~UISoundEditorForm()
{
    xr_delete(m_ItemProps);
    xr_delete(m_ItemList);
}

void UISoundEditorForm::Draw()
{
    ImGui::BeginChild("Left", ImVec2(200, 400), true);
    m_ItemList->Draw();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Right", ImVec2(300, 400));
    m_ItemProps->Draw();
    ImGui::EndChild();
    ImGui::Separator();
    if (ImGui::Button("Close"))
    {
        bOpen = false;
        ImGui::CloseCurrentPopup();
        HideLib();
    }
    ImGui::SameLine();
    if (ImGui::Button("Ok"))
    {
        m_Snd.destroy();
        UpdateLib();
        HideLib();
    }
}

void UISoundEditorForm::Update()
{
    if (Form)
    {
        if (!Form->IsClosed())
        {
            if (ImGui::BeginPopupModal("SoundEditor", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize, true))
            {
                Form->Draw();
                ImGui::EndPopup();
            }
        }
        else
        {
            xr_delete(Form);
        }
    }
}

void UISoundEditorForm::Show()
{
    VERIFY(!Form);
    Form = xr_new<UISoundEditorForm>();
}

void UISoundEditorForm::HideLib()
{
    m_Snd.destroy();
    bOpen = false;
    ImGui::CloseCurrentPopup();
}

void UISoundEditorForm::RegisterModifiedTHM()
{
    if (m_ItemProps->IsModified())
    {
        for (THMIt t_it = m_THM_Current.begin(); t_it != m_THM_Current.end(); t_it++)
        {
            //.            (*t_it)->Save	(0,0);
            AppendModif((*t_it)->SrcName());
        }
    }
}

void UISoundEditorForm::OnModified()
{
}

void UISoundEditorForm::UpdateLib()
{
    RegisterModifiedTHM();
    SaveUsedTHM();
    // save game sounds
    if (modif_map.size())
    {
        AStringVec modif;
        SndLib->SynchronizeSounds(true, true, true, &modif_map, 0);
        //		SndLib->ChangeFileAgeTo		(&modif_map,time(NULL));
        SndLib->RefreshSounds(false);
        modif_map.clear();
    }
}

void UISoundEditorForm::AppendModif(LPCSTR nm)
{
    FS_File dest;
    string_path fname;
    FS.update_path(fname, _sounds_, EFS.ChangeFileExt(nm, ".wav").c_str());
    BOOL bFind = FS.file_find(fname, dest);
    R_ASSERT(bFind);
    modif_map.insert(dest);
}

void UISoundEditorForm::OnControlClick(ButtonValue *V, bool &bModif, bool &bSafe)
{
    switch (V->btn_num)
    {
    case 0:
        m_Snd.play(0, sm_2D);
        break;
    case 1:
        m_Snd.stop();
        break;
    case 2:
    {
        bAutoPlay = !bAutoPlay;
        V->value[V->btn_num] = shared_str().printf("Auto (%s)", bAutoPlay ? "on" : "off");
    }
    break;
    }
    bModif = false;
}

void UISoundEditorForm::OnControl2Click(ButtonValue *V, bool &bModif, bool &bSafe)
{
    switch (V->btn_num)
    {
    case 0:
    {
        bAutoPlay = !bAutoPlay;
        V->value[V->btn_num] = bAutoPlay ? "on" : "off";
    }
    break;
    }
    bModif = false;
}

void UISoundEditorForm::OnSyncCurrentClick(ButtonValue *V, bool &bModif, bool &bSafe)
{
    THMIt it = m_THM_Current.begin();
    THMIt it_e = m_THM_Current.end();

    for (; it != it_e; ++it)
    {
        ESoundThumbnail *pTHM = *it;

        string_path src_name, game_name;
        FS.update_path(src_name, _sounds_, pTHM->SrcName());
        strconcat(sizeof(src_name), src_name, src_name, ".wav");

        FS.update_path(game_name, _game_sounds_, pTHM->SrcName());
        strconcat(sizeof(game_name), game_name, game_name, ".ogg");

        Msg("synchronizing [%s]", game_name);
        SndLib->MakeGameSound(pTHM, src_name, game_name);
    }
    Msg("Done.");
}

void UISoundEditorForm::OnAttClick(ButtonValue *V, bool &bModif, bool &bSafe)
{
    bModif = true;
    ESoundThumbnail *thm = m_THM_Current.back();
    switch (V->btn_num)
    {
    case 0:
    {
        float dist = thm->MinDist() / (0.01f * psSoundRolloff);
        thm->SetMaxDist(dist + 0.1f * dist);
    }
    break;
    case 1:
    {
        float dist = psSoundRolloff * (thm->MaxDist() - (0.1f / 1.1f) * thm->MaxDist()) * 0.01f;
        thm->SetMinDist(dist);
    }
    break;
    }
}

void UISoundEditorForm::InitItemList()
{
    FS_FileSet sound_map;
    SndLib->GetSounds(sound_map, TRUE);

    ListItemsVec items;

    // fill items
    FS_FileSetIt it = sound_map.begin();
    FS_FileSetIt _E = sound_map.end();
    for (; it != _E; it++)
        LHelper().CreateItem(items, it->name.c_str(), 0);

    m_ItemList->AssignItems(items /*, false, true*/);
}

void UISoundEditorForm::OnItemsFocused(ListItem *item)
{
    PropItemVec props;

    RegisterModifiedTHM();
    m_Snd.destroy();
    m_THM_Current.clear();

    if (item != nullptr)
    {
        ListItem *prop = item;
        if (prop)
        {
            ESoundThumbnail *thm = FindUsedTHM(prop->Key());
            if (!thm)
                m_THM_Used.push_back(thm = xr_new<ESoundThumbnail>(prop->Key()));
            m_THM_Current.push_back(thm);
            thm->FillProp(props);
        }
    }

    ButtonValue *B = 0;
    if (m_THM_Current.size() == 1)
    {
        ESoundThumbnail *thm = m_THM_Current.back();
        u32 size = 0;
        u32 time = 0;
        PlaySound(thm->SrcName(), size, time);

        CanvasValue *C = 0;
        C = PHelper().CreateCanvas(props, "Attenuation", "", 64);
        C->tag = (size_t)this;
        C->OnDrawCanvasEvent.bind(this, &UISoundEditorForm::OnAttenuationDraw);
        //		C->OnTestEqual.bind			(this,&TfrmSoundLib::OnPointDataTestEqual);
        B = PHelper().CreateButton(props, "Auto Att", "By Min,By Max", ButtonValue::flFirstOnly);
        B->OnBtnClickEvent.bind(this, &UISoundEditorForm::OnAttClick);

        PHelper().CreateCaption(props, "File Length", shared_str().printf("%.2f Kb", float(size) / 1024.f));
        PHelper().CreateCaption(props, "Total Time", shared_str().printf("%.2f sec", float(time) / 1000.f));
        if (!m_Flags.is(flReadOnly))
        {
            B = PHelper().CreateButton(props, "Control", "Play,Stop", ButtonValue::flFirstOnly);
            B->OnBtnClickEvent.bind(this, &UISoundEditorForm::OnControlClick);
        }
    }
    if (!m_Flags.is(flReadOnly))
    {
        B = PHelper().CreateButton(props, "Auto Play", bAutoPlay ? "on" : "off", ButtonValue::flFirstOnly);
        B->OnBtnClickEvent.bind(this, &UISoundEditorForm::OnControl2Click);
    }

    if (!m_Flags.is(flReadOnly) && m_THM_Current.size())
    {
        B = PHelper().CreateButton(props, "MANAGE", "SyncCurrent", ButtonValue::flFirstOnly);
        B->OnBtnClickEvent.bind(this, &UISoundEditorForm::OnSyncCurrentClick);
    }

    m_ItemProps->AssignItems(props);
}
void UISoundEditorForm::PlaySound(LPCSTR name, u32 &size, u32 &time)
{
    string_path fname;
    FS.update_path(fname, _game_sounds_, EFS.ChangeFileExt(name, ".ogg").c_str());
    FS_File F;
    if (FS.file_find(fname, F))
    {
        m_Snd.create(name, st_Effect, sg_Undefined);
        m_Snd.play(0, sm_2D);
        CSoundRender_Source *src = (CSoundRender_Source *)m_Snd._handle();
        VERIFY(src);
        size = F.size;
        time = iFloor(src->fTimeTotal / 1000.0f);
        if (!bAutoPlay)
            m_Snd.stop();
    }
}
void UISoundEditorForm::OnAttenuationDraw(CanvasValue *sender)
{
#define WIETH 90
#define HEIGHT 80.f
    ESoundThumbnail *thm = m_THM_Current.back();
    float d_cost = thm->MaxDist() / WIETH;
    static float values[WIETH] = {};
    for (int d = 1; d < WIETH + 1; d++)
    {
        float R = d * d_cost;
        float b = thm->MinDist() / (psSoundRolloff * R);
        //		float b = m_Brightness/(m_Attenuation0+m_Attenuation1*R+m_Attenuation2*R*R);
        float bb = (HEIGHT * b);
        float y = floorf(bb);
        clamp(y, 0.f, HEIGHT);
        values[d - 1] = y;
    }
    ImGui::PlotLines("##LinesSound", values, IM_ARRAYSIZE(values), 0, 0, 0, HEIGHT, ImVec2(0, HEIGHT), 4);
}
ESoundThumbnail *UISoundEditorForm::FindUsedTHM(LPCSTR name)
{
    for (THMIt it = m_THM_Used.begin(); it != m_THM_Used.end(); it++)
        if (0 == strcmp((*it)->SrcName(), name))
            return *it;
    return 0;
}

void UISoundEditorForm::SaveUsedTHM()
{
    int m_age = time(NULL);
    for (THMIt t_it = m_THM_Used.begin(); t_it != m_THM_Used.end(); t_it++)
        (*t_it)->Save(m_age, 0);
}

void UISoundEditorForm::DestroyUsedTHM()
{
    for (THMIt it = m_THM_Used.begin(); it != m_THM_Used.end(); it++)
        xr_delete(*it);
    m_THM_Used.clear();
}
