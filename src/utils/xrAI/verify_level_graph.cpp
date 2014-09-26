////////////////////////////////////////////////////////////////////////////
//	Module 		: verify_level_graph.cpp
//	Created 	: 25.05.2004
//  Modified 	: 25.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Verifying level graph
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "level_graph.h"

CLevelGraph::CVertex	**stack_storage;

#define PUSH(a) *stack_iterator++ = (a)
#define POP()	vertex = *--stack_iterator

void floodfill			(const CLevelGraph &level_graph, xr_vector<bool> &marks, u32 start_vertex_id)
{
	CLevelGraph::CVertex		**stack_iterator = stack_storage;
	CLevelGraph::CVertex		*vertex = 0;
	CLevelGraph::const_iterator	I, E;
	u32							vertex_id;
	PUSH						(level_graph.vertex(start_vertex_id));
	while (stack_iterator != stack_storage) {
		POP						();
		vertex_id				= level_graph.vertex_id(vertex);
		if (marks[vertex_id])
			continue;
		marks[vertex_id]		= true;
		level_graph.begin		(vertex,I,E);
		for ( ; I != E; ++I) {
			u32					neighbour_vertex_id = level_graph.value(vertex,I);
			if (level_graph.valid_vertex_id(neighbour_vertex_id) && !marks[neighbour_vertex_id])
				PUSH			(level_graph.vertex(neighbour_vertex_id));
		}
	}
}

IC	bool single_link(const CLevelGraph &level_graph, u32 i1, u32 i2, u32 link_index)
{
	return						(level_graph.value(level_graph.vertex(i1),link_index) != i2);
}

bool verify_invalid_links	(const CLevelGraph &graph)
{
	bool								result = true;
	CLevelGraph::const_vertex_iterator	I = graph.begin();
	CLevelGraph::const_vertex_iterator	E = graph.end();
	for ( ; I != E; ++I) {
		u32								vertex_id = graph.vertex_id(I);
		CLevelGraph::const_iterator		i,e;
		graph.begin						(I,i,e);
		for ( ; i != e; ++i) {
			u32							link_vertex_id = graph.value(I,i);
			if (!graph.valid_vertex_id(link_vertex_id))
				continue;

			if (vertex_id == link_vertex_id) {
				Msg						("Vertex [%d][%f][%f][%f] has link to itself",vertex_id,VPUSH(graph.vertex_position(I)));
				result					= false;
				continue;
			}
		}
	};
	return								(result);
}

void verify_level_graph	(LPCSTR name, bool verbose)
{
	Msg				("Verifying level %s",name);
	Phase			("Verifying level graph");
	Progress		(0.f);
	CLevelGraph		*level_graph = xr_new<CLevelGraph>(name);
	if (!level_graph->header().vertex_count()) {
		Progress	(1.f);
		Msg			("Level graph is empty!");
		xr_delete	(level_graph);
		return;
	}

	if (!verify_invalid_links(*level_graph)) {
		Progress	(1.f);
		Msg			("AI map is CORRUPTED : REGENERATE AI-MAP");
		xr_delete	(level_graph);
		return;
	}

	stack_storage	= (CLevelGraph::CVertex**)xr_malloc(level_graph->header().vertex_count()*sizeof(CLevelGraph::CVertex*));

	xr_vector<bool>	marks;

	xr_vector<u32>	single_links;
	single_links.reserve(level_graph->header().vertex_count());
	Progress		(0.05f);

	for (u32 i=0, n=level_graph->header().vertex_count(); i<n; ++i) {
		CLevelGraph::const_iterator	I, E;
		CLevelGraph::CVertex		*vertex = level_graph->vertex(i);
		level_graph->begin			(vertex,I,E);
		for ( ; I != E; ++I) {
			u32						neighbour_vertex_id = level_graph->value(vertex,I);
			if (level_graph->valid_vertex_id(neighbour_vertex_id) && single_link(*level_graph,neighbour_vertex_id,i,(I + 2)%4)) {
				single_links.push_back	(neighbour_vertex_id);
			}
		}
		Progress					(0.05f + 0.05f*float(i)/float(n));
	}

	bool							no_single_links = single_links.empty();
	Progress						(0.1f);
	if (single_links.empty())
		single_links.push_back		(0);

	{
		std::sort					(single_links.begin(),single_links.end());
		xr_vector<u32>::iterator	I = std::unique(single_links.begin(),single_links.end());
		single_links.erase			(I,single_links.end());
	}

	if (!no_single_links) {
		if (verbose) {
			xr_vector<u32>::const_iterator	I = single_links.begin();
			xr_vector<u32>::const_iterator	E = single_links.end();
			for ( ; I != E; ++I)
				Msg					("Vertex %d[%f][%f][%f] is single linked!",*I,VPUSH(level_graph->vertex_position(*I)));
		}
		Msg							("There are %d single linked nodes!",single_links.size());
	}

	Progress						(0.15f);
	bool							valid = true;
	xr_vector<u32>::const_iterator	I = single_links.begin();
	xr_vector<u32>::const_iterator	E = single_links.end();
	for (u32 i=0, n = single_links.size(); I != E; ++I, ++i) {
		marks.assign	(level_graph->header().vertex_count(),false);
		floodfill		(*level_graph,marks,*I);
		xr_vector<bool>::const_iterator	II = marks.begin(), BB = II;
		xr_vector<bool>::const_iterator	EE = marks.end();
		for ( ; II != EE; ++II)
			if (!*II) {
				valid	= false;
				Msg		("AI-map is NOT valid :\nNode \n%6d[%f][%f][%f]\ncannot be reached from the node\n%6d[%f][%f][%f]\n",u32(II - BB),VPUSH(level_graph->vertex_position(u32(II - BB))),*I,VPUSH(level_graph->vertex_position(*I)));
				break;
			}

		if (!valid)
			break;
		Progress		(0.15f + 0.85f*float(i)/float(n));
	}

	xr_free			(stack_storage);
	xr_delete		(level_graph);
	Progress		(1.f);
	if (valid)
		Msg			("AI-map is valid!");

	Msg				("Verifying level %s completed",name);
}