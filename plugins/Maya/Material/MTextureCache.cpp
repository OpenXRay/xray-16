///////////////////////////////////////////////////////////////////
// DESCRIPTION: Texture cache, used to temporarily store textures.
//				Eventually, we'll likely expose the actual Maya
//				texture cache to plug-ins to improve memory utilization
//				and performance.
//
// PS: Thanks to sbrew@lucasarts.com for contributing several fixes 
// to this class! ;-)
//
///////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "MTextureCache.h"
#include "NodeMonitor.h"

// Initialize the singleton instance, and the refcount is originally 0.
MTextureCache* MTextureCache::m_instance = NULL;
/*static*/ int MTextureCache::refcount = 0;

MTextureCacheElement::~MTextureCacheElement()
{ 
	if (m_texture) 
	{
		xr_delete(m_texture);
		m_texture = NULL;
	}
}


MTextureCache::~MTextureCache()
{
	if (m_textureTable.empty())
		return;

	// Delete all texture cache elements.
	//
	string_to_cacheElement_map::iterator p = m_textureTable.begin();
	for ( ; p != m_textureTable.end(); ++p)
	{
		xr_delete(p->second);
	}
}

// Return a reference to the texture. Need to dereference by calling "release".
MTexture* MTextureCache::texture(MObject textureObj, 
			 MTexture::Type type /* = MTexture::RGBA */, 
			 bool mipmapped /* = true */,
			 GLenum target /* = GL_TEXTURE_2D */)
{
	// Get the name of the texture object.
	MString textureName = getNameFromObj(textureObj);
	
	// If this isn't a file texture node, or if it has no valid name, 
	// return NULL.
	if (!textureObj.hasFn(MFn::kFileTexture) ||
		textureName == "")
		return NULL;

	// Check if we already have a texCacheElement assigned to the given texture name.
	MTextureCacheElement *texCacheElement = 
		m_textureTable[textureName.asChar()];
	bool newTexture = !texCacheElement;
	bool textureDirty = texCacheElement && texCacheElement->fMonitor.dirty();

	if (textureDirty)
	{
		texCacheElement->fMonitor.stopWatching();
		xr_delete(texCacheElement->m_texture);
		texCacheElement->m_texture = NULL;
	}

	if (newTexture)
	{
		texCacheElement = xr_new<MTextureCacheElement>();

		texCacheElement->fMonitor.setManager(this);
		
		// Add it to the map.
		m_textureTable[textureName.asChar()] = texCacheElement;
	}

	if (textureDirty || newTexture)
	{
		// Get the filename of the file texture node.
		MString textureFilename;
		MFnDependencyNode textureNode(textureObj);
		MPlug filenamePlug( textureObj, 
			textureNode.attribute(MString("fileTextureName")) );
		filenamePlug.getValue(textureFilename);
		
		// Create the MTexture
		texCacheElement->m_texture = xr_new<MTexture>();

		// Monitor the given texture node for "dirty" or "rename" messages.
		texCacheElement->fMonitor.watch(textureObj);

		// Attempt to load the texture from disk and bind it in the OpenGL driver. 
		if (texCacheElement->m_texture->load(textureFilename, 
								type, mipmapped, target) == false)
		{
			// An error occured. Most likely, it was impossible to 
			// open the given filename.
			// Clean up and return NULL.
			xr_delete(texCacheElement);
			texCacheElement = NULL;
			m_textureTable.erase(textureName.asChar());
			return NULL;
		}
	}

	// Update the last updated timestamp.
	texCacheElement->lastAccessedTimestamp = m_currentTimestamp;

	return texCacheElement->texture();
}

// Returns true if the texture was found and bound; 
// returns false otherwise.
bool MTextureCache::bind(MObject textureObj, 
						 MTexture::Type type /* = MTexture::RGBA */, 
						 bool mipmapped /* = true */,
 						 GLenum target /* = GL_TEXTURE_2D */)
{
	// Get a reference to the texture, allocating it if necessary.
	MTexture* pTex = texture(textureObj, type, mipmapped, target);

	if (pTex)
	{
		// bind the texture.
		pTex->bind();
	
		return true;
	}
	
	return false;
}

void MTextureCache::onNodeRenamed(MObject& node, MString oldName, MString newName)
{
	// Remove the texture from the cache.
	MTextureCacheElement *texCacheElement = m_textureTable[oldName.asChar()];
	xr_delete(texCacheElement->m_texture);
	texCacheElement->m_texture = NULL;
}

void MTextureCache::incrementTimestamp(unsigned int increment /* =  1 */)
{
	m_currentTimestamp += increment;
	
	// Optionally, go through all textures and get rid of each of them that
	// is too old.
}
