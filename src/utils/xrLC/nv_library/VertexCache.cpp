#include "stdafx.h"
#include "VertexCache.h"

VertexCache::VertexCache()
{
  VertexCache(16);
}


VertexCache::VertexCache(int size)
{
	entries.assign	(size,-1);
}


VertexCache::~VertexCache()
{
	entries.clear	();
}


int VertexCache::At	(int index)
{
  return entries[index];
}

void VertexCache::Set(int index, int value)
{
	entries[index] = value;
}


void VertexCache::Clear()
{
	for(u32 i = 0; i < entries.size(); i++)
		entries[i] = -1;
}

void VertexCache::Copy(VertexCache* inVcache)
{
	for(u32 i = 0; i < entries.size(); i++)
	{
		inVcache->Set(i, entries[i]);
	}
}
