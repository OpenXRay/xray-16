#include "stdafx.h"
#include "build.h"
#include "sector.h"
#include "OGF_Face.h"
xr_vector<CSector*>	g_sectors;

void CBuild::BuildSectors()
{
	Status("Determining sectors...");
	Progress(0);
	u32 SectorMax=0;
	for (u32 I=0; I<g_tree.size(); I++)
		if (g_tree[I]->Sector>SectorMax) SectorMax=g_tree[I]->Sector;
	R_ASSERT(SectorMax<0xffff);

	u32 SectorCount = SectorMax+1; 
	g_sectors.resize(SectorCount);
	ZeroMemory(&*g_sectors.begin(),(u32)g_sectors.size()*sizeof(void*));
	clMsg("%d sectors accepted.",SectorCount);

	Status("Spatializing geometry...");
	for (u32 I=0; I<g_tree.size(); I++)
	{
		u32 Sector = g_tree[I]->Sector;
		if (0==g_sectors[Sector]) g_sectors[Sector] = xr_new<CSector> (Sector);
	}

	Status("Building hierrarhy...");
	for (u32 I=0; I<g_sectors.size(); I++)
	{
		R_ASSERT(g_sectors[I]);
		g_sectors[I]->BuildHierrarhy();
		Progress(float(I)/float(g_sectors.size()));
	}

	Status("Assigning portals, occluders, glows, lights...");
	// portals
	for (u32 I=0; I<portals.size(); I++)
	{
		b_portal &P = portals[I];
		R_ASSERT(u32(P.sector_front)<g_sectors.size());
		R_ASSERT(u32(P.sector_back) <g_sectors.size());
		g_sectors[u32(P.sector_front)]->add_portal	(u16(I));
		g_sectors[u32(P.sector_back)]->add_portal		(u16(I));
	}
	// glows
	for (u32 I=0; I<glows.size(); I++)
	{
		b_glow		&G = glows[I];
		b_material	&M = materials()[G.dwMaterial];
		R_ASSERT(M.sector<g_sectors.size());
		g_sectors[M.sector]->add_glow			(u16(I));
	}
	// lights
	for (u32 I=0; I<L_dynamic.size(); I++)
	{
		b_light_dynamic	&L = L_dynamic[I];
		if (L.data.type == D3DLIGHT_DIRECTIONAL)
		{
			for (u32 j=0; j<g_sectors.size(); j++)
			{
				R_ASSERT(g_sectors[j]);
				g_sectors[j]->add_light(u16(I));
			}
		} else {
			if	(L.sectors.size()) {
				for (u32 j=0; j<L.sectors.size(); j++)
				{
					R_ASSERT	(L.sectors[j]<g_sectors.size());
					g_sectors	[L.sectors[j]]->add_light(u16(I));
				}
			} else {
				clMsg("Fuck!!! Light at position %f,%f,%f non associated!!!",
					L.data.position.x,L.data.position.y,L.data.position.z
					);
			}
		}
	}
}

void CBuild::SaveSectors(IWriter& fs)
{
	CMemoryWriter MFS;
	Status("Processing...");

	// validate & save
	for (u32 I=0; I<g_sectors.size(); I++)
	{
		MFS.open_chunk(I);
		g_sectors[I]->Validate();
		g_sectors[I]->Save(MFS);
		MFS.close_chunk();
		Progress(float(I)/float(g_sectors.size()));
	}

	fs.w_chunk(fsL_SECTORS,MFS.pointer(),MFS.size());
}
