//======================================================================
//interp.c
//
//Interpolation (and extrapolation) of LightWave envelopes.
//
//Ernie Wright  16 Nov 00
//
//The LightWave plug-in SDK provides functions for evaluating envelopes
//and channels at arbitrary times, which is what plug-ins should use.
//This code shows how to evaluate envelopes in standalone programs.
//======================================================================

#include "stdafx.h"
#pragma hdrstop

#include "envelope.h"

//======================================================================
//range()
//
//Given the value v of a periodic function, returns the equivalent value
//v2 in the principal interval [lo, hi].  If i isn't NULL, it receives
//the number of wavelengths between v and v2.
//
//   v2 = v - i * (hi - lo)
//
//For example, range( 3 pi, 0, 2 pi, i ) returns pi, with i = 1.
//======================================================================
static float range( float v, float lo, float hi, int *i ){
   float v2, r = hi - lo;
   if ( r == 0.0 ){
      if ( i ) *i = 0;
      return lo;
   }
   v2 = lo + v - r * ( float ) floor(( float ) v / r );
   if ( i ) *i = -( int )(( v2 - v ) / r + ( v2 > v ? 0.5 : -0.5 ));
   return v2;
}


//======================================================================
//hermite()
//
//Calculate the Hermite coefficients.
//======================================================================
static void hermite( float t, float *h1, float *h2, float *h3, float *h4 ){
   float t2, t3;

   t2 = t * t;
   t3 = t * t2;

   *h2 = 3.0f * t2 - t3 - t3;
   *h1 = 1.0f - *h2;
   *h4 = t3 - t2;
   *h3 = *h4 - t2 + t;
}


//======================================================================
//bezier()
//
//Interpolate the value of a 1D Bezier curve.
//======================================================================
static float bezier( float x0, float x1, float x2, float x3, float t ){
   float a, b, c, t2, t3;

   t2 = t * t;
   t3 = t2 * t;

   c = 3.0f * ( x1 - x0 );
   b = 3.0f * ( x2 - x1 ) - c;
   a = x3 - x0 - c - b;

   return a * t3 + b * t2 + c * t + x0;
}


//======================================================================
//bez2_time()
//
//Find the t for which bezier() returns the input time.  The handle
//endpoints of a BEZ2 curve represent the control points, and these have
//(time, value) coordinates, so time is used as both a coordinate and a
//parameter for this curve type.
//======================================================================
static float bez2_time( float x0, float x1, float x2, float x3, float time, float *t0, float *t1 ){
   float v, t;

   t = *t0 + ( *t1 - *t0 ) * 0.5f;
   v = bezier( x0, x1, x2, x3, t );
   if ( _abs( time - v ) > .0001f ) {
      if ( v > time )
         *t1 = t;
      else
         *t0 = t;
      return bez2_time( x0, x1, x2, x3, time, t0, t1 );
   }
   else
      return t;
}


//======================================================================
//bez2()
//
//Interpolate the value of a BEZ2 curve.
//======================================================================
static float bez2( st_Key *key0, st_Key *key1, float time ){
   float x, y, t, t0 = 0.0f, t1 = 1.0f;

   if ( key0->shape == SHAPE_BEZ2 )
      x = key0->time + key0->param[ 2 ];
   else
      x = key0->time + ( key1->time - key0->time ) / 3.0f;

   t = bez2_time( key0->time, x, key1->time + key1->param[ 0 ], key1->time,
      time, &t0, &t1 );

   if ( key0->shape == SHAPE_BEZ2 )
      y = key0->value + key0->param[ 3 ];
   else
      y = key0->value + key0->param[ 1 ] / 3.0f;

   return bezier( key0->value, y, key1->param[ 1 ] + key1->value, key1->value, t );
}


//======================================================================
//outgoing()
//
//Return the outgoing tangent to the curve at key0.  The value returned
//for the BEZ2 case is used when extrapolating a linear pre behavior and
//when interpolating a non-BEZ2 span.
//======================================================================
static float outgoing( st_Key *key0p, st_Key *key0, st_Key *key1 ){
   float a, b, d, t, out;

   switch ( key0->shape ){
      case SHAPE_TCB:
         a = ( 1.0f - key0->tension ) * ( 1.0f + key0->continuity ) * ( 1.0f + key0->bias );
         b = ( 1.0f - key0->tension ) * ( 1.0f - key0->continuity ) * ( 1.0f - key0->bias );
         d = key1->value - key0->value;

         if ( key0p ) {
            t = ( key1->time - key0->time ) / ( key1->time - key0p->time );
            out = t * ( a * ( key0->value - key0p->value ) + b * d );
         }else
            out = b * d;
         break;

      case SHAPE_LINE:
         d = key1->value - key0->value;
         if ( key0p ) {
            t = ( key1->time - key0->time ) / ( key1->time - key0p->time );
            out = t * ( key0->value - key0p->value + d );
         }
         else
            out = d;
         break;

      case SHAPE_BEZI:
      case SHAPE_HERM:
         out = key0->param[ 1 ];
         if ( key0p )
            out *= ( key1->time - key0->time ) / ( key1->time - key0p->time );
         break;

      case SHAPE_BEZ2:
         out = key0->param[ 3 ] * ( key1->time - key0->time );
         if ( _abs( key0->param[ 2 ] ) > 1e-5f )
            out /= key0->param[ 2 ];
         else
            out *= 1e5f;
         break;

      case SHAPE_STEP:
      default:
         out = 0.0f;
         break;
   }

   return out;
}

