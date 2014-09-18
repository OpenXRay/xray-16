// MTexture.cpp
//
// Copyright (C) 2000-2003 Alias|Wavefront, a division of Silicon Graphics Limited.
// 
// The information in this file is provided for the exclusive use of the
// licensees of Alias|Wavefront.  Such users have the right to use, modify,
// and incorporate this code into other products for purposes authorized
// by the Alias|Wavefront license agreement, without fee.
// 
// ALIAS|WAVEFRONT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
// INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
// EVENT SHALL ALIAS|WAVEFRONT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
// CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
// DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
// TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.


///////////////////////////////////////////////////////////////////
// DESCRIPTION: Texture object, that can be mipmapped. Eventually,
//				this class will likely end up in the Maya API.
//
// AUTHOR: Christian Laforte
//
///////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "MTexture.h"
#include "MNormalMapConverter.h"

MTexture::MTexture()
{
	// Initialize everything
	m_levels = NULL;
	m_numLevels = 0;
}

bool MTexture::load(MString filename, 
					MTexture::Type type, 
					bool mipmapped /* = true */,
					GLenum target /* = GL_TEXTURE_2D */)
{
	MImage image;
	MStatus stat = image.readFromFile(filename);
	if (!stat)
	{
		MGlobal::displayWarning("In MTexture::load(), file not found: \"" + filename + "\".");
		return false;
	}

	return set( image, type, mipmapped, target );
}

