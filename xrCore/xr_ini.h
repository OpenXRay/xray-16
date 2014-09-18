#ifndef xr_iniH
#define xr_iniH

#include "fastdelegate.h"

// refs
class	CInifile;
struct	xr_token;


class XRCORE_API CInifile
{
public:
	struct XRCORE_API	Item
	{
		shared_str	first;
		shared_str	second;
//#ifdef DEBUG
//		shared_str	comment;
//#endif
		Item() : first(0), second(0)
//#ifdef DEBUG
//			, comment(0)
//#endif
		{};
	};
	typedef xr_vector<Item>				Items;
	typedef Items::const_iterator		SectCIt;
	typedef Items::iterator				SectIt_;
    struct XRCORE_API	Sect {
		shared_str		Name;
		Items			Data;

		BOOL			line_exist	(LPCSTR L, LPCSTR* val=0);
	};
	typedef	xr_vector<Sect*>		Root;
	typedef Root::iterator			RootIt;
	typedef Root::const_iterator	RootCIt;
	
#ifndef _EDITOR
	typedef fastdelegate::FastDelegate1<LPCSTR, bool>	allow_include_func_t;
#endif
	static CInifile*	Create		( LPCSTR szFileName, BOOL ReadOnly=TRUE);
	static void			Destroy		( CInifile*);
    static IC BOOL		IsBOOL		( LPCSTR B)	{ return (xr_strcmp(B,"on")==0 || xr_strcmp(B,"yes")==0 || xr_strcmp(B,"true")==0 || xr_strcmp(B,"1")==0);}
private:
	enum{eSaveAtEnd = (1<<0), eReadOnly= (1<<1), eOverrideNames=(1<<2),};
	Flags8			m_flags;
	string_path		m_file_name;
	Root			DATA;
	
	void			Load		(IReader* F, LPCSTR path
                                #ifndef _EDITOR
                                    , allow_include_func_t	allow_include_func = NULL
                                #endif
                                );
public:
				CInifile		( IReader* F,
								   LPCSTR path=0
                                #ifndef _EDITOR
								   ,allow_include_func_t allow_include_func = NULL
                                #endif
                                    );

				CInifile		( LPCSTR szFileName,
								  BOOL ReadOnly=TRUE,
								  BOOL bLoadAtStart=TRUE,
								  BOOL SaveAtEnd=TRUE,
								  u32 sect_count=0
                                #ifndef _EDITOR
								   ,allow_include_func_t allow_include_func = NULL
                                #endif
                                    );

	virtual 	~CInifile		( );
    bool		save_as         ( LPCSTR new_fname=0 );
	void		save_as			(IWriter& writer, bool bcheck=false)const;
	void		set_override_names(BOOL b){m_flags.set(eOverrideNames,b);}
	void		save_at_end		(BOOL b){m_flags.set(eSaveAtEnd,b);}
	LPCSTR		fname			( ) const { return m_file_name; };

	Sect&		r_section		( LPCSTR S			)const;
	Sect&		r_section		( const shared_str& S	)const;
	BOOL		line_exist		( LPCSTR S, LPCSTR L )const;
	BOOL		line_exist		( const shared_str& S, const shared_str& L )const;
	u32			line_count		( LPCSTR S			)const;
	u32			line_count		( const shared_str& S	)const;
	u32			section_count	( )const;
	BOOL		section_exist	( LPCSTR S			)const;
	BOOL		section_exist	( const shared_str& S	)const;
	Root&		sections		( ){return DATA;}
	Root const&	sections		( ) const {return DATA;}

