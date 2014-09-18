#ifndef MAYA_API_MTexture
#define MAYA_API_MTexture

///////////////////////////////////////////////////////////////////
// DESCRIPTION: Texture object, that can be mipmapped. Eventually, this
//				class will likely end up in the Maya API.
//
// AUTHOR: Christian Laforte
//
///////////////////////////////////////////////////////////////////

#include <maya/MImage.h>
#include <maya/MString.h>
#include <assert.h>

#include <GL/gl.h>
#include <GL/glu.h>

int highestPowerOf2(int num);

class TexObj
{
public:
	TexObj(GLenum target = GL_TEXTURE_2D)
	{
		assert(glGetError() == GL_NO_ERROR);

		// Get a texture identifier.
		glGenTextures(1, &fTextureNum);
		assert(glGetError() == GL_NO_ERROR);

		fTarget = target;

		// Set up default values for the texture parameters.
		// They are the same as the OpenGL default.
		fMinFilterParam = GL_NEAREST;
		fMagFilterParam = GL_LINEAR;
		fWrapSParam = GL_REPEAT;
		fWrapTParam = GL_REPEAT;
	}

	~TexObj()
	{
		glDeleteTextures(1, &fTextureNum);
	}

	void bind()
	{
		assert(glGetError() == GL_NO_ERROR);

		// Bind the texture.
		glBindTexture(fTarget, fTextureNum);
		assert(glGetError() == GL_NO_ERROR);

		// Set up the texture parameters.
		glTexParameteri(fTarget, GL_TEXTURE_MIN_FILTER, fMinFilterParam);
		assert(glGetError() == GL_NO_ERROR);
		glTexParameteri(fTarget, GL_TEXTURE_MAG_FILTER, fMagFilterParam);
		assert(glGetError() == GL_NO_ERROR);
		glTexParameteri(fTarget, GL_TEXTURE_WRAP_S, fWrapSParam);
		assert(glGetError() == GL_NO_ERROR);
		glTexParameteri(fTarget, GL_TEXTURE_WRAP_T, fWrapTParam);
		assert(glGetError() == GL_NO_ERROR);
	}

	void parameter(GLenum pname, GLint param)
	{
		switch (pname)
		{
		case GL_TEXTURE_MIN_FILTER:	fMinFilterParam = param;	break;
		case GL_TEXTURE_MAG_FILTER:	fMagFilterParam = param;	break;
		case GL_TEXTURE_WRAP_S:		fWrapSParam = param;		break;
		case GL_TEXTURE_WRAP_T:		fWrapTParam = param;		break;
		}
	}

private:
	GLenum fTarget;

	// Various parameters. See glTexParameterf() in MSDN for more info.
	GLint fMinFilterParam;
	GLint fMagFilterParam;
	GLint fWrapSParam;
	GLint fWrapTParam;
	
	GLuint fTextureNum;
};

class MTexture
{
public:
	enum Type
	{
		RGBA,
		HILO,
		NMAP
	};

	MTexture();

	~MTexture()
	{
		if (m_levels)
		{
			for (unsigned int i=0; i < m_numLevels; i++)
			{
				if (m_levels[i])
				{
					xr_free(m_levels[i]);
				}
			}
			
			xr_free(m_levels);
		}
	}

	bool set(MImage &image, Type type, bool mipmapped = true, GLenum target = GL_TEXTURE_2D);

	// This function assumes that the file texture is square, and
	// that its dimensions are exponents of 2.
	bool load(MString filename, Type type, bool mipmapped = true, GLenum target = GL_TEXTURE_2D);
	
	bool specify(GLenum target);

	// Returns 1 if no mipmapping, >1 otherwise.
	unsigned int levels() { return m_numLevels; }

	bool bind();

	unsigned char* fetch(unsigned int s, unsigned int t, unsigned int level = 0)
	{
		// Verify that the mipmap level exists.
		if (level > m_numLevels || m_levels == NULL || m_levels[level] == NULL)
			return NULL;
		
		return internalFetch(s, t, level);
	}
	
	inline bool square()
	{
		return m_width == m_height;
	}

	inline bool mipmapped()
	{
		return levels() > 1;
	}

	// Return the width of a specific mipmap level.
	// If level == 0, return the width of the base level (source image).
	// Width is always >= 1, to prevent non-square textures from having zero-sized levels.
	inline unsigned int width(unsigned int level = 0)
	{
		unsigned int w = m_width >> level;
		if (w > 0)
			return w;
		else
			return 1;
	}
	
	inline unsigned int height(unsigned int level = 0)
	{
		unsigned int h = m_height >> level;
		if (h > 0)
			return h;
		else
			return 1;
	}

protected:

	inline unsigned char* internalFetch(unsigned int s, unsigned int t, unsigned int level)
	{
		assert((s >= 0) && (s < width(level)));
		assert((t >= 0) && (t < height(level)));
		return m_levels[level] + 4 * ((width(level) * t) + s);
	}


private:

	// Fairly permanent variables	
	unsigned int m_width, m_height;
	Type m_type;
	TexObj m_texObj;
	bool m_mipmapped;
	
	// Pyramid levels (assumes 4 bytes per pixel for now)
	unsigned char **m_levels;
	unsigned int m_numLevels;	// Number of mipmaps + base texture

	// Cached variables (Depend on previous private variables)
	GLint		m_internalFormat;
	GLenum		m_format;
	GLenum		m_componentFormat;
};


#endif // MAYA_API_MTexture
