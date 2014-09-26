#ifndef UI_ShaderMainH
#define UI_ShaderMainH

#include "../ECore/Editor/UI_MainCommand.h"
#include "../ECore/Editor/UI_Main.h"

enum {
	COMMAND_EXTFIRST_EXT = COMMAND_MAIN_LAST-1,
	COMMAND_UPDATE_LIST,
};
//------------------------------------------------------------------------------
class CShaderMain: public TUI{
	typedef TUI inherited;
    
    virtual void 	RealUpdateScene			();
    virtual void 	RealQuit				();
public:
    				CShaderMain 			();
    virtual 		~CShaderMain			();

    virtual LPSTR	GetCaption				();

    virtual void 	ResetStatus				();
    virtual void 	SetStatus				(LPSTR s, bool bOutLog);
    virtual void	ProgressDraw			();
    virtual void 	OutCameraPos			();
    virtual void 	OutUICursorPos			();
    virtual void 	OutGridSize				();
    virtual void 	OutInfo					();

    virtual LPCSTR	EditorName				(){return "shader";}
    virtual LPCSTR	EditorDesc				(){return "Shader Editor";}

    virtual bool 	ApplyShortCut			(WORD Key, TShiftState Shift);
    virtual bool 	ApplyGlobalShortCut		(WORD Key, TShiftState Shift);

    // commands
	virtual	void		RegisterCommands	(); 
};    
extern CShaderMain*&	PUI;

#endif //UI_MainCommandH



