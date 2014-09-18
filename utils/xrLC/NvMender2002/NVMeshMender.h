#ifndef NVMeshMenderH
#define NVMeshMenderH

/*********************************************************************NVMH2****
Copyright (C) 1999, 2000, 2001, 2002 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Questions to sim.dietrich@nvidia.com

Comments:

    This tool is designed to help condition meshes for use in vertex & pixel shaders.

    It can generate normals, texture coordinates, and perhaps most importantly, texture space 
  basis matrices.  It also can fix common texuring problems that come up when bump mapping, including

  Texture Mirroring -  When two one halves of a character use the same part of a texture.

  Cylindrical TexGen - When the rendering relies on cylindrical wrapping, texture space computation
                        won't work right.

  Stretched Basis -    When two adjacend faces use wildly different texture mappings, causing stretching
                        that can ruin per-pixel lighting.
 

  Here is an example usage scenario : 

  Say you have positions & indices & normals, and want textures, and tangents, and binormals.
  and assume that positions, indices, and normals are in STL arrays: vpos, triIndices and vnor respectively.

  xr_vector<float> vpos;
  xr_vector<int> triIndices;
  xr_vector<float> vnor; 
  ...

  NVMeshMender aMender;


  xr_vector<NVMeshMender::VertexAttribute> inputAtts;		// What you have
  xr_vector<NVMeshMender::VertexAttribute> outputAtts;	// What you want.

  NVMeshMender::VertexAttribute posAtt;
  posAtt.Name_ = "position";
  posAtt.floatVector_ = vpos;

  NVMeshMender::VertexAttribute triIndAtt;
  triIndAtt.Name_ = "indices";
  triIndAtt.intVector_ = triIndices;

  NVMeshMender::VertexAttribute norAtt;
  norAtt.Name_ = "normal";
  norAtt.floatVector_ = vnor;

  xr_vector<float> texCoords;
  NVMeshMender::VertexAttribute texCoordAtt;
  texCoordAtt.Name_ = "tex0";
  texCoordAtt.floatVector_;// = texCoords;

  NVMeshMender::VertexAttribute tgtSpaceAtt;
  tgtSpaceAtt.Name_ = "tangent";

  NVMeshMender::VertexAttribute binormalAtt;
  binormalAtt.Name_ = "binormal";

  inputAtts.push_back(posAtt);
  inputAtts.push_back(triIndAtt);
  inputAtts.push_back(norAtt);

  outputAtts.push_back(posAtt);
  outputAtts.push_back(triIndAtt);
  outputAtts.push_back(norAtt);
  outputAtts.push_back(texCoordAtt);
  outputAtts.push_back(tgtSpaceAtt);
  outputAtts.push_back(binormalAtt);

  // All inputs except for indices are assumed to be sets of 3 floats

  // "indices" are assumed to be unsigned ints

  // "tex0" is used for the tangent space calculation

  // "tex0" is the only coordinate set generated, and the texture matrix applies only to it

  //  All unknown attribute types, including "tex1", "tex2", "random_attribute", "weights", "bones", etc.
  // are simply passed through.  They will be duplicated as needed just like positions, normals, etc.

  bool bSuccess = aMender.Munge( inputAtts,              // these are my positions & indices
                                 outputAtts,             // these are the outputs I requested, plus extra stuff generated on my behalf
                                 3.141592654f / 2.5f,    // tangent space smooth angle
                                 NULL,                   // no Texture matrix applied to my tex0 coords
                                 FixTangents,            // fix degenerate bases & texture mirroring
                                 FixCylindricalTexGen    // handle cylidrically mapped textures via vertex duplication
                                 WeightNormalsByFaceSize // weight vertex normals by the triangle's size
                                 );


  if ( !bSuccess ) return false;
  
  vpos = outputAtts[0].floatVector_; // Note that there may be more vertices than you sent in.
  vnor = outputAtts[2].floatVector_;
  xr_vector<float> texCoords = outputAtts[3].floatVector_; // texcoords
  xr_vector<float> vtgt = outputAtts[4].floatVector_;		 // tgts
  triIndices = outputAtts[1].intVector_;					 // new indices.
  xr_vector<float> vbin = outputAtts[5].floatVector_;      // binormals.

  // Now the outputAtts may contain more vertex then you sent in !
  //  This is because in handling tangent space smoothing, and solving texture mirroring & 
  // cylindrical texture wrapping problems, we partially duplicate vertices.

  // All attributes are duplicated, even unknowns.

  //  You may also get things you didn't ask for.  For instance, if you ask for tangent space,
  // in other words, you ask for "tangent" or "binormal",  you will get "normal", "tex0", 
  // "tangent" and "binormal".  You can then ignore things in the output vector that you don't want.

  //   If you ask for FixCylindricalTexGen, it will fix any absolute change in texture coordinates > 0.5 
  // across a single edge.  Therefore, if you pass in a single quad or cube, it won't work.  Any more 
  // complicated or tessellated mesh should work fine.


******************************************************************************/