//======================================================================
//incoming()
//
//Return the incoming tangent to the curve at key1.  The value returned
//for the BEZ2 case is used when extrapolating a linear post behavior.
//======================================================================
static float incoming( st_Key *key0, st_Key *key1, st_Key *key1n ){
   float a, b, d, t, in;

   switch ( key1->shape ){
      case SHAPE_LINE:
         d = key1->value - key0->value;
         if ( key1n ) {
            t = ( key1->time - key0->time ) / ( key1n->time - key0->time );
            in = t * ( key1n->value - key1->value + d );
         }
         else
            in = d;
         break;

      case SHAPE_TCB:
         a = ( 1.0f - key1->tension ) * ( 1.0f - key1->continuity ) * ( 1.0f + key1->bias );
         b = ( 1.0f - key1->tension ) * ( 1.0f + key1->continuity ) * ( 1.0f - key1->bias );
         d = key1->value - key0->value;

         if ( key1n ) {
            t = ( key1->time - key0->time ) / ( key1n->time - key0->time );
            in = t * ( b * ( key1n->value - key1->value ) + a * d );
         }
         else
            in = a * d;
         break;

      case SHAPE_BEZI:
      case SHAPE_HERM:
         in = key1->param[ 0 ];
         if ( key1n )
            in *= ( key1->time - key0->time ) / ( key1n->time - key0->time );
         break;

      case SHAPE_BEZ2:
         in = key1->param[ 1 ] * ( key1->time - key0->time );
         if ( _abs( key1->param[ 0 ] ) > 1e-5f )
            in /= key1->param[ 0 ];
         else
            in *= 1e5f;
         break;

      case SHAPE_STEP:
      default:
         in = 0.0f;
         break;
   }

   return in;
}



//======================================================================
//evalEnvelope()
//
//Given a _list_ of keys and a time, returns the interpolated value of the
//envelope at that time.
//======================================================================
float evalEnvelope( CEnvelope *env, float time ){
   	st_Key *key0, *key1, *skey, *ekey, *skey_n, *ekey_p, *key0_p=0, *key1_n=0;
   	float t, h1, h2, h3, h4, in, out, offset = 0.0f;
   	int noff;


   	// if there's no key, the value is 0
   	if ( env->keys.empty() ) return 0.0f;

   	// if there's only one key, the value is constant
   	if ( env->keys.size() == 1 )
      	return env->keys[0]->value;

   // find the first and last keys
	int sz = env->keys.size();
   	skey = env->keys[0];
   	ekey = env->keys[sz-1];
   	skey_n=env->keys[1];
   	ekey_p=env->keys[sz-2];

   	// use pre-behavior if time is before first key time
   	if ( time < skey->time ){
      	switch ( env->behavior[ 0 ] ){
         	case BEH_RESET:            return 0.0f;
         	case BEH_CONSTANT:			return skey->value;
         	case BEH_REPEAT:
            	time = range( time, skey->time, ekey->time, NULL );
            break;
         	case BEH_OSCILLATE:
            	time = range( time, skey->time, ekey->time, &noff );
            	if ( noff % 2 )
               		time = ekey->time - skey->time - time;
            break;
         	case BEH_OFFSET:
            	time = range( time, skey->time, ekey->time, &noff );
            	offset = noff * ( ekey->value - skey->value );
            break;
         	case BEH_LINEAR:
            	out = outgoing( 0, skey, skey_n ) / ( skey_n->time - skey->time );
            	return out * ( time - skey->time ) + skey->value;
      	}
   	}
   	// use post-behavior if time is after last key time
   	else if ( time > ekey->time ) {
      	switch ( env->behavior[ 1 ] ){
         	case BEH_RESET:            return 0.0f;
         	case BEH_CONSTANT:			return ekey->value;
         	case BEH_REPEAT:
            	time = range( time, skey->time, ekey->time, NULL );
            break;
         	case BEH_OSCILLATE:
            	time = range( time, skey->time, ekey->time, &noff );
            	if ( noff % 2 )
               		time = ekey->time - skey->time - time;
            break;
         	case BEH_OFFSET:
            	time = range( time, skey->time, ekey->time, &noff );
            	offset = noff * ( ekey->value - skey->value );
            break;
         	case BEH_LINEAR:
            	in = incoming( ekey_p, ekey, 0 ) / ( ekey->time - ekey_p->time );
            	return in * ( time - ekey->time ) + ekey->value;
      	}
   	}
   	// get the endpoints of the interval being evaluated
    int k=0;
    while (time>env->keys[k+1]->time) k++;
    VERIFY((k+1)<sz);

    key1 = env->keys[k+1];
	key0 = env->keys[k];
    if (k>0)  		key0_p = env->keys[k-1];
    if ((k+2)<sz) 	key1_n = env->keys[k+2];

   	// check for singularities first
   	if ( time == key0->time )		return key0->value + offset;
   	else if ( time == key1->time )	return key1->value + offset;

   	// get interval length, time in [0, 1]
   	t = ( time - key0->time ) / ( key1->time - key0->time );

   	// interpolate
   	switch ( key1->shape ){
      	case SHAPE_TCB:
      	case SHAPE_BEZI:
      	case SHAPE_HERM:
         	out = outgoing( key0_p, key0, key1 );
         	in = incoming( key0, key1, key1_n );
         	hermite( t, &h1, &h2, &h3, &h4 );
         	return h1 * key0->value + h2 * key1->value + h3 * out + h4 * in + offset;
      	case SHAPE_BEZ2:         return bez2( key0, key1, time ) + offset;
      	case SHAPE_LINE:         return key0->value + t * ( key1->value - key0->value ) + offset;
      	case SHAPE_STEP:         return key0->value + offset;
      	default:
         	return offset;
   	}
}
