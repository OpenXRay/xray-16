// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef NV_IMAGE_NORMALMAP_H
#define NV_IMAGE_NORMALMAP_H

#include <nvmath/Vector.h>
#include <nvimage/nvimage.h>
#include <nvimage/FloatImage.h>


namespace nv
{
	class Image;

	enum NormalMapFilter
	{
		NormalMapFilter_Sobel3x3,	// fine detail
		NormalMapFilter_Sobel5x5,	// medium detail
		NormalMapFilter_Sobel7x7,	// large detail
		NormalMapFilter_Sobel9x9,	// very large
	};

	FloatImage * createNormalMap(const Image * img, FloatImage::WrapMode wm, Vector4::Arg heightWeights, NormalMapFilter filter = NormalMapFilter_Sobel3x3);

	FloatImage * createNormalMap(const Image * img, FloatImage::WrapMode wm, Vector4::Arg heightWeights, Vector4::Arg filterWeights);

	void normalizeNormalMap(FloatImage * img);

	// @@ Add generation of DU/DV maps.


} // nv namespace

#endif // NV_IMAGE_NORMALMAP_H
