#ifndef GAMESPY_SAKE
#define GAMESPY_SAKE

#include "GameSpy_FuncDefs.h"

namespace gamespy_sake
{
	
} //namespace gamespy_sake



class CGameSpy_SAKE
{
public:
							CGameSpy_SAKE	(HMODULE hGameSpyDLL);
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
	void										LoadGameSpySAKE				(HMODULE hGameSpyDLL);

	GAMESPY_FN_VAR_DECL(SAKEStartupResult,		sakeStartup,				(SAKE *sakePtr));
	GAMESPY_FN_VAR_DECL(void,					sakeShutdown,				(SAKE sake));
	GAMESPY_FN_VAR_DECL(void,					sakeSetProfile,				(SAKE sake,
																			 int profileId,
																			 const char *loginTicket));
	GAMESPY_FN_VAR_DECL(SAKEStartRequestResult,	sakeGetStartRequestResult,	(SAKE sake));
	GAMESPY_FN_VAR_DECL(SAKERequest,			sakeGetMyRecords,			(SAKE sake,
																			 SAKEGetMyRecordsInput * input,
																			 SAKERequestCallback callback,
																			 void * userData));
	GAMESPY_FN_VAR_DECL(SAKERequest,			sakeCreateRecord,			(SAKE sake,
																			 SAKECreateRecordInput * input,
																			 SAKERequestCallback callback,
																			 void * userdata));
	GAMESPY_FN_VAR_DECL(SAKERequest,			sakeUpdateRecord,			(SAKE sake,
																			 SAKEUpdateRecordInput * input,
																			 SAKERequestCallback callback,
																			 void * userdata));
	GAMESPY_FN_VAR_DECL(SAKERequest,			sakeSearchForRecords,		(SAKE sake,
																			 SAKESearchForRecordsInput * input,
																			 SAKERequestCallback callback,
																			 void * userData));
					
}; //class GameSpy_SAKE

#endif //#ifndef GAMESPY_SAKE
