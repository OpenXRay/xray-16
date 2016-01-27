#ifndef GAMESPY_SAKE
#define GAMESPY_SAKE

#include "xrCore/xrCore.h"
#include "xrGameSpy/xrGameSpy.h"

namespace gamespy_sake
{
	
} //namespace gamespy_sake

class XRGAMESPY_API CGameSpy_SAKE
{
public:
							CGameSpy_SAKE	();
							~CGameSpy_SAKE	();


	void					SetProfile		(int profileId,
											 const char *loginTicket);
	SAKEStartRequestResult	GetRequestResult();
	SAKERequest				GetMyRecords	(SAKEGetMyRecordsInput * input,
											 SAKERequestCallback callback,
											 void * userData);
	SAKERequest				CreateRecord	(SAKECreateRecordInput * input,
											 SAKERequestCallback callback,
											 void * userdata);
	SAKERequest				UpdateRecord	(SAKEUpdateRecordInput * input,
											 SAKERequestCallback callback,
											 void * userdata);
	SAKERequest				SearchForRecords(SAKESearchForRecordsInput * input,
											 SAKERequestCallback callback,
											 void * userData);
	static shared_str const	TryToTranslate	(SAKERequestResult const & request_result);
	static shared_str const	TryToTranslate	(SAKEStartRequestResult const & request_result);
private:
	SAKE								m_sake_inst;
	
	void										Init						();
}; //class GameSpy_SAKE

#endif //#ifndef GAMESPY_SAKE
