#pragma	once

#include "alife_space.h"

#include "object_interfaces.h"

struct ARTICLE_DATA : public IPureSerializeObject<IReader,IWriter>
{
	enum EArticleType {eEncyclopediaArticle, eJournalArticle, eTaskArticle, eInfoArticle};

	ARTICLE_DATA			()
		:	article_id		(NULL),
			receive_time	(0),
			readed			(false),
			article_type	(eEncyclopediaArticle)
	{}

	ARTICLE_DATA			(shared_str id, ALife::_TIME_ID time, EArticleType articleType)
		:	article_id		(id),
			receive_time	(time),
			readed			(false),
			article_type	(articleType)
	{}
	
	virtual void load (IReader& stream);
	virtual void save (IWriter&);

	ALife::_TIME_ID			receive_time;
	shared_str				article_id;
	bool					readed;

	EArticleType			article_type;
};

DEFINE_VECTOR		(shared_str, ARTICLE_ID_VECTOR, ARTICLE_ID_IT);
DEFINE_VECTOR		(ARTICLE_DATA, ARTICLE_VECTOR, ARTICLE_IT);

class FindArticleByIDPred
{
public:
	FindArticleByIDPred(shared_str id){object_id = id;}
	bool operator() (const ARTICLE_DATA& item)
	{
		if(item.article_id == object_id)
			return true;
		else
			return false;
	}
private:
	shared_str object_id;
};
