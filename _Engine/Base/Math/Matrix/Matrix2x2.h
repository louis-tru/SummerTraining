/*
=============================================================================
	File:	Matrix2x2.h
	Desc:	Row-major float2x2 matrix.

	Originally written by Id Software.
	Copyright (C) 2004 Id Software, Inc. ( idMat2 )
=============================================================================
*/

#ifndef __MATH_MATRIX_2x2_H__
#define __MATH_MATRIX_2x2_H__
mxSWIPED("idSoftware, Doom3/Prey/Quake4 SDKs");


//===============================================================
//
//	Matrix2 - 2x2 matrix ( row-major ).
//
//===============================================================

class Matrix2 {
public:
					Matrix2( void );
					explicit Matrix2( const Vec2D &x, const Vec2D &y );
					explicit Matrix2( const FLOAT xx, const FLOAT xy, const FLOAT yx, const FLOAT yy );
					explicit Matrix2( const FLOAT src[ 2 ][ 2 ] );

	const Vec2D &	operator[]( INT index ) const;
	Vec2D &			operator[]( INT index );
	Matrix2			operator-() const;
	Matrix2			operator*( const FLOAT a ) const;
	Vec2D			operator*( const Vec2D &vec ) const;
	Matrix2			operator*( const Matrix2 &a ) const;
	Matrix2			operator+( const Matrix2 &a ) const;
	Matrix2			operator-( const Matrix2 &a ) const;
	Matrix2 &		operator*=( const FLOAT a );
	Matrix2 &		operator*=( const Matrix2 &a );
	Matrix2 &		operator+=( const Matrix2 &a );
	Matrix2 &		operator-=( const Matrix2 &a );

	friend Matrix2	operator*( const FLOAT a, const Matrix2 &mRows );
	friend Vec2D	operator*( const Vec2D &vec, const Matrix2 &mRows );
	friend Vec2D &	operator*=( Vec2D &vec, const Matrix2 &mRows );

	bool			Compare( const Matrix2 &a ) const;						// exact compare, no epsilon
	bool			Compare( const Matrix2 &a, const FLOAT epsilon ) const;	// compare with epsilon
	bool			operator==( const Matrix2 &a ) const;					// exact compare, no epsilon
	bool			operator!=( const Matrix2 &a ) const;					// exact compare, no epsilon

	void			SetZero( void );
	void			SetIdentity( void );
	bool			IsIdentity( const FLOAT epsilon = MATRIX_EPSILON ) const;
	bool			IsSymmetric( const FLOAT epsilon = MATRIX_EPSILON ) const;
	bool			IsDiagonal( const FLOAT epsilon = MATRIX_EPSILON ) const;

	FLOAT			Trace( void ) const;
	FLOAT			Determinant( void ) const;
	Matrix2			Transpose( void ) const;	// returns transpose
	Matrix2 &		TransposeSelf( void );
	Matrix2			Inverse( void ) const;		// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseSelf( void );		// returns false if determinant is zero
	Matrix2			InverseFast( void ) const;	// returns the inverse ( m * m.Inverse() = identity )
	bool			InverseFastSelf( void );	// returns false if determinant is zero

	INT				GetDimension( void ) const;

	void			SetRow( UINT iRow, const Vec2D& v );

	const FLOAT *	ToPtr( void ) const;
	FLOAT *			ToPtr( void );

public:
	static const Matrix2	mat2_zero;
	static const Matrix2	mat2_identity;

	//for reflection
	mxREFLECTION_IS_MY_FRIEND(Matrix2);

private:
	Vec2D			mRows[ 2 ];
};

mxDECLARE_STRUCT( Matrix2 );

mxFORCEINLINE Matrix2::Matrix2( void ) {
}

mxFORCEINLINE Matrix2::Matrix2( const Vec2D &x, const Vec2D &y ) {
	mRows[ 0 ].x = x.x; mRows[ 0 ].y = x.y;
	mRows[ 1 ].x = y.x; mRows[ 1 ].y = y.y;
}