	CLASS_ID	r_clsid			( LPCSTR S, LPCSTR L )const;
	CLASS_ID	r_clsid			( const shared_str& S, LPCSTR L )const				{ return r_clsid(*S,L);			}
	LPCSTR 		r_string		( LPCSTR S, LPCSTR L)const;															// оставляет кавычки
	LPCSTR 		r_string		( const shared_str& S, LPCSTR L)const				{ return r_string(*S,L);		}	// оставляет кавычки
	shared_str	r_string_wb		( LPCSTR S, LPCSTR L)const;															// убирает кавычки
	shared_str	r_string_wb		( const shared_str& S, LPCSTR L)const				{ return r_string_wb(*S,L);		}	// убирает кавычки
	u8	 		r_u8			( LPCSTR S, LPCSTR L ) const;
	u8	 		r_u8			( const shared_str& S, LPCSTR L )const				{ return r_u8(*S,L);			}
	u16	 		r_u16			( LPCSTR S, LPCSTR L )const;
	u16	 		r_u16			( const shared_str& S, LPCSTR L )const				{ return r_u16(*S,L);			}
	u32	 		r_u32			( LPCSTR S, LPCSTR L )const;
	u32	 		r_u32			( const shared_str& S, LPCSTR L )const				{ return r_u32(*S,L);			}
	u64	 		r_u64			( LPCSTR S, LPCSTR L )const;
	s8	 		r_s8			( LPCSTR S, LPCSTR L )const;
	s8	 		r_s8			( const shared_str& S, LPCSTR L )const				{ return r_s8(*S,L);			}
	s16	 		r_s16			( LPCSTR S, LPCSTR L )const;
	s16	 		r_s16			( const shared_str& S, LPCSTR L )const				{ return r_s16(*S,L);			}
	s32	 		r_s32			( LPCSTR S, LPCSTR L )const;
	s32	 		r_s32			( const shared_str& S, LPCSTR L )const				{ return r_s32(*S,L);			}
	s64	 		r_s64			( LPCSTR S, LPCSTR L )const;
	float		r_float			( LPCSTR S, LPCSTR L )const;
	float		r_float			( const shared_str& S, LPCSTR L )const				{ return r_float(*S,L);			}
	Fcolor		r_fcolor		( LPCSTR S, LPCSTR L )const;
	Fcolor		r_fcolor		( const shared_str& S, LPCSTR L )const				{ return r_fcolor(*S,L);		}
	u32			r_color			( LPCSTR S, LPCSTR L )const;
	u32			r_color			( const shared_str& S, LPCSTR L )const				{ return r_color(*S,L);			}
	Ivector2	r_ivector2		( LPCSTR S, LPCSTR L )const;
	Ivector2	r_ivector2		( const shared_str& S, LPCSTR L )const				{ return r_ivector2(*S,L);		}
	Ivector3	r_ivector3		( LPCSTR S, LPCSTR L )const;
	Ivector3	r_ivector3		( const shared_str& S, LPCSTR L )const				{ return r_ivector3(*S,L);		}
	Ivector4	r_ivector4		( LPCSTR S, LPCSTR L )const;
	Ivector4	r_ivector4		( const shared_str& S, LPCSTR L )const				{ return r_ivector4(*S,L);		}
	Fvector2	r_fvector2		( LPCSTR S, LPCSTR L )const;
	Fvector2	r_fvector2		( const shared_str& S, LPCSTR L )const				{ return r_fvector2(*S,L);		}
	Fvector3	r_fvector3		( LPCSTR S, LPCSTR L )const;
	Fvector3	r_fvector3		( const shared_str& S, LPCSTR L )const				{ return r_fvector3(*S,L);		}
	Fvector4	r_fvector4		( LPCSTR S, LPCSTR L )const;
	Fvector4	r_fvector4		( const shared_str& S, LPCSTR L )const				{ return r_fvector4(*S,L);		}
	BOOL		r_bool			( LPCSTR S, LPCSTR L )const;
	BOOL		r_bool			( const shared_str& S, LPCSTR L )const				{ return r_bool(*S,L);			}
	int			r_token			( LPCSTR S, LPCSTR L,	const xr_token *token_list)const;
	BOOL		r_line			( LPCSTR S, int L,	LPCSTR* N, LPCSTR* V )const;
	BOOL		r_line			( const shared_str& S, int L,	LPCSTR* N, LPCSTR* V )const;

    void		w_string		( LPCSTR S, LPCSTR L, LPCSTR			V, LPCSTR comment=0 );
	void		w_u8			( LPCSTR S, LPCSTR L, u8				V, LPCSTR comment=0 );
	void		w_u16			( LPCSTR S, LPCSTR L, u16				V, LPCSTR comment=0 );
	void		w_u32			( LPCSTR S, LPCSTR L, u32				V, LPCSTR comment=0 );
	void		w_u64			( LPCSTR S, LPCSTR L, u64				V, LPCSTR comment=0 );
	void		w_s64			( LPCSTR S, LPCSTR L, s64				V, LPCSTR comment=0 );
    void		w_s8			( LPCSTR S, LPCSTR L, s8				V, LPCSTR comment=0 );
	void		w_s16			( LPCSTR S, LPCSTR L, s16				V, LPCSTR comment=0 );
	void		w_s32			( LPCSTR S, LPCSTR L, s32				V, LPCSTR comment=0 );
	void		w_float			( LPCSTR S, LPCSTR L, float				V, LPCSTR comment=0 );
    void		w_fcolor		( LPCSTR S, LPCSTR L, const Fcolor&		V, LPCSTR comment=0 );
    void		w_color			( LPCSTR S, LPCSTR L, u32				V, LPCSTR comment=0 );
    void		w_ivector2		( LPCSTR S, LPCSTR L, const Ivector2&	V, LPCSTR comment=0 );
	void		w_ivector3		( LPCSTR S, LPCSTR L, const Ivector3&	V, LPCSTR comment=0 );
	void		w_ivector4		( LPCSTR S, LPCSTR L, const Ivector4&	V, LPCSTR comment=0 );
	void		w_fvector2		( LPCSTR S, LPCSTR L, const Fvector2&	V, LPCSTR comment=0 );
	void		w_fvector3		( LPCSTR S, LPCSTR L, const Fvector3&	V, LPCSTR comment=0 );
	void		w_fvector4		( LPCSTR S, LPCSTR L, const Fvector4&	V, LPCSTR comment=0 );
	void		w_bool			( LPCSTR S, LPCSTR L, BOOL				V, LPCSTR comment=0 );

    void		remove_line		( LPCSTR S, LPCSTR L );
};

// Main configuration file
extern XRCORE_API CInifile const * pSettings;
extern XRCORE_API CInifile const * pSettingsAuth;

#endif //__XR_INI_H__
