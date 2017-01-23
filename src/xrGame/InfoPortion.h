#pragma once
/*
#include "shared_data.h"
#include "PhraseScript.h"

#include "xml_str_id_loader.h"
#include "encyclopedia_article_defs.h"

#include "PhraseDialogDefs.h"


struct SInfoPortionData : CSharedResource
{
						SInfoPortionData ();
	virtual				~SInfoPortionData ();

	//массив с именами диалогов, которые могут быть инициированы
	//из этого InfoPortion
	DIALOG_ID_VECTOR	m_DialogNames;

	//список статей в энциклопедии, которые становятся известными 
	ARTICLE_ID_VECTOR	m_Articles;
	//список статей в энциклопедии, которые становятся неизвестными (на тот случай если
	//нужно заменить одну статью другой)
	ARTICLE_ID_VECTOR	m_ArticlesDisable;
    	
	//скриптовые действия, которые активируется после того как 
	//информацию получает персонаж
	CDialogScriptHelper		m_InfoScriptHelper;

	//массив с индексами тех порций информации, которые
	//исчезнут, после получения этой info_portion
	DEFINE_VECTOR		(shared_str, INFO_ID_VECTOR, INFO_ID_VECTOR_IT);
	INFO_ID_VECTOR		m_DisableInfo;
};


class CInfoPortion;

//квант  - порция информации
class CInfoPortion : public CSharedClass<SInfoPortionData, shared_str, false>,
					 public CXML_IdToIndex<CInfoPortion>
{
private:
	typedef CSharedClass<SInfoPortionData, shared_str, false>	inherited_shared;
	typedef CXML_IdToIndex<CInfoPortion>						id_to_index;

	friend id_to_index;
public:
				CInfoPortion	(void);
	virtual		~CInfoPortion	(void);

	//инициализация info данными
	//если info с таким id раньше не использовался
	//он будет загружен из файла
	virtual void Load	(shared_str info_str_id);
	const ARTICLE_ID_VECTOR&						Articles	()	const {return info_data()->m_Articles;}
	const ARTICLE_ID_VECTOR&						ArticlesDisable	()	const {return info_data()->m_ArticlesDisable;}
	const DIALOG_ID_VECTOR&							DialogNames	()	const {return info_data()->m_DialogNames;}
	const SInfoPortionData::INFO_ID_VECTOR&			DisableInfos()	const {return info_data()->m_DisableInfo;}
	
			void									RunScriptActions		(const CGameObject* pOwner)	{info_data()->m_InfoScriptHelper.Action(pOwner, NULL, NULL);}

	//текстовое представление информации
			shared_str								GetText () const ;

protected:
    shared_str		m_InfoId;

	void			load_shared						(LPCSTR);
	SInfoPortionData* info_data						() { VERIFY(inherited_shared::get_sd()); return inherited_shared::get_sd();}
	const SInfoPortionData* info_data				() const { VERIFY(inherited_shared::get_sd()); return inherited_shared::get_sd();}

	static void InitXmlIdToIndex();
};
*/
class CInfoPortion
{
	shared_str			m_InfoId;
public:
	void				Load	(const shared_str& info_id) {m_InfoId = info_id;}
	const shared_str&	InfoId	() const					{return m_InfoId;}
};