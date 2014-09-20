//---------------------------------------------------------------------------
#ifndef Cursor3DH
#define Cursor3DH
//---------------------------------------------------------------------------
#define CURSOR_PRECISION_SEGMENT 16

enum ECursorStyle{
	csLasso,
    csPoint
};

class C3DCursor{
    float   d_angle;
    u32	dwColor;
    FvectorVec m_RenderBuffer;

    Fvector brush_start, brush_dir;
    Fmatrix brush_mat;
    float   brush_radius;
    float   brush_up_depth, brush_dn_depth;

    bool    m_Visible;
    void    GetPickPoint (Fvector& src, Fvector& dst, Fvector* N);
    ECursorStyle eStyle;
public:
            C3DCursor   ();
    virtual ~C3DCursor  ();
    void    SetBrushSegment(float segment=CURSOR_PRECISION_SEGMENT);
    void    Render      ();
    bool	GetVisible	(){return m_Visible;}

    float   GetBrushSize(){return brush_radius;}
    void    SetBrushRadius(float r){brush_radius=r;}
    void    SetBrushDepth(float up_d, float dn_d){brush_up_depth=up_d;brush_dn_depth=dn_d;}
    void    SetColor	(Fcolor& c);
    bool    PrepareBrush();
    void    GetRandomBrushPos(Fvector& pos, Fvector& norm);
//.    void    Visible     (bool b){m_Visible = b;}
    void	Style		(ECursorStyle st){eStyle=st;}
};
#endif
