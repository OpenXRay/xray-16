// Sector.h: interface for the CSector class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SECTOR_H__751706E5_383E_40CB_9F3D_6A4D1BB8F3CD__INCLUDED_)
#define AFX_SECTOR_H__751706E5_383E_40CB_9F3D_6A4D1BB8F3CD__INCLUDED_
#pragma once
struct OGF_Base;

class CSector  
{
	u32					SelfID;
	OGF_Base *			TreeRoot;
	xr_vector<u16>		Portals;
	xr_vector<u16>		Glows;
	xr_vector<u16>		Lights;
public: 
	void add_portal		(u16 P)		{ Portals.push_back(P);		}
	void add_glow		(u16 G)		{ Glows.push_back(G);		}
	void add_light		(u16 L)		{ Lights.push_back(L);		}

	void BuildHierrarhy	();
	void Validate		();
	void Save			(IWriter& fs);

	CSector				(u32 ID);
	~CSector			();
};

extern xr_vector<CSector*>	g_sectors;

#endif // !defined(AFX_SECTOR_H__751706E5_383E_40CB_9F3D_6A4D1BB8F3CD__INCLUDED_)
