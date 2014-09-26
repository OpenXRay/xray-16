#include "stdafx.h"
#pragma hdrstop

#include <vcl.h>
#include "FrmDBXpacker.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ElShellCtl"
#pragma link "ElTree"
#pragma link "ElXPThemedControl"
#pragma link "ElListBox"
#pragma link "ExtBtn"
#pragma link "MXCtrls"
#pragma resource "*.dfm"

TDB_packer* TDB_packer::m_form = NULL;

//---------------------------------------------------------------------------
__fastcall TDB_packer::TDB_packer(TComponent* Owner)
        : TForm(Owner)
{
	m_cfgFileName 	= "mod_pack.ltx";
    _load_			(m_cfgFileName);
}
//---------------------------------------------------------------------------
void __fastcall TDB_packer::ActivatePacker()
{
    if(!m_form)
    {
         m_form             = xr_new<TDB_packer>((TComponent*)0);
    }
    m_form->prepare			();
    m_form->ShowModal       ();
}

void __fastcall TDB_packer::prepare()
{
	string_path 				_curr_path;
    GetCurrentDirectory			(sizeof(_curr_path), _curr_path);
    FS_Path* pth = FS.get_path	("$game_data$");
    u32 sz 						= xr_strlen(_curr_path);
    string16					tmp;
    tmp[0]						= 0;

    if(_curr_path[sz-1] !='\\')
		strcpy(tmp,"\\");

    strconcat					(sizeof(m_root_folder),m_root_folder, _curr_path, tmp, pth->m_Path);
        
    Log(m_root_folder);
   shellTree->CustomRootFolder 	= m_root_folder;
    Log(m_root_folder);
   shellTree->RootFolder 		= sfoCustom;
    Log(m_root_folder);
}

void __fastcall TDB_packer::btnSaveClick(TObject *Sender)
{
   	CInifile ini						(m_cfgFileName.c_str(), FALSE, FALSE, TRUE);

   	for(int i=0; i<lbIncludeFolders->Items->Count; ++i)
    {
    	AnsiString astr = lbIncludeFolders->Items->Strings[i];
    	ini.w_bool("include_folders",astr.c_str(), TRUE);
    }

   	for(int j=0; j<lbIncludeFiles->Items->Count; ++j)
    {
    	AnsiString astr = lbIncludeFiles->Items->Strings[j];
    	ini.w_bool("include_files",astr.c_str(), TRUE);
    }

}

void TDB_packer::_load_(const xr_string& fname)
{
    	lbIncludeFolders->Items->Clear		();
    	lbIncludeFiles->Items->Clear		();

     	CInifile ini						(fname.c_str());

        if(ini.section_exist("include_folders"))
        {
           CInifile::Sect S 		= ini.r_section("include_folders");
           CInifile::SectCIt it 		= S.Data.begin();
           CInifile::SectCIt it_e 	= S.Data.end();
           for( ;it!=it_e; ++it)
           {
           		WideString 					ws;
                ws 							= (*it).first.c_str();
               lbIncludeFolders->Items->Add	(ws);
           }
        }
        if(ini.section_exist("include_files"))
        {
           CInifile::Sect S 		= ini.r_section("include_files");
           CInifile::SectCIt it 	= S.Data.begin();
           CInifile::SectCIt it_e 	= S.Data.end();
           for( ;it!=it_e; ++it)
           {
           		WideString 					ws;
                ws 							= (*it).first.c_str();
               lbIncludeFiles->Items->Add	(ws);
           }
        }
        Caption	=	fname.c_str();
}

void __fastcall TDB_packer::btnLoadClick(TObject *Sender)
{
    if( EFS.GetOpenName("$fs_root$", m_cfgFileName, false, NULL, 0) )
    {
    	_load_(m_cfgFileName);
    }
}
void remove_item_from_lb(TElListBox* lb)
{
bool b = true;
    while(b)
    {
    	b = false;
    	for(int i=0; i<lb->Items->Count; ++i)
        {
			if( lb->Selected[i] )
            {
	            lb->Items->Delete(i);
                b = true;
                break;
            }
        }
     }
}

void __fastcall TDB_packer::ExtBtn2Click(TObject *Sender)
{
	remove_item_from_lb(lbIncludeFolders);
}
//---------------------------------------------------------------------------

void __fastcall TDB_packer::ExtBtn4Click(TObject *Sender)
{
	remove_item_from_lb(lbIncludeFiles);
}
//---------------------------------------------------------------------------


void __fastcall TDB_packer::ExtBtn1Click(TObject *Sender)
{
	TElShellTreeItem* itm = shellTree->ItemFocused;
    if(itm->IsFolder)
    {
        AnsiString str				= itm->FullName;
        int root_len 				= xr_strlen(m_root_folder);
        int len						= str.Length();
    	lbIncludeFolders->Items->Add( str.SubString(root_len+1,len-root_len) );
    }
}
//---------------------------------------------------------------------------

void __fastcall TDB_packer::ExtBtn3Click(TObject *Sender)
{
	TElShellTreeItem* itm = shellTree->ItemFocused;
    if(!itm->IsFolder)
    {
        AnsiString str				= itm->FullName;
        int root_len 				= xr_strlen(m_root_folder);
        int len						= str.Length();
    	lbIncludeFiles->Items->Add( str.SubString(root_len+1,len-root_len) );
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

void __fastcall TDB_packer::ExtBtn5Click(TObject *Sender)
{
	btnSaveClick((TObject*)0);
	spawnl(P_WAIT, "compress_dbx.bat", "compress_dbx.bat", m_cfgFileName.c_str());
}
//---------------------------------------------------------------------------

