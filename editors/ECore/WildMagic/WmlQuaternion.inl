// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion ()
{
    // the object is uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (Real fW, Real fX, Real fY, Real fZ)
{
    m_afTuple[0] = fW;
    m_afTuple[1] = fX;
    m_afTuple[2] = fY;
    m_afTuple[3] = fZ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (const Quaternion& rkQ)
{
    memcpy(m_afTuple,rkQ.m_afTuple,4*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (const Matrix3<Real>& rkRot)
{
    FromRotationMatrix(rkRot);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (const Vector3<Real>& rkAxis, Real fAngle)
{
    FromAxisAngle(rkAxis,fAngle);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (const Vector3<Real> akRotColumn[3])
{
    FromRotationMatrix(akRotColumn);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::operator const Real* () const
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::operator Real* ()
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <class Real>
Real Quaternion<Real>::operator[] (int i) const
{
    assert( 0 <= i && i < 4 );
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Quaternion<Real>::operator[] (int i)
{
    assert( 0 <= i && i < 4 );
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <class Real>
Real Quaternion<Real>::W () const
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Quaternion<Real>::W ()
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
Real Quaternion<Real>::X () const
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Quaternion<Real>::X ()
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Real Quaternion<Real>::Y () const
{
    return m_afTuple[2];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Quaternion<Real>::Y ()
{
    return m_afTuple[2];
}
//----------------------------------------------------------------------------
template <class Real>
Real Quaternion<Real>::Z () const
{
    return m_afTuple[3];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Quaternion<Real>::Z ()
{
    return m_afTuple[3];
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::operator= (const Quaternion& rkQ)
{
    memcpy(m_afTuple,rkQ.m_afTuple,4*sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator== (const Quaternion& rkQ) const
{
    for (int i = 0; i < 4; i++)
    {
        if ( m_afTuple[i] != rkQ.m_afTuple[i] )
            return false;
    }
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator!= (const Quaternion& rkQ) const
{
    return !operator==(rkQ);
}
//----------------------------------------------------------------------------
template <class Real>
int Quaternion<Real>::CompareArrays (const Quaternion& rkQ) const
{
    return memcmp(m_afTuple,rkQ.m_afTuple,4*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator< (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) < 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator<= (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) <= 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator> (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) > 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator>= (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) >= 0;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::operator+ (const Quaternion& rkQ) const
{
    Quaternion<Real> kSum;
    for (int i = 0; i < 4; i++)
        kSum.m_afTuple[i] = m_afTuple[i] + rkQ.m_afTuple[i];
    return kSum;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::operator- (const Quaternion& rkQ) const
{
    Quaternion<Real> kDiff;
    for (int i = 0; i < 4; i++)
        kDiff.m_afTuple[i] = m_afTuple[i] - rkQ.m_afTuple[i];
    return kDiff;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::operator* (const Quaternion& rkQ) const
{
    // NOTE:  Multiplication is not generally commutative, so in most
    // cases p*q != q*p.

    Quaternion kProd;

    kProd.m_afTuple[0] =
        m_afTuple[0]*rkQ.m_afTuple[0] -
        m_afTuple[1]*rkQ.m_afTuple[1] -
        m_afTuple[2]*rkQ.m_afTuple[2] -
        m_afTuple[3]*rkQ.m_afTuple[3];

    kProd.m_afTuple[1] =
        m_afTuple[0]*rkQ.m_afTuple[1] +
        m_afTuple[1]*rkQ.m_afTuple[0] +
        m_afTuple[2]*rkQ.m_afTuple[3] -
        m_afTuple[3]*rkQ.m_afTuple[2];

    kProd.m_afTuple[2] =
        m_afTuple[0]*rkQ.m_afTuple[2] +
        m_afTuple[2]*rkQ.m_afTuple[0] +
        m_afTuple[3]*rkQ.m_afTuple[1] -
        m_afTuple[1]*rkQ.m_afTuple[3];

    kProd.m_afTuple[3] =
        m_afTuple[0]*rkQ.m_afTuple[3] +
        m_afTuple[3]*rkQ.m_afTuple[0] +
        m_afTuple[1]*rkQ.m_afTuple[2] -
        m_afTuple[2]*rkQ.m_afTuple[1];

    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::operator* (Real fScalar) const
{
    Quaternion<Real> kProd;
    for (int i = 0; i < 4; i++)
        kProd.m_afTuple[i] = fScalar*m_afTuple[i];
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::operator/ (Real fScalar) const
{
    Quaternion<Real> kQuot;
    int i;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < 4; i++)
            kQuot.m_afTuple[i] = fInvScalar*m_afTuple[i];
    }
    else
    {
        for (i = 0; i < 4; i++)
            kQuot.m_afTuple[i] = Math<Real>::MAX_REAL;
    }

    return kQuot;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::operator- () const
{
    Quaternion<Real> kNeg;
    for (int i = 0; i < 4; i++)
        kNeg.m_afTuple[i] = -m_afTuple[i];
    return kNeg;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> operator* (Real fScalar, const Quaternion<Real>& rkQ)
{
    Quaternion<Real> kProd;
    for (int i = 0; i < 4; i++)
        kProd[i] = fScalar*rkQ[i];
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::operator+= (const Quaternion& rkQ)
{
    for (int i = 0; i < 4; i++)
        m_afTuple[i] += rkQ.m_afTuple[i];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::operator-= (const Quaternion& rkQ)
{
    for (int i = 0; i < 4; i++)
        m_afTuple[i] -= rkQ.m_afTuple[i];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::operator*= (Real fScalar)
{
    for (int i = 0; i < 4; i++)
        m_afTuple[i] *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::operator/= (Real fScalar)
{
    int i;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < 4; i++)
            m_afTuple[i] *= fInvScalar;
    }
    else
    {
        for (i = 0; i < 4; i++)
            m_afTuple[i] = Math<Real>::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::FromRotationMatrix (const Matrix3<Real>& rkRot)
{
    // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
    // article "Quaternion Calculus and Fast Animation".

    Real fTrace = rkRot(0,0) + rkRot(1,1) + rkRot(2,2);
    Real fRoot;

    if ( fTrace > (Real)0.0 )
    {
        // |w| > 1/2, may as well choose w > 1/2
        fRoot = Math<Real>::Sqrt(fTrace + (Real)1.0);  // 2w
        m_afTuple[0] = ((Real)0.5)*fRoot;
        fRoot = ((Real)0.5)/fRoot;  // 1/(4w)
        m_afTuple[1] = (rkRot(2,1)-rkRot(1,2))*fRoot;
        m_afTuple[2] = (rkRot(0,2)-rkRot(2,0))*fRoot;
        m_afTuple[3] = (rkRot(1,0)-rkRot(0,1))*fRoot;
    }
    else
    {
        // |w| <= 1/2
        int i = 0;
        if ( rkRot(1,1) > rkRot(0,0) )
            i = 1;
        if ( rkRot(2,2) > rkRot(i,i) )
            i = 2;
        int j = ms_iNext[i];
        int k = ms_iNext[j];

        fRoot = Math<Real>::Sqrt(rkRot(i,i)-rkRot(j,j)-rkRot(k,k)+(Real)1.0);
        Real* apfQuat[3] = { &m_afTuple[1], &m_afTuple[2], &m_afTuple[3] };
        *apfQuat[i] = ((Real)0.5)*fRoot;
        fRoot = ((Real)0.5)/fRoot;
        m_afTuple[0] = (rkRot(k,j)-rkRot(j,k))*fRoot;
        *apfQuat[j] = (rkRot(j,i)+rkRot(i,j))*fRoot;
        *apfQuat[k] = (rkRot(k,i)+rkRot(i,k))*fRoot;
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::ToRotationMatrix (Matrix3<Real>& rkRot) const
{
    Real fTx  = ((Real)2.0)*m_afTuple[1];
    Real fTy  = ((Real)2.0)*m_afTuple[2];
    Real fTz  = ((Real)2.0)*m_afTuple[3];
    Real fTwx = fTx*m_afTuple[0];
    Real fTwy = fTy*m_afTuple[0];
    Real fTwz = fTz*m_afTuple[0];
    Real fTxx = fTx*m_afTuple[1];
    Real fTxy = fTy*m_afTuple[1];
    Real fTxz = fTz*m_afTuple[1];
    Real fTyy = fTy*m_afTuple[2];
    Real fTyz = fTz*m_afTuple[2];
    Real fTzz = fTz*m_afTuple[3];

    rkRot(0,0) = ((Real)1.0)-(fTyy+fTzz);
    rkRot(0,1) = fTxy-fTwz;
    rkRot(0,2) = fTxz+fTwy;
    rkRot(1,0) = fTxy+fTwz;
    rkRot(1,1) = ((Real)1.0)-(fTxx+fTzz);
    rkRot(1,2) = fTyz-fTwx;
    rkRot(2,0) = fTxz-fTwy;
    rkRot(2,1) = fTyz+fTwx;
    rkRot(2,2) = ((Real)1.0)-(fTxx+fTyy);
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::FromRotationMatrix (const Vector3<Real> akRotColumn[3])
{
    Matrix3<Real> kRot;
    for (int iCol = 0; iCol < 3; iCol++)
    {
        kRot(0,iCol) = akRotColumn[iCol][0];
        kRot(1,iCol) = akRotColumn[iCol][1];
        kRot(2,iCol) = akRotColumn[iCol][2];
    }
    FromRotationMatrix(kRot);
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::ToRotationMatrix (Vector3<Real> akRotColumn[3]) const
{
    Matrix3<Real> kRot;
    ToRotationMatrix(kRot);
    for (int iCol = 0; iCol < 3; iCol++)
    {
        akRotColumn[iCol][0] = kRot(0,iCol);
        akRotColumn[iCol][1] = kRot(1,iCol);
        akRotColumn[iCol][2] = kRot(2,iCol);
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::FromAxisAngle (const Vector3<Real>& rkAxis,
    Real fAngle)
{
    // assert:  axis[] is unit length
    //
    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

    Real fHalfAngle = ((Real)0.5)*fAngle;
    Real fSin = Math<Real>::Sin(fHalfAngle);
    m_afTuple[0] = Math<Real>::Cos(fHalfAngle);
    m_afTuple[1] = fSin*rkAxis[0];
    m_afTuple[2] = fSin*rkAxis[1];
    m_afTuple[3] = fSin*rkAxis[2];
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::ToAxisAngle (Vector3<Real>& rkAxis, Real& rfAngle)
    const
{
    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

    Real fSqrLength = m_afTuple[1]*m_afTuple[1] + m_afTuple[2]*m_afTuple[2]
        + m_afTuple[3]*m_afTuple[3];
    if ( fSqrLength > Math<Real>::EPSILON )
    {
        rfAngle = ((Real)2.0)*Math<Real>::ACos(m_afTuple[0]);
        Real fInvLength = Math<Real>::InvSqrt(fSqrLength);
        rkAxis[0] = m_afTuple[1]*fInvLength;
        rkAxis[1] = m_afTuple[2]*fInvLength;
        rkAxis[2] = m_afTuple[3]*fInvLength;
    }
    else
    {
        // angle is 0 (mod 2*pi), so any axis will do
        rfAngle = (Real)0.0;
        rkAxis[0] = (Real)1.0;
        rkAxis[1] = (Real)0.0;
        rkAxis[2] = (Real)0.0;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Real Quaternion<Real>::Dot (const Quaternion& rkQ) const
{
    Real fDot = (Real)0.0;
    for (int i = 0; i < 4; i++)
        fDot += m_afTuple[i]*rkQ.m_afTuple[i];
    return fDot;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Inverse () const
{
    Quaternion<Real> kInverse;

    Real fNorm = (Real)0.0;
    int i;
    for (i = 0; i < 4; i++)
        fNorm += m_afTuple[i]*m_afTuple[i];

    if ( fNorm > (Real)0.0 )
    {
        Real fInvNorm = ((Real)1.0)/fNorm;
        kInverse.m_afTuple[0] = m_afTuple[0]*fInvNorm;
        kInverse.m_afTuple[1] = -m_afTuple[1]*fInvNorm;
        kInverse.m_afTuple[2] = -m_afTuple[2]*fInvNorm;
        kInverse.m_afTuple[3] = -m_afTuple[3]*fInvNorm;
    }
    else
    {
        // return an invalid result to flag the error
        for (i = 0; i < 4; i++)
            kInverse.m_afTuple[i] = (Real)0.0;
    }

    return kInverse;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Conjugate () const
{
    // assert:  'this' is unit length
    return Quaternion<Real>(m_afTuple[0],-m_afTuple[1],-m_afTuple[2],
        -m_afTuple[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Exp () const
{
    // If q = A*(x*i+y*j+z*k) where (x,y,z) is unit length, then
    // exp(q) = cos(A)+sin(A)*(x*i+y*j+z*k).  If sin(A) is near zero,
    // use exp(q) = cos(A)+A*(x*i+y*j+z*k) since A/sin(A) has limit 1.

    Quaternion<Real> kResult;

    Real fAngle = Math<Real>::Sqrt(m_afTuple[1]*m_afTuple[1] +
        m_afTuple[2]*m_afTuple[2] + m_afTuple[3]*m_afTuple[3]);

    Real fSin = Math<Real>::Sin(fAngle);
    kResult.m_afTuple[0] = Math<Real>::Cos(fAngle);

    int i;

    if ( Math<Real>::FAbs(fSin) >= Math<Real>::EPSILON )
    {
        Real fCoeff = fSin/fAngle;
        for (i = 1; i <= 3; i++)
            kResult.m_afTuple[i] = fCoeff*m_afTuple[i];
    }
    else
    {
        for (i = 1; i <= 3; i++)
            kResult.m_afTuple[i] = m_afTuple[i];
    }

    return kResult;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Log () const
{
    // If q = cos(A)+sin(A)*(x*i+y*j+z*k) where (x,y,z) is unit length, then
    // log(q) = A*(x*i+y*j+z*k).  If sin(A) is near zero, use log(q) =
    // sin(A)*(x*i+y*j+z*k) since sin(A)/A has limit 1.

    Quaternion<Real> kResult;
    kResult.m_afTuple[0] = (Real)0.0;

    int i;

    if ( Math<Real>::FAbs(m_afTuple[0]) < (Real)1.0 )
    {
        Real fAngle = Math<Real>::ACos(m_afTuple[0]);
        Real fSin = Math<Real>::Sin(fAngle);
        if ( Math<Real>::FAbs(fSin) >= Math<Real>::EPSILON )
        {
            Real fCoeff = fAngle/fSin;
            for (i = 1; i <= 3; i++)
                kResult.m_afTuple[i] = fCoeff*m_afTuple[i];
            return kResult;
        }
    }

    for (i = 1; i <= 3; i++)
        kResult.m_afTuple[i] = m_afTuple[i];
    return kResult;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Quaternion<Real>::operator* (const Vector3<Real>& rkVector)
    const
{
    // Given a vector u = (x0,y0,z0) and a unit length quaternion
    // q = <w,x,y,z>, the vector v = (x1,y1,z1) which represents the
    // rotation of u by q is v = q*u*q^{-1} where * indicates quaternion
    // multiplication and where u is treated as the quaternion <0,x0,y0,z0>.
    // Note that q^{-1} = <w,-x,-y,-z>, so no real work is required to
    // invert q.  Now
    //
    //   q*u*q^{-1} = q*<0,x0,y0,z0>*q^{-1}
    //     = q*(x0*i+y0*j+z0*k)*q^{-1}
    //     = x0*(q*i*q^{-1})+y0*(q*j*q^{-1})+z0*(q*k*q^{-1})
    //
    // As 3-vectors, q*i*q^{-1}, q*j*q^{-1}, and 2*k*q^{-1} are the columns
    // of the rotation matrix computed in Quaternion::ToRotationMatrix.
    // The vector v is obtained as the product of that rotation matrix with
    // vector u.  As such, the quaternion representation of a rotation
    // matrix requires less space than the matrix and more time to compute
    // the rotated vector.  Typical space-time tradeoff...

    Matrix3<Real> kRot;
    ToRotationMatrix(kRot);
    return kRot*rkVector;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Slerp (Real fT, const Quaternion& rkP,
    const Quaternion& rkQ)
{
    Real fCos = rkP.Dot(rkQ);
    Real fAngle = Math<Real>::ACos(fCos);

    if ( Math<Real>::FAbs(fAngle) < Math<Real>::EPSILON )
        return rkP;

    Real fSin = Math<Real>::Sin(fAngle);
    Real fInvSin = ((Real)1.0)/fSin;
    Real fCoeff0 = Math<Real>::Sin((((Real)1.0)-fT)*fAngle)*fInvSin;
    Real fCoeff1 = Math<Real>::Sin(fT*fAngle)*fInvSin;
    return fCoeff0*rkP + fCoeff1*rkQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::SlerpExtraSpins (Real fT,
    const Quaternion& rkP, const Quaternion& rkQ, int iExtraSpins)
{
    Real fCos = rkP.Dot(rkQ);
    Real fAngle = Math<Real>::ACos(fCos);

    if ( Math<Real>::FAbs(fAngle) < Math<Real>::EPSILON )
        return rkP;

    Real fSin = Math<Real>::Sin(fAngle);
    Real fPhase = Math<Real>::PI*iExtraSpins*fT;
    Real fInvSin = ((Real)1.0)/fSin;
    Real fCoeff0 = Math<Real>::Sin((((Real)1.0)-fT)*fAngle-fPhase)*fInvSin;
    Real fCoeff1 = Math<Real>::Sin(fT*fAngle + fPhase)*fInvSin;
    return fCoeff0*rkP + fCoeff1*rkQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetIntermediate (const Quaternion& rkQ0,
    const Quaternion& rkQ1, const Quaternion& rkQ2)
{
    // assert:  Q0, Q1, Q2 all unit-length
    Quaternion<Real> kQ1Inv = rkQ1.Conjugate();
    Quaternion<Real> kP0 = kQ1Inv*rkQ0;
    Quaternion<Real> kP2 = kQ1Inv*rkQ2;
    Quaternion<Real> kArg = -((Real)0.25)*(kP0.Log()+kP2.Log());
    Quaternion<Real> kA = rkQ1*kArg.Exp();
    return kA;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Squad (Real fT, const Quaternion& rkQ0,
    const Quaternion& rkA0, const Quaternion& rkA1, const Quaternion& rkQ1)
{
    Real fSlerpT = ((Real)2.0)*fT*(((Real)1.0)-fT);
    Quaternion<Real> kSlerpP = Slerp(fT,rkQ0,rkQ1);
    Quaternion<Real> kSlerpQ = Slerp(fT,rkA0,rkA1);
    return Slerp(fSlerpT,kSlerpP,kSlerpQ);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Align (const Vector3<Real>& rkV1,
    const Vector3<Real>& rkV2)
{
    // If V1 and V2 are not parallel, the axis of rotation is the unit-length
    // vector U = Cross(V1,V2)/Length(Cross(V1,V2)).  The angle of rotation,
    // A, is the angle between V1 and V2.  The quaternion for the rotation is
    // q = cos(A/2) + sin(A/2)*(ux*i+uy*j+uz*k) where U = (ux,uy,uz).
    //
    // (1) Rather than extract A = acos(Dot(V1,V2)), multiply by 1/2, then
    //     compute sin(A/2) and cos(A/2), we reduce the computational costs by
    //     computing the bisector B = (V1+V2)/Length(V1+V2), so cos(A/2) =
    //     Dot(V1,B).
    //
    // (2) The rotation axis is U = Cross(V1,B)/Length(Cross(V1,B)), but
    //     Length(Cross(V1,B)) = Length(V1)*Length(B)*sin(A/2) = sin(A/2), in
    //     which case sin(A/2)*(ux*i+uy*j+uz*k) = (cx*i+cy*j+cz*k) where
    //     C = Cross(V1,B).
    //
    // If V1 and V2 are parallel, or nearly parallel as far as the floating
    // point calculations are concerned, the calculation of B will produce
    // the zero vector: Vector3::Normalize checks for closeness to zero and
    // returns the zero vector accordingly.  Thus, we test for parallelism
    // by checking if cos(A/2) is zero.  The test for exactly zero is usually
    // not recommend for floating point arithmetic, but the implementation of
    // Vector3::Normalize guarantees the comparison is robust.

    Vector3<Real> kBisector = rkV1 + rkV2;
    kBisector.Normalize();

    Real fCosHalfAngle = rkV1.Dot(kBisector);
    Vector3<Real> kCross;

    if ( fCosHalfAngle != (Real)0.0 )
        kCross = rkV1.Cross(kBisector);
    else
        kCross = rkV1.UnitCross(Vector3<Real>(rkV2.Z(),rkV2.X(),rkV2.Y()));

    return Quaternion(fCosHalfAngle,kCross.X(),kCross.Y(),kCross.Z());
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::DecomposeTwistTimesNoTwist (
    const Vector3<Real>& rkAxis, Quaternion& rkTwist, Quaternion& rkNoTwist)
{
    Vector3<Real> kRotatedAxis = (*this)*rkAxis;
    rkNoTwist = Align(rkAxis,kRotatedAxis);
    rkTwist = (*this)*rkNoTwist.Conjugate();
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::DecomposeNoTwistTimesTwist (
    const Vector3<Real>& rkAxis, Quaternion& rkTwist, Quaternion& rkNoTwist)
{
    Vector3<Real> kRotatedAxis = (*this)*rkAxis;
    rkNoTwist = Align(rkAxis,kRotatedAxis);
    rkTwist = rkNoTwist.Conjugate()*(*this);
}
//----------------------------------------------------------------------------
