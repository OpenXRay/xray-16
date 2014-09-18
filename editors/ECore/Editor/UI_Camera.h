//---------------------------------------------------------------------------
#ifndef UI_CameraH
#define UI_CameraH

enum ECameraStyle{
	csPlaneMove=0,
	cs3DArcBall,
    csFreeFly
};

class ECORE_API CUI_Camera{
	ECameraStyle	m_Style;
    bool			m_bMoving;
    TShiftState	 	m_Shift;
    Ivector2		m_StartPos;
    float 			m_FlySpeed;
    float 			m_FlyAltitude;

    Fmatrix			m_CamMat;
    Fvector			m_HPB;
    Fvector			m_Position;
    Fvector			m_Target;
protected:
	friend class	CEditorRenderDevice;
	friend class	TUI;

	float 			m_Znear;
	float 			m_Zfar;
    float			m_SR, m_SM;

    void 			Pan			(float X, float Z);
    void 			Scale		(float Y);
    void 			Rotate		(float X, float Y);
    void			ArcBall		(TShiftState Shift, float X, float Y);
public:
					CUI_Camera	();
    virtual			~CUI_Camera	();

	IC float 		_Znear		(){return m_Znear;}
	IC float 		_Zfar		(){return m_Zfar;}

	void			BuildCamera	();
	void			Reset		();
    void 			Update		(float dt);
    void			SetStyle	(ECameraStyle style);
	ECameraStyle	GetStyle	(){return m_Style;}

    bool			MoveStart	(TShiftState Shift);
    bool			MoveEnd		(TShiftState Shift);
    bool			IsMoving	(){return m_bMoving;}
	bool 			Process		(TShiftState Shift, int dx, int dy);
    bool			KeyDown		(WORD Key, TShiftState Shift);
    bool			KeyUp		(WORD Key, TShiftState Shift);

    void			ViewFront	(){m_HPB.set(0.f,0.f,0.f); 			BuildCamera();}
    void			ViewBack 	(){m_HPB.set(M_PI,0.f,0.f); 		BuildCamera();}
    void			ViewLeft	(){m_HPB.set(-PI_DIV_2,0.f,0.f); 	BuildCamera();}
    void			ViewRight 	(){m_HPB.set(PI_DIV_2,0.f,0.f); 	BuildCamera();}
    void			ViewTop		(){m_HPB.set(0.f,-PI_DIV_2,0.f); 	BuildCamera();}
    void			ViewBottom 	(){m_HPB.set(0.f,PI_DIV_2,0.f); 	BuildCamera();}
    void			ViewReset 	(){m_HPB.set(0.f,0,0.f); m_Position.set(0,0,0); m_Target.set(0,0,0); BuildCamera();}

    const Fmatrix&	GetTransform() const {return m_CamMat;}
    const Fmatrix&	GetView		(Fmatrix& V) const {return V.invert(m_CamMat);}
    const Fvector&	GetHPB		() const {return m_HPB;}
    const Fvector&	GetPosition	() const {return m_Position;}
    const Fvector&	GetRight	() const {return m_CamMat.i;}
    const Fvector&	GetNormal	() const {return m_CamMat.j;}
    const Fvector&	GetDirection() const {return m_CamMat.k;}
    void			Set			(float h, float p, float b, float x, float y, float z);
    void			Set			(const Fvector& hpb, const Fvector& pos);
    void			SetSensitivity(float sm, float sr);
    void			SetViewport	(float _near, float _far, float _fov);
    void			SetDepth	(float _far, bool bForcedUpdate);
    void			SetFlyParams(float speed, float fAltitude){ m_FlySpeed=speed; m_FlyAltitude=fAltitude; }

    void			ZoomExtents	(const Fbox& bb);

	void 			MouseRayFromPoint(Fvector& start, Fvector& direction, const Ivector2& point);
};
#endif