mxFORCEINLINE Matrix2::Matrix2( const FLOAT xx, const FLOAT xy, const FLOAT yx, const FLOAT yy ) {
	mRows[ 0 ].x = xx; mRows[ 0 ].y = xy;
	mRows[ 1 ].x = yx; mRows[ 1 ].y = yy;
}

mxFORCEINLINE Matrix2::Matrix2( const FLOAT src[ 2 ][ 2 ] ) {
	memcpy( mRows, src, 2 * 2 * sizeof( FLOAT ) );
}

mxFORCEINLINE const Vec2D &Matrix2::operator[]( INT index ) const {
	//mxASSERT( ( index >= 0 ) && ( index < 2 ) );
	return mRows[ index ];
}

mxFORCEINLINE Vec2D &Matrix2::operator[]( INT index ) {
	//mxASSERT( ( index >= 0 ) && ( index < 2 ) );
	return mRows[ index ];
}

mxFORCEINLINE Matrix2 Matrix2::operator-() const {
	return Matrix2(	-mRows[0][0], -mRows[0][1],
					-mRows[1][0], -mRows[1][1] );
}

mxFORCEINLINE Vec2D Matrix2::operator*( const Vec2D &vec ) const {
	return Vec2D(
		mRows[ 0 ].x * vec.x + mRows[ 0 ].y * vec.y,
		mRows[ 1 ].x * vec.x + mRows[ 1 ].y * vec.y );
}

mxFORCEINLINE Matrix2 Matrix2::operator*( const Matrix2 &a ) const {
	return Matrix2(
		mRows[0].x * a[0].x + mRows[0].y * a[1].x,
		mRows[0].x * a[0].y + mRows[0].y * a[1].y,
		mRows[1].x * a[0].x + mRows[1].y * a[1].x,
		mRows[1].x * a[0].y + mRows[1].y * a[1].y );
}

mxFORCEINLINE Matrix2 Matrix2::operator*( const FLOAT a ) const {
	return Matrix2(
		mRows[0].x * a, mRows[0].y * a, 
		mRows[1].x * a, mRows[1].y * a );
}

mxFORCEINLINE Matrix2 Matrix2::operator+( const Matrix2 &a ) const {
	return Matrix2(
		mRows[0].x + a[0].x, mRows[0].y + a[0].y, 
		mRows[1].x + a[1].x, mRows[1].y + a[1].y );
}
    
mxFORCEINLINE Matrix2 Matrix2::operator-( const Matrix2 &a ) const {
	return Matrix2(
		mRows[0].x - a[0].x, mRows[0].y - a[0].y,
		mRows[1].x - a[1].x, mRows[1].y - a[1].y );
}

mxFORCEINLINE Matrix2 &Matrix2::operator*=( const FLOAT a ) {
	mRows[0].x *= a; mRows[0].y *= a;
	mRows[1].x *= a; mRows[1].y *= a;

    return *this;
}

mxFORCEINLINE Matrix2 &Matrix2::operator*=( const Matrix2 &a ) {
	FLOAT x, y;
	x = mRows[0].x; y = mRows[0].y;
	mRows[0].x = x * a[0].x + y * a[1].x;
	mRows[0].y = x * a[0].y + y * a[1].y;
	x = mRows[1].x; y = mRows[1].y;
	mRows[1].x = x * a[0].x + y * a[1].x;
	mRows[1].y = x * a[0].y + y * a[1].y;
	return *this;
}

mxFORCEINLINE Matrix2 &Matrix2::operator+=( const Matrix2 &a ) {
	mRows[0].x += a[0].x; mRows[0].y += a[0].y;
	mRows[1].x += a[1].x; mRows[1].y += a[1].y;

    return *this;
}

mxFORCEINLINE Matrix2 &Matrix2::operator-=( const Matrix2 &a ) {
	mRows[0].x -= a[0].x; mRows[0].y -= a[0].y;
	mRows[1].x -= a[1].x; mRows[1].y -= a[1].y;

    return *this;
}

