//#include "stdafx.h"
#ifndef __DXTRILIST_INCLUDED__

#define __DXTRILIST_INCLUDED__


#include "../ode_include.h"




struct dcVector3{

	float x, y, z;



	dcVector3(){}

	dcVector3(dReal x, dReal y, dReal z){

		this->x = (float)x;

		this->y = (float)y;

		this->z = (float)z;

	}



	dcVector3(const dReal* v){

		x = (float)v[0];

		y = (float)v[1];

		z = (float)v[2];

	}



	~dcVector3(){}




	operator float* (){  //&slipch

	return reinterpret_cast<float*>(this);
	}
	/* Add */

	dcVector3 operator+(const dcVector3& v) const{

		dcVector3 Out;

		Out.x = x + v.x;

		Out.y = y + v.y;

		Out.z = z + v.z;

		return Out;

	}



	dcVector3& operator+=(const dcVector3& v){

		x += v.x;

		y += v.y;

		z += v.z;

		return *this;

	}



	/* Sub */

	dcVector3 operator-(const dcVector3& v) const{

		dcVector3 Out;

		Out.x = x - v.x;

		Out.y = y - v.y;

		Out.z = z - v.z;

		return Out;

	}



	dcVector3& operator-=(const dcVector3& v){

		x -= v.x;

		y -= v.y;

		z -= v.z;

		return *this;

	}



	/* Mul */

	dcVector3 operator*(const dcVector3& v) const{

		dcVector3 Out;

		Out.x = x * v.x;

		Out.y = y * v.y;

		Out.z = z * v.z;

		return Out;

	}



	dcVector3 operator*(float Scalar) const{

		dcVector3 Out;

		Out.x = x * Scalar;

		Out.y = y * Scalar;

		Out.z = z * Scalar;

		return Out;

	}



	dcVector3& operator*=(const dcVector3& v){

		x *= v.x;

		y *= v.y;

		z *= v.z;

		return *this;

	}



	dcVector3& operator*=(float Scalar){

		x *= Scalar;

		y *= Scalar;

		z *= Scalar;

		return *this;

	}



	/* Div */

	dcVector3 operator/(const dcVector3& v) const{

		dcVector3 Out;

		Out.x = x / v.x;

		Out.y = y / v.y;

		Out.z = z / v.z;

		return Out;

	}



	dcVector3 operator/(float Scalar) const{

		dcVector3 Out;

		Out.x = x / Scalar;

		Out.y = y / Scalar;

		Out.z = z / Scalar;

		return Out;

	}



	dcVector3& operator/=(const dcVector3& v){

		x /= v.x;

		y /= v.y;

		z /= v.z;

		return *this;

	}



	dcVector3& operator/=(float Scalar){

		x /= Scalar;

		y /= Scalar;

		z /= Scalar;

		return *this;

	}



	/* Negative */

	dcVector3& operator-(){

		x = -x;

		y = -y;

		z = -z;

		return *this;

	}



	/* Comparison */

	bool operator==(const dcVector3& v) const{

		return x == v.x && y == v.y && z == v.z;

	}



	bool operator!=(const dcVector3& v) const{

		return v.x != x || v.y != y || v.z != z;

	}



	float DotProduct(const dcVector3& v) const{

		return x * v.x + y * v.y + z * v.z;

	}



	dcVector3 CrossProduct(const dcVector3& v) const{

		dcVector3 Out;

		Out.x = y * v.z - z * v.y;

		Out.y = z * v.x - x * v.z;

		Out.z = x * v.y - y * v.x;

		return Out;

	}



	float MagnitudeSq() const{

		return DotProduct(*this);

	}



	float Magnitude() const{

		return _sqrt(MagnitudeSq());

	}



	void Normalize(){

		operator/=(Magnitude());

	}



	/* Member access */

	float& operator[](int Index){

		return *(&x + Index);

	}



	float operator[](int Index) const{

		return *(&x + Index);

	}

};





/* Class ID */

extern int dTriListClass;



/* Per triangle callback */

typedef int dTriCallback(dGeomID TriList, dGeomID RefObject, int TriangleIndex);

void dGeomTriListSetCallback(dGeomID g, dTriCallback* Callback);

dTriCallback* dGeomTriListGetCallback(dGeomID g);



/* Per object callback */

typedef void dTriArrayCallback(dGeomID TriList, dGeomID RefObject, const int* TriIndices, int TriCount);

void dGeomTriListSetArrayCallback(dGeomID g, dTriArrayCallback* ArrayCallback);

dTriArrayCallback* dGeomTriListGetArrayCallback(dGeomID g);



/* Construction */

dxGeom* dCreateTriList(dSpaceID space, dTriCallback* Callback, dTriArrayCallback* ArrayCallback);



/* Setting data */

void dGeomTriListBuild(dGeomID g, const dcVector3* Vertices, int VertexCount, const int* Indices, int IndexCount);



/* Getting data */

void dGeomTriListGetTriangle(dGeomID g, int Index, dVector3* v0, dVector3* v1, dVector3* v2);



/* Internal types */

class dcTriListCollider;



struct dxTriList{

	dReal p[4];						// dxPlane

	dTriCallback* Callback;

	dTriArrayCallback* ArrayCallback;

	dcTriListCollider* Collider;

};



struct dcPlane{

	dcVector3 Normal;

	float Distance;



	dcPlane(){}

	dcPlane(const dcVector3& v0, const dcVector3& v1, const dcVector3& v2){

		dcVector3 u = v1 - v0;

		dcVector3 v = v2 - v0;



		Normal = u.CrossProduct(v);

		Distance = v0.DotProduct(Normal);

		Normalize();

	}



	void Normalize(){

		float Factor = 1.0f / Normal.Magnitude();

		Normal *= Factor;

		Distance *= Factor;

	}



	bool Contains(const dcVector3& RefObject, float Epsilon = 0.0f) const{

		return Normal.DotProduct(RefObject) - Distance >= - Epsilon; //@slipch ">=" instead ">"

	}

};



template<class T> const T& dcMAX(const T& x, const T& y){

	return x > y ? x : y;

}



template<class T> const T& dcMIN(const T& x, const T& y){

	return x < y ? x : y;

}



#endif	//__DXTRILIST_INCLUDED__