bool MTexture::set(MImage &image, Type type, 
				   bool mipmapped /* = true */, 
				   GLenum target /* = GL_TEXTURE_2D) */)
{
	unsigned int i;	// used as a temporary index.

	// Store the type of texture, and derive other parameters.
	// (Depth is assumed to be 4 bytes per pixel RGBA.
	// MImage always returns that pixel format anyway.)
	m_type = type;
	if ( (m_type == RGBA) || (m_type == NMAP) )
	{
		m_internalFormat = GL_RGBA8;
		m_format = GL_RGBA;
		m_componentFormat = GL_UNSIGNED_BYTE;
	}
	else if (m_type == HILO)
	{
#if NVIDIA_SPECIFIC
		m_internalFormat = GL_SIGNED_HILO_NV;
		m_format = GL_HILO_NV;
		m_componentFormat = GL_SHORT;
#endif
	}
	else assert(0);


	// Get the dimension of the texture.
	MStatus stat = image.getSize(m_width, m_height);
	assert(stat);
	m_mipmapped = mipmapped;

	unsigned int maxWidthLevels  = highestPowerOf2(m_width);
	unsigned int maxHeightLevels = highestPowerOf2(m_height);

	// Standard OpenGL doesn't accept width or height that are not power of 2.
	// If that's the case we resize the picture to the closest larger valid rectangle.
	bool widthIsExponent = (m_width == (unsigned int) (1 << maxWidthLevels));
	bool heightIsExponent = (m_height == (unsigned int) (1 << maxHeightLevels));

	if (!widthIsExponent || !heightIsExponent)
	{
		// Calculate the new width/height.
		if (!widthIsExponent)
			maxWidthLevels++;
		if (!heightIsExponent)
			maxHeightLevels++;

		// Resize the image, without bothering to preserve the aspect ratio.
		m_width = 1 << maxWidthLevels;
		m_height = 1 << maxHeightLevels;
		image.resize(m_width, m_height, false);
	}

	// Deallocate any existing levels
	if (m_levels != NULL)
	{
		for (i=0; i < m_numLevels; i++)
		{
			if (m_levels[i])
			{
				xr_free(m_levels[i]);
				m_levels[i] = NULL;
			}
		}		
		xr_free(m_levels);
	}

	// The number of mipmap levels cannot be greater than the exponent of width or height.
	// The number of mipmap levels is 1 for a non-mipmapped texture.
	// For mipmapped textures, m_numLevels = max level + 1.
	m_numLevels = mipmapped ? _max(maxWidthLevels, maxHeightLevels) + 1 : 1;

	// Allocate the proper amount of memory, for the base level and the mipmaps.
	m_levels = xr_alloc<unsigned char*>(m_numLevels);
	for (i=0; i < m_numLevels; i++)
	{
		m_levels[i] = xr_alloc<unsigned char>(width(i) * height(i) * 4);
	}

	// Copy the base level. (the actual file texture)
	Memory.mem_copy(m_levels[0], image.pixels(), m_width * m_height * 4);
	
	// Create the mipmapped levels.
	// NOTE REGARDING THE width_ratio and height_ratio:
	// 	   The smallest mipmap levels of non-square textures must be handled
	// carefully. Say we have a 8x2 texture. Mipmap levels will be
	// 4x1, 2x1, 1x1. We cannot simply multiply the current st coordinate by
	// 2 like we do for square textures to find the source st coordinates, 
	// or we'll end up fetching outside of the source level. Instead, we
	// multiply the target s, t coordinates by the width and height ratio respectively.
	for (unsigned int current_level = 1; current_level < m_numLevels; current_level++)
	{
		unsigned int width_ratio = width(i-1) / width(i);
		unsigned int height_ratio = height(i-1) / height(i-1);
		unsigned int previous_level = current_level - 1;

		for (unsigned int target_t = 0; target_t < height(current_level); target_t++)
		{
			for (unsigned int target_s = 0; target_s < width(current_level); target_s++)
			{
				// The st coordinates from the source level.
				unsigned int source_s = target_s * width_ratio;
				unsigned int source_t = target_t * height_ratio;
				unsigned int source_s2 = source_s + ((width_ratio == 2) ? 1 : 0);
				unsigned int source_t2 = source_t + ((height_ratio == 2) ? 1 : 0);

				unsigned char *destination	= internalFetch(target_s,	target_t,	current_level);
				unsigned char *source1		= internalFetch(source_s,	source_t,	previous_level);
				unsigned char *source2		= internalFetch(source_s2,	source_t,	previous_level);
				unsigned char *source3		= internalFetch(source_s,	source_t2,	previous_level);
				unsigned char *source4		= internalFetch(source_s2,	source_t2,	previous_level);

				// Average byte per byte.
				unsigned int average1 = (*source1++ + *source2++ + *source3++ + *source4++) / 4;
				*destination++ = average1;

				unsigned int average2 = (*source1++ + *source2++ + *source3++ + *source4++) / 4;
				*destination++ = average2;

				unsigned int average3 = (*source1++ + *source2++ + *source3++ + *source4++) / 4;
				*destination++ = average3;

				unsigned int average4 = (*source1++ + *source2++ + *source3++ + *source4++) / 4;
				*destination++ = average4;
			}
		}
	}

	if( type == NMAP )
	{
		// Convert each level to the NORMAL map format
		//
		MNormalMapConverter	mapConverter;

		for (unsigned int i = 0; i < m_numLevels; i++)
		{
			mapConverter.convertToNormalMap( m_levels[i], width(i), height(i), MNormalMapConverter::RGBA, 2.0f );
		}
	}

	specify(target);

	return true;
}

bool MTexture::specify(GLenum target /* = GL_TEXTURE_2D */)
{
	assert(glGetError() == GL_NO_ERROR);

	m_texObj.bind();

	assert(glGetError() == GL_NO_ERROR);

	for (unsigned int i=0; i < m_numLevels; i++)
	{
		glTexImage2D(target, i, m_internalFormat, width(i), height(i), 0,
					 m_format, m_componentFormat, m_levels[i]);

		assert(glGetError() == GL_NO_ERROR);
	}

	if (mipmapped())
	{
		// Mipmapping enabled
		m_texObj.parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		assert(glGetError() == GL_NO_ERROR);

		m_texObj.parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		assert(glGetError() == GL_NO_ERROR);
	}
	else
	{
		m_texObj.parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		m_texObj.parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	m_texObj.parameter(GL_TEXTURE_WRAP_S, GL_CLAMP);
	m_texObj.parameter(GL_TEXTURE_WRAP_T, GL_CLAMP);

	return true;
}

bool MTexture::bind()
{
	m_texObj.bind();
	//specify(GL_TEXTURE_2D);

	return true;
}


int highestPowerOf2(int num)
{
	int power = 0;

	while (num > 1)
	{
		power++;
		num = num >> 1;
	}

	return power;
}


