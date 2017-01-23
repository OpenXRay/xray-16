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

  Say you have positions & indices, and want textures, normals, and tangents.

  NVMeshMender aMender;

  MVMeshMender::VAVector inputAtts;  // this is what you have

  MVMeshMender::VAVector outputAtts; // this is what you want

  MVMeshMender::VertexAttribute att;  // this is my working attribute

  att.Name_ = "position";

  for ( int p = 0; p < number_of_vertices; ++p )
  {
    att.floatVector_.push_back( myPositions[ p ].x );
    att.floatVector_.push_back( myPositions[ p ].y );
    att.floatVector_.push_back( myPositions[ p ].z );
  }

  att.floatVector.clear();

  att.Name_ = "indices";

  for ( int i = 0; i < number_of_indices; ++i )
  {
    att.intVector_.push_back( myIndices[ i ] );
  }

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

#include "stdafx.h"
#pragma hdrstop

#include "NVMeshMender.h"
#include "nv_math.h"

bool NVMeshMender::Munge(  const NVMeshMender::VAVector& input, 
			               NVMeshMender::VAVector& output, 
					       const float bSmoothCreaseAngleRadians,
					       const float* pTextureMatrix,
					       const Option _FixTangents,
					       const Option _FixCylindricalTexGen,
                           const Option _WeightNormalsByFaceSize )
{
    typedef xr_map< xr_string, unsigned int > Mapping;
	typedef xr_set< Edge > EdgeSet;
    typedef xr_vector< xr_set< unsigned int > > IdenticalVertices;

    IdenticalVertices IdenticalVertices_;

    // make room for potential tex coords, normals, binormals and tangents
    output.resize( input.size() + 4 ); 

    Mapping inmap;
    Mapping outmap;

    for ( unsigned int a = 0; a < input.size(); ++a )
    {
        inmap[ input[ a ].Name_ ] = a;
    }

    for ( unsigned int b = 0; b < output.size(); ++b )
    {
        output[ b ].intVector_.clear();
        output[ b ].floatVector_.clear();
        outmap[ output[ b ].Name_ ] = b;
    }

    for ( unsigned int c = 0; c < output.size(); ++c )
    {
        // for every output that has a match in the input, just copy it over
        Mapping::iterator in = inmap.find( output[ c ].Name_ );
        if ( in != inmap.end() )
        {
            // copy over existing indices, position, or whatever
            output[ c ] = input[ (*in).second ];
        }
    }

    if ( inmap.find( "indices" ) == inmap.end() )
    {
        SetLastError( "Missing indices from input" );
        return false;
    }
    if ( outmap.find( "indices" ) == outmap.end() )
    {
        SetLastError( "Missing indices from output" );
        return false;
    }

    // Go through all required outputs & generate as necessary
    if ( inmap.find( "position" ) == inmap.end() )
    {
        SetLastError( "Missing position from input" );
        return false;
    }
    if ( outmap.find( "position" ) == outmap.end() )
    {
        SetLastError( "Missing position from output" );
        return false;
    }

    Mapping::iterator pos = outmap.find( "position" );
    VertexAttribute::FloatVector& positions = output[ (*pos).second ].floatVector_;
    vec3* pPositions = (vec3*)( &( positions[ 0 ] ) );

    xr_set< unsigned int > EmptySet;

    for ( unsigned int i = 0; i < positions.size(); i += 3 )
    {
        IdenticalVertices_.push_back( EmptySet );
    }

	// initialize all attributes
	for ( unsigned int att = 0; att < output.size(); ++att )
	{
		if ( output[ att ].Name_ != "indices" )
		{
			if ( output[ att ].floatVector_.size() == 0 )
			{
				output[ att ].floatVector_ = positions;
			}
		}
	}

    Mapping::iterator ind = outmap.find( "indices" );
    VertexAttribute::IntVector& indices = output[ (*ind).second ].intVector_;
    int* pIndices = (int*)( &( indices[ 0 ] ) );

    vec3* pNormals = 0;
    //vec3* pBiNormals = 0;
    //vec3* pTangents = 0;
    vec3* pTex0 = 0;

    bool bNeedNormals = false;
    bool bNeedTexCoords = false;
    bool bComputeTangentSpace = false;

    // see if texture coords are needed
    if ( outmap.find( "tex0" ) != outmap.end() )
    {
        bNeedTexCoords = true;
    }

    // see if tangent or binormal are needed
    if ( ( outmap.find( "binormal" ) != outmap.end() ) || 
         ( outmap.find( "tangent" ) != outmap.end() ) )
    {
        bComputeTangentSpace = true;
    }

    // see if normals are needed
    if ( outmap.find( "normal" ) != outmap.end() )
    {
        bNeedNormals = true;
    }

    // Compute normals.
    Mapping::iterator want = outmap.find( "normal" );
    bool have_normals = ( inmap.find( "normal" ) != inmap.end() ) ? true : false;

    if ( bNeedNormals || bComputeTangentSpace )
    {
        // see if normals are provided
        if ( !have_normals )
        {
            // create normals
            if ( want == outmap.end() )
			{
	            VertexAttribute norAtt;
	            norAtt.Name_ = "normal";
                output.push_back( norAtt );

                outmap[ "normal" ] = output.size() - 1;
                want = outmap.find( "normal" );
            }

			// just initialize array so it's the correct size
			output[ (*want).second ].floatVector_ = positions;

            //VertexAttribute::FloatVector& normals = output[ (*want).second ].floatVector_;

            // zero out normals
            for ( unsigned n = 0; n < positions.size(); ++n )
            {
                output[ (*want).second ].floatVector_[ n ] = nv_zero;
            }

            pNormals = (vec3*)( &( output[ (*want).second ].floatVector_[0] ) );

            // calculate face normals for each face
            //  & add its normal to vertex normal total
            for ( unsigned int t = 0; t < indices.size(); t += 3 )
            {
                vec3 edge0;
                vec3 edge1;

                edge0 = pPositions[ indices[ t + 1 ] ] - pPositions[ indices[ t + 0 ] ];
                edge1 = pPositions[ indices[ t + 2 ] ] - pPositions[ indices[ t + 0 ] ];

				exact_normalize(&edge0.x);
				exact_normalize(&edge1.x);
			
                vec3 faceNormal = edge0 ^ edge1;

                if ( _WeightNormalsByFaceSize == DontWeightNormalsByFaceSize )
                {
                    // Renormalize face normal, so it's not weighted by face size
					exact_normalize(&faceNormal.x);
                }
                else
                {
                    // Leave it as-is, to weight by face size naturally by the cross product result
                }

                pNormals[ indices[ t + 0 ] ] += faceNormal;
                pNormals[ indices[ t + 1 ] ] += faceNormal;
                pNormals[ indices[ t + 2 ] ] += faceNormal;
            }

            // Renormalize each vertex normal
            for ( unsigned int v = 0; v < output[ (*want).second ].floatVector_.size() / 3; ++v )
				pNormals[v].normalize();
        }
    }

    // Compute texture coordinates.
    if ( bNeedTexCoords || bComputeTangentSpace )
    {
        if ( outmap.find("tex0") == outmap.end() )
		{
	        VertexAttribute texCoordAtt;
	        texCoordAtt.Name_ = "tex0";
            output.push_back( texCoordAtt );
            outmap[ "tex0" ] = output.size() - 1;
        }
    	want = outmap.find("tex0");
		Mapping::iterator have = inmap.find( "tex0" );
        bool have_texcoords = (have != inmap.end());

        // see if texcoords are provided
        if ( have_texcoords )
			output[ (*want).second ].floatVector_ = input[ (*have).second ].floatVector_;
        else {
			// just initialize array so it's the correct size
			output[ (*want).second ].floatVector_ = positions;

            pTex0 = (vec3*)( &(output[ (*want).second ].floatVector_[ 0 ]) );

			// Generate cylindrical coordinates

			// Find min and max positions for object bounding box

			vec3 maxPosition( -flt_max,  -flt_max,   -flt_max  );
			vec3 minPosition(  flt_max,   flt_max,    flt_max );

			// there are 1/3 as many vectors as floats
			const unsigned int theCount = static_cast<unsigned int>(positions.size() / 3.0f);

			for ( unsigned int i = 0; i < theCount; ++i )
			{
				maxPosition.x = nv_max( maxPosition.x, pPositions[ i ].x );
				maxPosition.y = nv_max( maxPosition.y, pPositions[ i ].y );
				maxPosition.z = nv_max( maxPosition.z, pPositions[ i ].z );

				minPosition.x = nv_min( minPosition.x, pPositions[ i ].x );
				minPosition.y = nv_min( minPosition.y, pPositions[ i ].y );
				minPosition.z = nv_min( minPosition.z, pPositions[ i ].z );
			}

			// Find major, minor and other axis for cylindrical texgen

			vec3 delta = maxPosition - minPosition;

			delta.x = _abs( delta.x );
			delta.y = _abs( delta.y );
			delta.z = _abs( delta.z );

			bool maxx,maxy,maxz;
			maxx = maxy = maxz = false;
			bool minz,miny,minx;
			minx = miny = minz = false;

			nv_scalar deltaMajor;

			if ( ( delta.x >= delta.y ) && ( delta.x >= delta.z ) )
			{
				maxx = true;
				deltaMajor = delta.x;
				if ( delta.y > delta.z )
				{
					minz = true;
				}
				else
				{
					miny = true;
				}
			}
			else
			if ( ( delta.z >= delta.y ) && ( delta.z >= delta.x ) )
			{
				maxz = true;
				deltaMajor = delta.z;
				if ( delta.y > delta.x )
				{
					minx = true;
				}
				else
				{
					miny = true;
				}
			}
			else
			if ( ( delta.y >= delta.z ) && ( delta.y >= delta.x ) )
			{
				maxy = true;
				deltaMajor = delta.y;
				if ( delta.x > delta.z )
				{
					minz = true;
				}
				else
				{
					minx = true;
				}
			}

			for ( unsigned int p = 0; p < theCount; ++p )
			{
				// Find position relative to center of bounding box

				vec3 texCoords = ( ( maxPosition + minPosition ) / 2.0f ) - pPositions[ p ];
				
				nv_scalar Major, Minor, Other = nv_zero;

				if ( maxx )
				{
					Major = texCoords.x;
					if ( miny )
					{
						Minor = texCoords.y;
						Other = texCoords.z;
					} else {
						Minor = texCoords.z;
						Other = texCoords.y;
					}
				}
				else
				if ( maxy )
				{
					Major = texCoords.y;
					if ( minx )
					{
						Minor = texCoords.x;
						Other = texCoords.z;
					} else {
						Minor = texCoords.z;
						Other = texCoords.x;
					}
				}
				else
				if ( maxz )
				{
					Major = texCoords.z;
					if ( miny )
					{
						Minor = texCoords.y;
						Other = texCoords.x;
					} else {
						Minor = texCoords.x;
						Other = texCoords.y;
					}
				}

				nv_scalar longitude = nv_zero;

				// Prevent zero or near-zero from being passed into atan2
				if ( _abs( Other ) < 0.0001f )
				{
					if ( Other >= nv_zero )
					{
						Other = 0.0001f;
					} else {
						Other = -0.0001f;
					}
				}

				// perform cylindrical mapping onto object, and remap from -pi,pi to -1,1

				longitude = (( atan2( Minor, Other ) ) / nv_scalar(3.141592654));

				texCoords.x = 0.5f * longitude + 0.5f;
				texCoords.y = (Major/deltaMajor) + 0.5f;

				texCoords.x = nv_max( texCoords.x, nv_zero );
				texCoords.y = nv_max( texCoords.y, nv_zero );

				texCoords.x = nv_min( texCoords.x, 1.0f );
				texCoords.y = nv_min( texCoords.y, 1.0f );

				pTex0[ p ].x = texCoords.x-0.25f;
				if ( pTex0[ p ].x < nv_zero ) pTex0[ p ].x += 1.0;
				pTex0[ p ].y = 1.0f-texCoords.y;
				pTex0[ p ].z = 1.0f;
			}
		}

		if ( _FixCylindricalTexGen == FixCylindricalTexGen )
		{
    		Mapping::iterator texIter = outmap.find( "tex0" );

			VertexAttribute::FloatVector& texcoords = ( output[ (*texIter).second ].floatVector_ );

			const unsigned int theSize = indices.size();
			
			for ( unsigned int f = 0; f < theSize; f += 3 )
			{
				for ( int v = 0; v < 3; ++v )
				{
					int start = f + v;
					int end = start + 1;

					if ( v == 2 )
					{
						end = f;
					}

					nv_scalar dS = texcoords[ indices[ end ] * 3 + 0 ] - texcoords[ indices[ start ] * 3 + 0 ];

					nv_scalar newS = nv_zero;

					bool bDoS = false;

					unsigned int theOneToChange = start;

					if ( _abs( dS ) >= 0.5f )
					{
						bDoS = true;
						if ( texcoords[ indices[ start ] * 3 + 0 ] < texcoords[ indices[ end ] * 3 + 0 ] )
						{
							newS = texcoords[ indices[ start ]* 3 + 0 ] + 1.0f;
						}
						else
						{
							theOneToChange = end;
							newS = texcoords[ indices[ end ] * 3 + 0 ] + 1.0f;
						}
					}

					if ( bDoS == true )
					{
						unsigned int theNewIndex = texcoords.size() / 3;
						// Duplicate every part of the vertex
						for ( unsigned int att = 0; att < output.size(); ++att )
						{
							// No new indices are created, just vertex attributes
							if ( output[ att ].Name_ != "indices" )
							{
								if ( output[ att ].Name_ == "tex0" ) 
								{
									output[ att ].floatVector_.push_back( (float)newS ); // y
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 1 ] ); // x
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 2 ] ); // z
								}
								else
								{
									// *3 b/c we are looking up 3vectors in an array of floats
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 0 ] ); // x
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 1 ] ); // y
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 2 ] ); // z
								}
							}
						}

                        IdenticalVertices_.push_back( EmptySet );

                        IdenticalVertices_[ indices[ theOneToChange ] ].insert( theNewIndex );
                        IdenticalVertices_[ theNewIndex ].insert( indices[ theOneToChange ] );

						// point to where the new vertices will go
						indices[ theOneToChange ] = theNewIndex;
					}

				} // for v

				{

				for ( int v = 0; v < 3; ++v )
				{
					int start = f + v;
					int end = start + 1;

					if ( v == 2 )
					{
						end = f;
					}

					nv_scalar dT = texcoords[ indices[ end ] * 3 + 1 ] - texcoords[ indices[ start ] * 3 + 1 ];

					nv_scalar newT = nv_zero;

					bool bDoT = false;

					unsigned int theOneToChange = start;

					if ( _abs( dT ) >= 0.5f )
					{
						bDoT = true;
						if ( texcoords[ indices[ start ] * 3 + 1 ] < texcoords[ indices[ end ] * 3 + 1 ] )
						{
							newT = texcoords[ indices[ start ] * 3 + 1 ] + 1.0f;
						}
						else
						{
							theOneToChange = end;
							newT = texcoords[ indices[ end ] * 3 + 1 ] + 1.0f;
						}
					}

					if ( bDoT == true )
					{
						unsigned int theNewIndex = texcoords.size() / 3;
						// Duplicate every part of the vertex
						for ( unsigned int att = 0; att < output.size(); ++att )
						{
							// No new indices are created, just vertex attributes
							if ( output[ att ].Name_ != "indices" )
							{
								if ( output[ att ].Name_ == "tex0" ) 
								{
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 0 ] ); // x
									output[ att ].floatVector_.push_back( (float) newT ); // y
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 2 ] ); // z
								}
								else
								{
									// *3 b/c we are looking up 3vectors in an array of floats
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 0 ] ); // x
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 1 ] ); // y
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ theOneToChange ] * 3 + 2 ] ); // z
								}
							}
						}

                        IdenticalVertices_.push_back( EmptySet );

                        IdenticalVertices_[ theNewIndex ].insert( indices[ theOneToChange ] );
                        IdenticalVertices_[ indices[ theOneToChange ] ].insert( theNewIndex );

						// point to where the new vertices will go
						indices[ theOneToChange ] = theNewIndex;
					}
				}

				} // for v

			} // for f
		} // if fix texgen
		if (pTextureMatrix) {
			const mat4 M(	pTextureMatrix[0],	pTextureMatrix[1],	pTextureMatrix[2],	pTextureMatrix[3],
							pTextureMatrix[4],	pTextureMatrix[5],	pTextureMatrix[6],	pTextureMatrix[7],
							pTextureMatrix[8],	pTextureMatrix[9],	pTextureMatrix[10],	pTextureMatrix[11],
							pTextureMatrix[12],	pTextureMatrix[13],	pTextureMatrix[14],	pTextureMatrix[15]);
    		Mapping::iterator texIter = outmap.find("tex0");
			VertexAttribute::FloatVector& texcoords = output[(*texIter).second].floatVector_;

			// now apply matrix
			for (unsigned int v = 0; v < texcoords.size(); v += 3) {
				vec3& V = *reinterpret_cast<vec3*>(&texcoords[v]);
				V = V * M;
			}
		}

    }

	if ( bComputeTangentSpace )
	{
        EdgeSet Edges;

		Mapping::iterator texIter = outmap.find( "tex0" );

        vec3* tex = (vec3*)&( output[ (*texIter).second ].floatVector_[ 0 ] );

        typedef xr_vector< vec3 > VecVector;

        // create tangents
        want = outmap.find( "tangent" );
        if ( want == outmap.end() )
		{
	        VertexAttribute tanAtt;
	        tanAtt.Name_ = "tangent";
            output.push_back( tanAtt );
            outmap[ "tangent" ] = output.size() - 1;
            want = outmap.find( "tangent" );
        }
		// just initialize array so it's the correct size
		output[ (*want).second ].floatVector_ = positions;

        // create binormals
        want = outmap.find( "binormal" );
        if ( want == outmap.end() )
		{
	        VertexAttribute binAtt;
	        binAtt.Name_ = "binormal";
            output.push_back( binAtt );
            outmap[ "binormal" ] = output.size() - 1;
            want = outmap.find( "binormal" );
        }
		// just initialize array so it's the correct size
	    output[ (*want).second ].floatVector_ = positions;

        // Create a vector of s,t and sxt for each face of the model
        VecVector sVector;
        VecVector tVector;
        VecVector sxtVector;

        const unsigned int theSize = indices.size();
		// for each face, calculate its S,T & SxT vector, & store its edges
		for ( unsigned int f = 0; f < theSize; f += 3 )
		{
			vec3d edge0;
			vec3d edge1;

			vec3d s;
			vec3d t;

            // grap position & tex coords again in case they were reallocated
            pPositions	= (vec3*)	( &( positions[ 0 ] ) );
            tex			= (vec3*)	( &( output[ (*texIter).second ].floatVector_[ 0 ] ) );

			double		_eps	= type_epsilon	(double)*10;
			double		a,b,c;
			vec3d		sxt;

			// create an edge(s) out of s and t
			edge0.y		= tex[ indices[ f + 1 ] ].x - tex[ indices[ f ] ].x;
			edge0.z		= tex[ indices[ f + 1 ] ].y - tex[ indices[ f ] ].y;
			edge1.y		= tex[ indices[ f + 2 ] ].x - tex[ indices[ f ] ].x;
			edge1.z		= tex[ indices[ f + 2 ] ].y - tex[ indices[ f ] ].y;

			// create an edge out of x, s and t
			edge0.x		= pPositions[ indices[ f + 1 ] ].x - pPositions[ indices[ f ] ].x;
			edge1.x		= pPositions[ indices[ f + 2 ] ].x - pPositions[ indices[ f ] ].x;
			sxt			= edge0 ^ edge1;

            a=sxt.x;b=sxt.y;c= sxt.z;
            double ds_dx = nv_zero;		if ( _abs( a ) > _eps )		ds_dx = - b / a;
            double dt_dx = nv_zero;		if ( _abs( a ) > _eps )		dt_dx = - c / a;

			// create an edge out of y, s and t
			edge0.x		= pPositions[ indices[ f + 1 ] ].y - pPositions[ indices[ f ] ].y;
			edge1.x		= pPositions[ indices[ f + 2 ] ].y - pPositions[ indices[ f ] ].y;
			sxt			= edge0 ^ edge1;

            a=sxt.x;b=sxt.y;c= sxt.z;
			double ds_dy = nv_zero;		if ( _abs( a ) > _eps )		ds_dy = -b / a;
			double dt_dy = nv_zero;		if ( _abs( a ) > _eps )		dt_dy = -c / a;

			// create an edge out of z, s and t
			edge0.x		= pPositions[ indices[ f + 1 ] ].z - pPositions[ indices[ f ] ].z;
			edge1.x		= pPositions[ indices[ f + 2 ] ].z - pPositions[ indices[ f ] ].z;
			sxt			= edge0 ^ edge1;

            a=sxt.x;b=sxt.y;c= sxt.z;
			double ds_dz = nv_zero;		if ( _abs( a ) > _eps )		ds_dz = -b / a;
            double dt_dz = nv_zero;		if ( _abs( a ) > _eps )		dt_dz = -c / a;

            // generate coordinate frame from the gradients
            s				= vec3d( ds_dx, ds_dy, ds_dz );
            t				= vec3d( dt_dx, dt_dy, dt_dz );

			s.normalize		();
			t.normalize		();
			sxt				= s ^ t;
			sxt.normalize	();

            // save vectors for this face
            sVector.push_back	( vec3(s)	);
            tVector.push_back	( vec3(t)	);
            sxtVector.push_back	( vec3(sxt) );

			if ( _FixTangents == FixTangents )
			{
				// Look for each edge of the triangle in the edge map, in order to find 
				//  a neighboring face along the edge

				for ( int e = 0; e < 3; ++e )
				{
					Edge edge;

					int start = f + e;
					int end = start + 1;

					if ( e == 2 )
					{
						end = f;
					}
					// order vertex indices ( low, high )
					edge.v0 = (unsigned int)nv_min( (nv_scalar)indices[ start ], (nv_scalar)indices[ end ] );
					edge.v1 = (unsigned int)nv_max( (nv_scalar)indices[ start ], (nv_scalar)indices[ end ] );

					EdgeSet::iterator iter = Edges.find( edge );

					// if we are the only triangle with this edge...
					if ( iter == Edges.end() )
					{
						// ...then add us to the set of edges
						edge.face = f / 3;
						Edges.insert( edge );
					}
					else
					{
						// otherwise, check our neighbor's s,t & sxt vectors vs our own
						const nv_scalar sAgreement		= dot(s, sVector[(*iter).face]);
						const nv_scalar tAgreement		= dot(t, tVector[(*iter).face]);
						const nv_scalar sxtAgreement	= dot(sxt, sxtVector[(*iter).face]);

						// Check Radian angle split limit
						const nv_scalar epsilon			= _cos( bSmoothCreaseAngleRadians );

						//  if the discontinuity in s, t, or sxt is greater than some epsilon,
						//   duplicate the vertex so it won't smooth with its neighbor anymore

						if ( ( _abs(   sAgreement ) < epsilon ) ||
							 ( _abs(   tAgreement ) < epsilon ) ||
							 ( _abs( sxtAgreement ) < epsilon ) )
						{
							// Duplicate two vertices of this edge for this triangle only.
							//  This way the faces won't smooth with each other, thus
							//  preventing the tangent basis from becoming degenerate

							//  divide by 3 b/c vector is of floats and not vectors
							const unsigned int theNewIndex = positions.size() / 3;

							// Duplicate every part of the vertex
							for ( unsigned int att = 0; att < output.size(); ++att )
							{
								// No new indices are created, just vertex attributes
								if ( output[ att ].Name_ != "indices" )
								{
									// *3 b/c we are looking up 3vectors in an array of floats
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ start ] * 3 + 0 ] ); // x
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ start ] * 3 + 1 ] ); // y
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ start ] * 3 + 2 ] ); // z

									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ end ] * 3 + 0 ] ); // x
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ end ] * 3 + 1 ] ); // y
									output[ att ].floatVector_.push_back( output[ att ].floatVector_[ indices[ end ] * 3 + 2 ] ); // z
								}
							}

                            IdenticalVertices_.push_back( EmptySet );
                            IdenticalVertices_.push_back( EmptySet );

							// point to where the new vertices will go
							indices[ start ] = theNewIndex;
							indices[ end ] =   theNewIndex + 1;

						}

						// Now that the vertices are duplicated, smoothing won't occur over this edge,
						//  because the two faces will sum their tangent basis vectors into separate indices 
					}
				}
			} // if fixtangents
		}

        // Allocate std::vector & Zero out average basis for tangent space smoothing
        VecVector avgS;
        VecVector avgT;

        for ( unsigned int p = 0; p < positions.size(); p += 3 )
        {
            avgS.push_back( vec3_null ); // do S
            avgT.push_back( vec3_null ); // now t
        }

        //  go through faces and add up the bases for each vertex 
        const int theFaceCount = indices.size() / 3;

        for ( unsigned int face = 0; face < (unsigned int)theFaceCount; ++face )
        {
            // sum bases, so we smooth the tangent space across edges
            avgS[ pIndices[ face * 3 ] ]	 +=   sVector[ face ];
            avgT[ pIndices[ face * 3 ] ]	 +=   tVector[ face ];

            avgS[ pIndices[ face * 3 + 1 ] ] +=   sVector[ face ];
            avgT[ pIndices[ face * 3 + 1 ] ] +=   tVector[ face ];

            avgS[ pIndices[ face * 3 + 2 ] ] +=   sVector[ face ];
            avgT[ pIndices[ face * 3 + 2 ] ] +=   tVector[ face ];
        }

        if ( _FixCylindricalTexGen == FixCylindricalTexGen )
        {
            for ( unsigned int v = 0; v < IdenticalVertices_.size(); ++v )
            {
                // go through each vertex & sum up it's true neighbors
                for ( xr_set< unsigned int >::iterator iter = IdenticalVertices_[ v ].begin();
                      iter != IdenticalVertices_[ v ].end();
                      ++iter )
                {
                    avgS[ v ] += avgS[ *iter ];
                    avgT[ v ] += avgT[ *iter ];
                }
            }
        }

        Mapping::iterator tangent	= outmap.find( "tangent" );
        Mapping::iterator binormal	= outmap.find( "binormal" );

        // now renormalize
        for ( unsigned int b = 0; b < positions.size(); b += 3 ) 
        {
            *reinterpret_cast<vec3*>(&output[(*tangent).second].floatVector_[b])	= normalize(avgS[b / 3]);  // s
            *reinterpret_cast<vec3*>(&output[(*binormal).second].floatVector_[b])	= normalize(avgT[b / 3]);  // T
        }
	}

	// At this point, tex coords, normals, binormals and tangents should be generated if necessary,
	//  and other attributes are simply copied as available

    return true;
}