mxFORCEINLINE Vec2D operator*( const Vec2D &vec, const Matrix2 &mRows ) {
	return mRows * vec;
}

mxFORCEINLINE Matrix2 operator*( const FLOAT a, Matrix2 const &mRows ) {
	return mRows * a;
}

mxFORCEINLINE Vec2D &operator*=( Vec2D &vec, const Matrix2 &mRows ) {
	vec = mRows * vec;
	return vec;
}

mxFORCEINLINE bool Matrix2::Compare( const Matrix2 &a ) const {
	if ( mRows[0].Compare( a[0] ) &&
		mRows[1].Compare( a[1] ) ) {
		return true;
	}
	return false;
}

mxFORCEINLINE bool Matrix2::Compare( const Matrix2 &a, const FLOAT epsilon ) const {
	if ( mRows[0].Compare( a[0], epsilon ) &&
		mRows[1].Compare( a[1], epsilon ) ) {
		return true;
	}
	return false;
}

mxFORCEINLINE bool Matrix2::operator==( const Matrix2 &a ) const {
	return Compare( a );
}

mxFORCEINLINE bool Matrix2::operator!=( const Matrix2 &a ) const {
	return !Compare( a );
}

mxFORCEINLINE void Matrix2::SetZero( void ) {
	mRows[0].SetZero();
	mRows[1].SetZero();
}

mxFORCEINLINE void Matrix2::SetIdentity( void ) {
	*this = mat2_identity;
}

mxFORCEINLINE bool Matrix2::IsIdentity( const FLOAT epsilon ) const {
	return Compare( mat2_identity, epsilon );
}

mxFORCEINLINE bool Matrix2::IsSymmetric( const FLOAT epsilon ) const {
	return ( fabs( mRows[0][1] - mRows[1][0] ) < epsilon );
}

mxFORCEINLINE bool Matrix2::IsDiagonal( const FLOAT epsilon ) const {
	if ( fabs( mRows[0][1] ) > epsilon ||
		fabs( mRows[1][0] ) > epsilon ) {
		return false;
	}
	return true;
}

mxFORCEINLINE FLOAT Matrix2::Trace( void ) const {
	return ( mRows[0][0] + mRows[1][1] );
}

mxFORCEINLINE FLOAT Matrix2::Determinant( void ) const {
	return mRows[0][0] * mRows[1][1] - mRows[0][1] * mRows[1][0];
}

mxFORCEINLINE Matrix2 Matrix2::Transpose( void ) const {
	return Matrix2(	mRows[0][0], mRows[1][0],
					mRows[0][1], mRows[1][1] );
}

mxFORCEINLINE Matrix2 &Matrix2::TransposeSelf( void ) {
	FLOAT tmp;

	tmp = mRows[0][1];
	mRows[0][1] = mRows[1][0];
	mRows[1][0] = tmp;

	return *this;
}

mxFORCEINLINE Matrix2 Matrix2::Inverse( void ) const {
	Matrix2 invMat;

	invMat = *this;
	//HH rww SDK - removed unnecessary variable initialization
	invMat.InverseSelf();
	return invMat;
}

mxFORCEINLINE Matrix2 Matrix2::InverseFast( void ) const {
	Matrix2 invMat;

	invMat = *this;
	//HH rww SDK - removed unnecessary variable initialization
	invMat.InverseFastSelf();
	return invMat;
}

mxFORCEINLINE INT Matrix2::GetDimension( void ) const {
	return 4;
}

mxFORCEINLINE void Matrix2::SetRow( UINT iRow, const Vec2D& v )
{
	mRows[ iRow ] = v;
}

mxFORCEINLINE const FLOAT *Matrix2::ToPtr( void ) const {
	return mRows[0].ToPtr();
}

mxFORCEINLINE FLOAT *Matrix2::ToPtr( void ) {
	return mRows[0].ToPtr();
}



#endif /* !__MATH_MATRIX_2x2_H__ */

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
