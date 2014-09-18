#ifndef MAYA_API_MTextureCache
#define MAYA_API_MTextureCache


///////////////////////////////////////////////////////////////////
// DESCRIPTION: Texture cache, used to temporarily store textures.
//				Eventually, this class will likely end up in the 
//				Maya API.
//
//				This class is not currently thread-safe.
//
// AUTHOR: Christian Laforte
//
///////////////////////////////////////////////////////////////////

#ifdef WIN32
#pragma warning( disable : 4786 )		// Disable stupid STL warnings.
#endif

#include <maya/MObject.h>
#include <map>
#include <string>
#include <algorithm>

#include "MTexture.h"
#include "NodeMonitor.h"


class MTextureCache;

class MTextureCacheElement
{
friend class MTextureCache;

public:
	MTextureCacheElement()
	{
		lastAccessedTimestamp = -1; 
		m_texture = NULL; 
	}
	
	~MTextureCacheElement();

	MTexture* texture() { return m_texture; }

private:
	MTexture* m_texture;
	unsigned int lastAccessedTimestamp;		// can be used to track when the texture was last used.
	NodeMonitor fMonitor;
};

// This class implements a singleton node with reference counting.
// The refcount starts with a value equal to 0. Everytime instance()
// gets called, the refcount is incremented by one. Everytime
// release() gets called, the refcount is decremented by one,
// and if following that the refcount value is 0, the texture cache
// singleton is destroyed.
class MTextureCache : public NodeMonitorManager
{
protected:
//	MTextureCache()
//	{
//		m_currentTimestamp = 0;
//	}

public:
	MTextureCache()
	{
		m_currentTimestamp = 0;
	}
	~MTextureCache();

	static MTextureCache* instance()
	{
		if (!m_instance)
		{
			m_instance = xr_new<MTextureCache>();
		}

		refcount++;

		return m_instance;
	}

	static void release()
	{
		assert(m_instance);

		refcount--;

		if (refcount == 0 && m_instance)
		{
			xr_delete(m_instance);
			m_instance = NULL;
		}
	}

	// Return a reference to the texture. There's no reference counting yet.
	MTexture* texture(MObject textureObj, 
				 MTexture::Type type = MTexture::RGBA, 
				 bool mipmapped = true,
 				 GLenum target = GL_TEXTURE_2D);

	// Returns true if the texture was found and bound; returns false otherwise.
	bool bind(MObject textureObj, 
			  MTexture::Type type = MTexture::RGBA, 
			  bool mipmapped = true,
  			  GLenum target = GL_TEXTURE_2D);

	void incrementTimestamp(unsigned int increment=1);

	// Called by a node monitor when the watched node is renamed.
	void onNodeRenamed(MObject& node, MString oldName, MString newName);

private:
	static int refcount;

	xr_map<std::string, MTextureCacheElement*> m_textureTable;
	typedef xr_map<std::string, MTextureCacheElement*> string_to_cacheElement_map;

	unsigned int m_currentTimestamp;

	static MTextureCache* m_instance;
};



#endif // MAYA_API_MTextureCache