#pragma warning(disable:4995)
#include <string>
#pragma warning(default:4995)

class NVMeshMender
{
    private :

        mutable xr_vector< xr_string > LastErrors_;


		struct Edge
		{
			unsigned int v0;
			unsigned int v1;

			unsigned int face;
			unsigned int face2;

			bool operator==( const Edge& rhs ) const
			{
                return ( ( v0 == rhs.v0 ) && ( v1 == rhs.v1 ) );
			}

			bool operator<( const Edge& rhs ) const
			{
                if ( v0 < rhs.v0 ) 
                {
                    return true;
                }

                if ( v0 > rhs.v0 )
                {
                    return false;
                }

                return  ( v1 < rhs.v1 );
			}
		};

    public :

        void SetLastError( const xr_string& rhs ) const
        {
            LastErrors_.push_back( rhs );
        }

        xr_string GetLastError() const
        {
            xr_string aString;

            if ( LastErrors_.size() > 0 )
            {
                aString = LastErrors_.back();
            }
            return aString;
        }

        struct VertexAttribute
        {
            xr_string  Name_;

            typedef xr_vector< int > IntVector;
            IntVector intVector_;


            typedef xr_vector< float > FloatVector;
            FloatVector floatVector_;

            VertexAttribute& operator=( const VertexAttribute& rhs )
            {
                Name_   = rhs.Name_;
                intVector_ = rhs.intVector_;
                floatVector_ = rhs.floatVector_;
                return *this;
            }

            VertexAttribute( const char* pName = "" ) : Name_(pName) {;}

            VertexAttribute( const VertexAttribute& rhs )
            {
                *this = rhs;
            }

            bool operator==( const VertexAttribute& rhs )
            {
                return ( Name_ == rhs.Name_ );
            }

            bool operator<( const VertexAttribute& rhs )
            {
                return ( Name_ < rhs.Name_ );
            }

        };

        typedef xr_vector< VertexAttribute > VAVector;

		enum Option
		{
			FixTangents,
			DontFixTangents,

			FixCylindricalTexGen,
			DontFixCylindricalTexGen,

            WeightNormalsByFaceSize,
            DontWeightNormalsByFaceSize
		};

        bool NVMeshMender::Munge( const NVMeshMender::VAVector& input, 
			                   NVMeshMender::VAVector& output, 
							   const float bSmoothCreaseAngleRadians = 3.141592654f / 3.0f,
							   const float* pTextureMatrix = 0,
							   const Option _FixTangents = FixTangents,
							   const Option _FixCylindricalTexGen = FixCylindricalTexGen,
                               const Option _WeightNormalsByFaceSize = WeightNormalsByFaceSize
							   );
		bool NVMeshMender::MungeD3DX( const NVMeshMender::VAVector& input, 
			                   NVMeshMender::VAVector& output, 
							   const float bSmoothCreaseAngleRadians = 3.141592654f / 3.0f,
							   const float* pTextureMatrix = 0,
							   const Option _FixTangents = FixTangents,
							   const Option _FixCylindricalTexGen = FixCylindricalTexGen,
                               const Option _WeightNormalsByFaceSize = WeightNormalsByFaceSize
							   );
};

#endif  //_NVMeshMender_H_

