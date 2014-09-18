#pragma once

enum EUIItemAlign{
	alNone	= 0x0000,
	alLeft	= 0x0001,
	alRight	= 0x0002,
	alTop	= 0x0004,
	alBottom= 0x0008,
	alCenter= 0x0010
};

class CUICustomItem
{
protected:
	enum {
		flValidRect				=(1<<0),
		flValidOriginalRect		=(1<<1),
		flValidHeadingPivot		=(1<<2),
		flFixedLTWhileHeading	=(1<<3),
	};
	//прямоугольник(в пикселях) 
	//геом. регион  на который натягикается текстура с текстурными координатами iOriginalRect
	Frect			iVisRect;

	//фрейм текстуры в пикселях отн. 0/0
	Frect			iOriginalRect;

	// точка, относительно которой применяем поворот
	Fvector2		iHeadingPivot;
	Fvector2		iHeadingOffset;

	Flags32			uFlags;
	u32				uAlign;

public:
					CUICustomItem			();
	virtual			~CUICustomItem			();
	IC void			SetRect					(float x1, float y1, float x2, float y2){iVisRect.set(x1,y1,x2,y2); uFlags.set(flValidRect,TRUE); }
	IC void			SetRect					(const Frect& r){iVisRect.set(r); uFlags.set(flValidRect, TRUE); }
	  void			SetOriginalRect			(float x, float y, float width, float height);

	IC Frect		GetRect					() {return iVisRect;}
	   Frect		GetOriginalRect			() const;
	
	   void			SetHeadingPivot			(const Fvector2& p, const Fvector2& offset, bool fixedLT);
	   void			ResetHeadingPivot		();
	   IC bool		GetFixedLTWhileHeading	() const								{return !!uFlags.test(flFixedLTWhileHeading);}
	   Fvector2		GetHeadingPivot			()										{return iHeadingPivot;}
	   
	   void			Render					(const Fvector2& pos, u32 color, 
												float x1, float y1, 
												float x2, float y2);

	   void			Render					(const Fvector2& pos, u32 color);
	   void			Render					(const Fvector2& pos, u32 color, float angle);

	IC void			SetAlign				(u32 align)					{uAlign=align;};
	IC u32			GetAlign				()							{return uAlign;}

};
