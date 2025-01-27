// Copyright 2014-2015 Isis Innovation Limited and the authors of InfiniTAM

#pragma once

#include <math.h>
#include <ostream>

#include "PlatformIndependence.h"

namespace ORUtils {
	//////////////////////////////////////////////////////////////////////////
	//						Basic Vector Structure
	//////////////////////////////////////////////////////////////////////////

	template <class T> struct Vector2_{
		union {
			struct { T x, y; }; // standard names for components
			struct { T s, t; }; // standard names for components
			struct { T width, height; };
			T v[2];     // array access
		};
	};

	template <class T> struct Vector3_{
		union {
			struct{ T x, y, z; }; // standard names for components
			struct{ T r, g, b; }; // standard names for components
			struct{ T s, t, p; }; // standard names for components
			T v[3];
		};
	};

	template <class T> struct Vector4_ {
		union {
			struct { T x, y, z, w; }; // standard names for components
			struct { T r, g, b, a; }; // standard names for components
			struct { T s, t, p, q; }; // standard names for components
			T v[4];
		};
	};

	template <class T> struct Vector6_ {
		//union {
		T v[6];
		//};
	};

	template<class T, int s> struct VectorX_
	{
		int vsize;
		T v[s];
	};

	//////////////////////////////////////////////////////////////////////////
	// Vector class with math operators: +, -, *, /, +=, -=, /=, [], ==, !=, T*(), etc.
	//////////////////////////////////////////////////////////////////////////
	template <class T> class Vector2 : public Vector2_ < T >
	{
	public:
		typedef T value_type;
		CPU_AND_GPU inline int size() const { return 2; }

		////////////////////////////////////////////////////////
		//  Constructors
		////////////////////////////////////////////////////////
		CPU_AND_GPU Vector2(){} // Default constructor
		CPU_AND_GPU Vector2(const T &t) { this->x = t; this->y = t; } // Scalar constructor
		CPU_AND_GPU Vector2(const T *tp) { this->x = tp[0]; this->y = tp[1]; } // Construct from array			            
		CPU_AND_GPU Vector2(const T v0, const T v1) { this->x = v0; this->y = v1; } // Construct from explicit values
		CPU_AND_GPU Vector2(const Vector2_<T> &v) { this->x = v.x; this->y = v.y; }// copy constructor

		CPU_AND_GPU explicit Vector2(const Vector3_<T> &u)  { this->x = u.x; this->y = u.y; }
		CPU_AND_GPU explicit Vector2(const Vector4_<T> &u)  { this->x = u.x; this->y = u.y; }

		CPU_AND_GPU inline Vector2<int> toInt() const {
			return Vector2<int>((int)ROUND(this->x), (int)ROUND(this->y));
		}

		CPU_AND_GPU inline Vector2<int> toIntFloor() const {
			return Vector2<int>((int)floor(this->x), (int)floor(this->y));
		}

		CPU_AND_GPU inline Vector2<unsigned char> toUChar() const {
			Vector2<int> vi = toInt(); return Vector2<unsigned char>((unsigned char)CLAMP(vi.x, 0, 255), (unsigned char)CLAMP(vi.y, 0, 255));
		}

		CPU_AND_GPU inline Vector2<float> toFloat() const {
			return Vector2<float>((float)this->x, (float)this->y);
		}

		CPU_AND_GPU const T *getValues() const { return this->v; }
		CPU_AND_GPU Vector2<T> &setValues(const T *rhs) { this->x = rhs[0]; this->y = rhs[1]; return *this; }

        CPU_AND_GPU T area() const {
            return x * y;
        }
		// indexing operators
		CPU_AND_GPU T &operator [](int i) { return this->v[i]; }
		CPU_AND_GPU const T &operator [](int i) const { return this->v[i]; }

		// type-cast operators
		CPU_AND_GPU operator T *() { return this->v; }
		CPU_AND_GPU operator const T *() const { return this->v; }

		////////////////////////////////////////////////////////
		//  Math operators
		////////////////////////////////////////////////////////

		// scalar multiply assign
		CPU_AND_GPU friend Vector2<T> &operator *= (const Vector2<T> &lhs, T d) {
			lhs.x *= d; lhs.y *= d; return lhs;
		}

		// component-wise vector multiply assign
		CPU_AND_GPU friend Vector2<T> &operator *= (Vector2<T> &lhs, const Vector2<T> &rhs) {
			lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs;
		}

		// scalar divide assign
		CPU_AND_GPU friend Vector2<T> &operator /= (Vector2<T> &lhs, T d) {
			if (d == 0) return lhs; lhs.x /= d; lhs.y /= d; return lhs;
		}

		// component-wise vector divide assign
		CPU_AND_GPU friend Vector2<T> &operator /= (Vector2<T> &lhs, const Vector2<T> &rhs) {
			lhs.x /= rhs.x; lhs.y /= rhs.y;	return lhs;
		}

		// component-wise vector add assign
		CPU_AND_GPU friend Vector2<T> &operator += (Vector2<T> &lhs, const Vector2<T> &rhs) {
			lhs.x += rhs.x; lhs.y += rhs.y;	return lhs;
		}

		// component-wise vector subtract assign
		CPU_AND_GPU friend Vector2<T> &operator -= (Vector2<T> &lhs, const Vector2<T> &rhs) {
			lhs.x -= rhs.x; lhs.y -= rhs.y;	return lhs;
		}

		// unary negate
		CPU_AND_GPU friend Vector2<T> operator - (const Vector2<T> &rhs) {
			Vector2<T> rv;	rv.x = -rhs.x; rv.y = -rhs.y; return rv;
		}

		// vector add
		CPU_AND_GPU friend Vector2<T> operator + (const Vector2<T> &lhs, const Vector2<T> &rhs)  {
			Vector2<T> rv(lhs); return rv += rhs;
		}

		// vector subtract
		CPU_AND_GPU friend Vector2<T> operator - (const Vector2<T> &lhs, const Vector2<T> &rhs) {
			Vector2<T> rv(lhs); return rv -= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector2<T> operator * (const Vector2<T> &lhs, T rhs) {
			Vector2<T> rv(lhs); return rv *= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector2<T> operator * (T lhs, const Vector2<T> &rhs) {
			Vector2<T> rv(lhs); return rv *= rhs;
		}

		// vector component-wise multiply
		CPU_AND_GPU friend Vector2<T> operator * (const Vector2<T> &lhs, const Vector2<T> &rhs) {
			Vector2<T> rv(lhs); return rv *= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector2<T> operator / (const Vector2<T> &lhs, T rhs) {
			Vector2<T> rv(lhs); return rv /= rhs;
		}

		// vector component-wise multiply
		CPU_AND_GPU friend Vector2<T> operator / (const Vector2<T> &lhs, const Vector2<T> &rhs) {
			Vector2<T> rv(lhs); return rv /= rhs;
		}

		////////////////////////////////////////////////////////
		//  Comparison operators
		////////////////////////////////////////////////////////

		// equality
		CPU_AND_GPU friend bool operator == (const Vector2<T> &lhs, const Vector2<T> &rhs) {
			return (lhs.x == rhs.x) && (lhs.y == rhs.y);
		}

		// inequality
		CPU_AND_GPU friend bool operator != (const Vector2<T> &lhs, const Vector2<T> &rhs) {
			return (lhs.x != rhs.x) || (lhs.y != rhs.y);
		}


		friend std::ostream& operator<<(std::ostream& os, const Vector2<T>& dt){
			os << dt.x << ", " << dt.y;
			return os;
		}
	};

	template <class T> class Vector3 : public Vector3_ < T >
	{
	public:
		typedef T value_type;
		CPU_AND_GPU inline int size() const { return 3; }

		////////////////////////////////////////////////////////
		//  Constructors
		////////////////////////////////////////////////////////
		CPU_AND_GPU Vector3(){} // Default constructor
		CPU_AND_GPU Vector3(const T &t)	{ this->x = t; this->y = t; this->z = t; } // Scalar constructor
		CPU_AND_GPU Vector3(const T *tp) { this->x = tp[0]; this->y = tp[1]; this->z = tp[2]; } // Construct from array
		CPU_AND_GPU Vector3(const T v0, const T v1, const T v2) { this->x = v0; this->y = v1; this->z = v2; } // Construct from explicit values
		CPU_AND_GPU explicit Vector3(const Vector4_<T> &u)	{ this->x = u.x; this->y = u.y; this->z = u.z; }
		CPU_AND_GPU explicit Vector3(const Vector2_<T> &u, T v0) { this->x = u.x; this->y = u.y; this->z = v0; }

		CPU_AND_GPU inline Vector3<int> toIntRound() const {
			return Vector3<int>((int)ROUND(this->x), (int)ROUND(this->y), (int)ROUND(this->z));
		}

		CPU_AND_GPU inline Vector3<int> toInt() const {
			return Vector3<int>((int)(this->x), (int)(this->y), (int)(this->z));
		}

		CPU_AND_GPU inline Vector3<int> toInt(Vector3<float> &residual) const {
			Vector3<int> intRound = toInt();
			residual = Vector3<float>(this->x - intRound.x, this->y - intRound.y, this->z - intRound.z);
			return intRound;
		}

		CPU_AND_GPU inline Vector3<short> toShortRound() const {
			return Vector3<short>((short)ROUND(this->x), (short)ROUND(this->y), (short)ROUND(this->z));
		}

		CPU_AND_GPU inline Vector3<short> toShortFloor() const {
			return Vector3<short>((short)floor(this->x), (short)floor(this->y), (short)floor(this->z));
		}

		CPU_AND_GPU inline Vector3<int> toIntFloor() const {
			return Vector3<int>((int)floor(this->x), (int)floor(this->y), (int)floor(this->z));
		}

        /// Floors the coordinates to integer values, returns this and the residual float.
        /// Use like
        /// TO_INT_FLOOR3(int_xyz, residual_xyz, xyz)
        /// for xyz === this
		CPU_AND_GPU inline Vector3<int> toIntFloor(Vector3<float> &residual) const {
			Vector3<float> intFloor(floor(this->x), floor(this->y), floor(this->z));
			residual = *this - intFloor;
			return Vector3<int>((int)intFloor.x, (int)intFloor.y, (int)intFloor.z);
		}

		CPU_AND_GPU inline Vector3<unsigned char> toUChar() const {
			Vector3<int> vi = toIntRound(); return Vector3<unsigned char>((unsigned char)CLAMP(vi.x, 0, 255), (unsigned char)CLAMP(vi.y, 0, 255), (unsigned char)CLAMP(vi.z, 0, 255));
		}

		CPU_AND_GPU inline Vector3<float> toFloat() const {
			return Vector3<float>((float)this->x, (float)this->y, (float)this->z);
		}

		CPU_AND_GPU inline Vector3<float> normalised() const {
			float norm = 1.0f / sqrt((float)(this->x * this->x + this->y * this->y + this->z * this->z));
			return Vector3<float>((float)this->x * norm, (float)this->y * norm, (float)this->z * norm);
		}

		CPU_AND_GPU const T *getValues() const	{ return this->v; }
		CPU_AND_GPU Vector3<T> &setValues(const T *rhs) { this->x = rhs[0]; this->y = rhs[1]; this->z = rhs[2]; return *this; }

		// indexing operators
		CPU_AND_GPU T &operator [](int i) { return this->v[i]; }
		CPU_AND_GPU const T &operator [](int i) const { return this->v[i]; }

		// type-cast operators
		CPU_AND_GPU operator T *()	{ return this->v; }
		CPU_AND_GPU operator const T *() const { return this->v; }

		////////////////////////////////////////////////////////
		//  Math operators
		////////////////////////////////////////////////////////

		// scalar multiply assign
		CPU_AND_GPU friend Vector3<T> &operator *= (Vector3<T> &lhs, T d)	{
			lhs.x *= d; lhs.y *= d; lhs.z *= d; return lhs;
		}

		// component-wise vector multiply assign
		CPU_AND_GPU friend Vector3<T> &operator *= (Vector3<T> &lhs, const Vector3<T> &rhs) {
			lhs.x *= rhs.x; lhs.y *= rhs.y; lhs.z *= rhs.z; return lhs;
		}

		// scalar divide assign
		CPU_AND_GPU friend Vector3<T> &operator /= (Vector3<T> &lhs, T d) {
			lhs.x /= d; lhs.y /= d; lhs.z /= d; return lhs;
		}

		// component-wise vector divide assign
		CPU_AND_GPU friend Vector3<T> &operator /= (Vector3<T> &lhs, const Vector3<T> &rhs)	{
			lhs.x /= rhs.x; lhs.y /= rhs.y; lhs.z /= rhs.z; return lhs;
		}

		// component-wise vector add assign
		CPU_AND_GPU friend Vector3<T> &operator += (Vector3<T> &lhs, const Vector3<T> &rhs)	{
			lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z; return lhs;
		}

		// component-wise vector subtract assign
		CPU_AND_GPU friend Vector3<T> &operator -= (Vector3<T> &lhs, const Vector3<T> &rhs) {
			lhs.x -= rhs.x; lhs.y -= rhs.y; lhs.z -= rhs.z; return lhs;
		}

		// unary negate
		CPU_AND_GPU friend Vector3<T> operator - (const Vector3<T> &rhs)	{
			Vector3<T> rv; rv.x = -rhs.x; rv.y = -rhs.y; rv.z = -rhs.z; return rv;
		}

		// vector add
		CPU_AND_GPU friend Vector3<T> operator + (const Vector3<T> &lhs, const Vector3<T> &rhs){
			Vector3<T> rv(lhs); return rv += rhs;
		}

		// vector subtract
		CPU_AND_GPU friend Vector3<T> operator - (const Vector3<T> &lhs, const Vector3<T> &rhs){
			Vector3<T> rv(lhs); return rv -= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector3<T> operator * (const Vector3<T> &lhs, T rhs) {
			Vector3<T> rv(lhs); return rv *= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector3<T> operator * (T lhs, const Vector3<T> &rhs) {
			Vector3<T> rv(lhs); return rv *= rhs;
		}

		// vector component-wise multiply
		CPU_AND_GPU friend Vector3<T> operator * (const Vector3<T> &lhs, const Vector3<T> &rhs)	{
			Vector3<T> rv(lhs); return rv *= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector3<T> operator / (const Vector3<T> &lhs, T rhs) {
			Vector3<T> rv(lhs); return rv /= rhs;
		}

		// vector component-wise multiply
		CPU_AND_GPU friend Vector3<T> operator / (const Vector3<T> &lhs, const Vector3<T> &rhs) {
			Vector3<T> rv(lhs); return rv /= rhs;
		}

		////////////////////////////////////////////////////////
		//  Comparison operators
		////////////////////////////////////////////////////////

		// inequality
		CPU_AND_GPU friend bool operator != (const Vector3<T> &lhs, const Vector3<T> &rhs) {
			return (lhs.x != rhs.x) || (lhs.y != rhs.y) || (lhs.z != rhs.z);
		}

		////////////////////////////////////////////////////////////////////////////////
		// dimension specific operations
		////////////////////////////////////////////////////////////////////////////////


		friend std::ostream& operator<<(std::ostream& os, const Vector3<T>& dt){
			os << dt.x << ", " << dt.y << ", " << dt.z;
			return os;
		}
	};

	////////////////////////////////////////////////////////
	//  Non-member comparison operators
	////////////////////////////////////////////////////////

	// equality
	template <typename T1, typename T2> CPU_AND_GPU inline bool operator == (const Vector3<T1> &lhs, const Vector3<T2> &rhs){
		return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
	}

	template <class T> class Vector4 : public Vector4_ < T >
	{
	public:
		typedef T value_type;
		CPU_AND_GPU inline int size() const { return 4; }

		////////////////////////////////////////////////////////
		//  Constructors
		////////////////////////////////////////////////////////

		CPU_AND_GPU Vector4() {} // Default constructor
		CPU_AND_GPU Vector4(const T &t) { this->x = t; this->y = t; this->z = t; this->w = t; } //Scalar constructor
		CPU_AND_GPU Vector4(const T *tp) { this->x = tp[0]; this->y = tp[1]; this->z = tp[2]; this->w = tp[3]; } // Construct from array
		CPU_AND_GPU Vector4(const T v0, const T v1, const T v2, const T v3) { this->x = v0; this->y = v1; this->z = v2; this->w = v3; } // Construct from explicit values
		CPU_AND_GPU explicit Vector4(const Vector3_<T> &u, T v0) { this->x = u.x; this->y = u.y; this->z = u.z; this->w = v0; }
		CPU_AND_GPU explicit Vector4(const Vector2_<T> &u, T v0, T v1) { this->x = u.x; this->y = u.y; this->z = v0; this->w = v1; }

		CPU_AND_GPU inline Vector4<int> toIntRound() const {
			return Vector4<int>((int)ROUND(this->x), (int)ROUND(this->y), (int)ROUND(this->z), (int)ROUND(this->w));
		}

		CPU_AND_GPU inline Vector4<unsigned char> toUChar() const {
			Vector4<int> vi = toIntRound(); return Vector4<unsigned char>((unsigned char)CLAMP(vi.x, 0, 255), (unsigned char)CLAMP(vi.y, 0, 255), (unsigned char)CLAMP(vi.z, 0, 255), (unsigned char)CLAMP(vi.w, 0, 255));
		}

		CPU_AND_GPU inline Vector4<float> toFloat() const {
			return Vector4<float>((float)this->x, (float)this->y, (float)this->z, (float)this->w);
		}

		CPU_AND_GPU inline Vector4<T> homogeneousCoordinatesNormalize() const {
			return (this->w <= 0) ? *this : Vector4<T>(this->x / this->w, this->y / this->w, this->z / this->w, 1);
		}

		CPU_AND_GPU inline Vector3<T> toVector3() const {
			return Vector3<T>(this->x, this->y, this->z);
		}

		CPU_AND_GPU const T *getValues() const { return this->v; }
		CPU_AND_GPU Vector4<T> &setValues(const T *rhs) { this->x = rhs[0]; this->y = rhs[1]; this->z = rhs[2]; this->w = rhs[3]; return *this; }

		// indexing operators
		CPU_AND_GPU T &operator [](int i) { return this->v[i]; }
		CPU_AND_GPU const T &operator [](int i) const { return this->v[i]; }

		// type-cast operators
		CPU_AND_GPU operator T *() { return this->v; }
		CPU_AND_GPU operator const T *() const { return this->v; }

		////////////////////////////////////////////////////////
		//  Math operators
		////////////////////////////////////////////////////////

		// scalar multiply assign
		CPU_AND_GPU friend Vector4<T> &operator *= (Vector4<T> &lhs, T d) {
			lhs.x *= d; lhs.y *= d; lhs.z *= d; lhs.w *= d; return lhs;
		}

		// component-wise vector multiply assign
		CPU_AND_GPU friend Vector4<T> &operator *= (Vector4<T> &lhs, const Vector4<T> &rhs) {
			lhs.x *= rhs.x; lhs.y *= rhs.y; lhs.z *= rhs.z; lhs.w *= rhs.w; return lhs;
		}

		// scalar divide assign
		CPU_AND_GPU friend Vector4<T> &operator /= (Vector4<T> &lhs, T d){
			lhs.x /= d; lhs.y /= d; lhs.z /= d; lhs.w /= d; return lhs;
		}

		// component-wise vector divide assign
		CPU_AND_GPU friend Vector4<T> &operator /= (Vector4<T> &lhs, const Vector4<T> &rhs) {
			lhs.x /= rhs.x; lhs.y /= rhs.y; lhs.z /= rhs.z; lhs.w /= rhs.w; return lhs;
		}

		// component-wise vector add assign
		CPU_AND_GPU friend Vector4<T> &operator += (Vector4<T> &lhs, const Vector4<T> &rhs)	{
			lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z; lhs.w += rhs.w; return lhs;
		}

		// component-wise vector subtract assign
		CPU_AND_GPU friend Vector4<T> &operator -= (Vector4<T> &lhs, const Vector4<T> &rhs)	{
			lhs.x -= rhs.x; lhs.y -= rhs.y; lhs.z -= rhs.z; lhs.w -= rhs.w; return lhs;
		}

		// unary negate
		CPU_AND_GPU friend Vector4<T> operator - (const Vector4<T> &rhs)	{
			Vector4<T> rv; rv.x = -rhs.x; rv.y = -rhs.y; rv.z = -rhs.z; rv.w = -rhs.w; return rv;
		}

		// vector add
		CPU_AND_GPU friend Vector4<T> operator + (const Vector4<T> &lhs, const Vector4<T> &rhs) {
			Vector4<T> rv(lhs); return rv += rhs;
		}

		// vector subtract
		CPU_AND_GPU friend Vector4<T> operator - (const Vector4<T> &lhs, const Vector4<T> &rhs) {
			Vector4<T> rv(lhs); return rv -= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector4<T> operator * (const Vector4<T> &lhs, T rhs) {
			Vector4<T> rv(lhs); return rv *= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector4<T> operator * (T lhs, const Vector4<T> &rhs) {
			Vector4<T> rv(lhs); return rv *= rhs;
		}

		// vector component-wise multiply
		CPU_AND_GPU friend Vector4<T> operator * (const Vector4<T> &lhs, const Vector4<T> &rhs) {
			Vector4<T> rv(lhs); return rv *= rhs;
		}

		// scalar divide
		CPU_AND_GPU friend Vector4<T> operator / (const Vector4<T> &lhs, T rhs) {
			Vector4<T> rv(lhs); return rv /= rhs;
		}

		// vector component-wise divide
		CPU_AND_GPU friend Vector4<T> operator / (const Vector4<T> &lhs, const Vector4<T> &rhs) {
			Vector4<T> rv(lhs); return rv /= rhs;
		}

		////////////////////////////////////////////////////////
		//  Comparison operators
		////////////////////////////////////////////////////////

		// equality
		CPU_AND_GPU friend bool operator == (const Vector4<T> &lhs, const Vector4<T> &rhs) {
			return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z) && (lhs.w == rhs.w);
		}

		// inequality
		CPU_AND_GPU friend bool operator != (const Vector4<T> &lhs, const Vector4<T> &rhs) {
			return (lhs.x != rhs.x) || (lhs.y != rhs.y) || (lhs.z != rhs.z) || (lhs.w != rhs.w);
		}

		friend std::ostream& operator<<(std::ostream& os, const Vector4<T>& dt){
			os << dt.x << ", " << dt.y << ", " << dt.z << ", " << dt.w;
			return os;
		}
	};

	template <class T> class Vector6 : public Vector6_ < T >
	{
	public:
		typedef T value_type;
		CPU_AND_GPU inline int size() const { return 6; }

		////////////////////////////////////////////////////////
		//  Constructors
		////////////////////////////////////////////////////////

		CPU_AND_GPU Vector6() {} // Default constructor
		CPU_AND_GPU Vector6(const T &t) { this->v[0] = t; this->v[1] = t; this->v[2] = t; this->v[3] = t; this->v[4] = t; this->v[5] = t; } //Scalar constructor
		CPU_AND_GPU Vector6(const T *tp) { this->v[0] = tp[0]; this->v[1] = tp[1]; this->v[2] = tp[2]; this->v[3] = tp[3]; this->v[4] = tp[4]; this->v[5] = tp[5]; } // Construct from array
		CPU_AND_GPU Vector6(const T v0, const T v1, const T v2, const T v3, const T v4, const T v5) { this->v[0] = v0; this->v[1] = v1; this->v[2] = v2; this->v[3] = v3; this->v[4] = v4; this->v[5] = v5; } // Construct from explicit values
		CPU_AND_GPU explicit Vector6(const Vector4_<T> &u, T v0, T v1) { this->v[0] = u.x; this->v[1] = u.y; this->v[2] = u.z; this->v[3] = u.w; this->v[4] = v0; this->v[5] = v1; }
		CPU_AND_GPU explicit Vector6(const Vector3_<T> &u, T v0, T v1, T v2) { this->v[0] = u.x; this->v[1] = u.y; this->v[2] = u.z; this->v[3] = v0; this->v[4] = v1; this->v[5] = v2; }
		CPU_AND_GPU explicit Vector6(const Vector2_<T> &u, T v0, T v1, T v2, T v3) { this->v[0] = u.x; this->v[1] = u.y; this->v[2] = v0; this->v[3] = v1; this->v[4] = v2, this->v[5] = v3; }

		CPU_AND_GPU inline Vector6<int> toIntRound() const {
			return Vector6<int>((int)ROUND(this[0]), (int)ROUND(this[1]), (int)ROUND(this[2]), (int)ROUND(this[3]), (int)ROUND(this[4]), (int)ROUND(this[5]));
		}

		CPU_AND_GPU inline Vector6<unsigned char> toUChar() const {
			Vector6<int> vi = toIntRound(); return Vector6<unsigned char>((unsigned char)CLAMP(vi[0], 0, 255), (unsigned char)CLAMP(vi[1], 0, 255), (unsigned char)CLAMP(vi[2], 0, 255), (unsigned char)CLAMP(vi[3], 0, 255), (unsigned char)CLAMP(vi[4], 0, 255), (unsigned char)CLAMP(vi[5], 0, 255));
		}

		CPU_AND_GPU inline Vector6<float> toFloat() const {
			return Vector6<float>((float)this[0], (float)this[1], (float)this[2], (float)this[3], (float)this[4], (float)this[5]);
		}

		CPU_AND_GPU const T *getValues() const { return this->v; }
		CPU_AND_GPU Vector6<T> &setValues(const T *rhs) { this[0] = rhs[0]; this[1] = rhs[1]; this[2] = rhs[2]; this[3] = rhs[3]; this[4] = rhs[4]; this[5] = rhs[5]; return *this; }

		// indexing operators
		CPU_AND_GPU T &operator [](int i) { return this->v[i]; }
		CPU_AND_GPU const T &operator [](int i) const { return this->v[i]; }

		// type-cast operators
		CPU_AND_GPU operator T *() { return this->v; }
		CPU_AND_GPU operator const T *() const { return this->v; }

		////////////////////////////////////////////////////////
		//  Math operators
		////////////////////////////////////////////////////////

		// scalar multiply assign
		CPU_AND_GPU friend Vector6<T> &operator *= (Vector6<T> &lhs, T d) {
			lhs[0] *= d; lhs[1] *= d; lhs[2] *= d; lhs[3] *= d; lhs[4] *= d; lhs[5] *= d; return lhs;
		}

		// component-wise vector multiply assign
		CPU_AND_GPU friend Vector6<T> &operator *= (Vector6<T> &lhs, const Vector6<T> &rhs) {
			lhs[0] *= rhs[0]; lhs[1] *= rhs[1]; lhs[2] *= rhs[2]; lhs[3] *= rhs[3]; lhs[4] *= rhs[4]; lhs[5] *= rhs[5]; return lhs;
		}

		// scalar divide assign
		CPU_AND_GPU friend Vector6<T> &operator /= (Vector6<T> &lhs, T d){
			lhs[0] /= d; lhs[1] /= d; lhs[2] /= d; lhs[3] /= d; lhs[4] /= d; lhs[5] /= d; return lhs;
		}

		// component-wise vector divide assign
		CPU_AND_GPU friend Vector6<T> &operator /= (Vector6<T> &lhs, const Vector6<T> &rhs) {
			lhs[0] /= rhs[0]; lhs[1] /= rhs[1]; lhs[2] /= rhs[2]; lhs[3] /= rhs[3]; lhs[4] /= rhs[4]; lhs[5] /= rhs[5]; return lhs;
		}

		// component-wise vector add assign
		CPU_AND_GPU friend Vector6<T> &operator += (Vector6<T> &lhs, const Vector6<T> &rhs)	{
			lhs[0] += rhs[0]; lhs[1] += rhs[1]; lhs[2] += rhs[2]; lhs[3] += rhs[3]; lhs[4] += rhs[4]; lhs[5] += rhs[5]; return lhs;
		}

		// component-wise vector subtract assign
		CPU_AND_GPU friend Vector6<T> &operator -= (Vector6<T> &lhs, const Vector6<T> &rhs)	{
			lhs[0] -= rhs[0]; lhs[1] -= rhs[1]; lhs[2] -= rhs[2]; lhs[3] -= rhs[3]; lhs[4] -= rhs[4]; lhs[5] -= rhs[5];  return lhs;
		}

		// unary negate
		CPU_AND_GPU friend Vector6<T> operator - (const Vector6<T> &rhs)	{
			Vector6<T> rv; rv[0] = -rhs[0]; rv[1] = -rhs[1]; rv[2] = -rhs[2]; rv[3] = -rhs[3]; rv[4] = -rhs[4]; rv[5] = -rhs[5];  return rv;
		}

		// vector add
		CPU_AND_GPU friend Vector6<T> operator + (const Vector6<T> &lhs, const Vector6<T> &rhs) {
			Vector6<T> rv(lhs); return rv += rhs;
		}

		// vector subtract
		CPU_AND_GPU friend Vector6<T> operator - (const Vector6<T> &lhs, const Vector6<T> &rhs) {
			Vector6<T> rv(lhs); return rv -= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector6<T> operator * (const Vector6<T> &lhs, T rhs) {
			Vector6<T> rv(lhs); return rv *= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend Vector6<T> operator * (T lhs, const Vector6<T> &rhs) {
			Vector6<T> rv(lhs); return rv *= rhs;
		}

		// vector component-wise multiply
		CPU_AND_GPU friend Vector6<T> operator * (const Vector6<T> &lhs, const Vector6<T> &rhs) {
			Vector6<T> rv(lhs); return rv *= rhs;
		}

		// scalar divide
		CPU_AND_GPU friend Vector6<T> operator / (const Vector6<T> &lhs, T rhs) {
			Vector6<T> rv(lhs); return rv /= rhs;
		}

		// vector component-wise divide
		CPU_AND_GPU friend Vector6<T> operator / (const Vector6<T> &lhs, const Vector6<T> &rhs) {
			Vector6<T> rv(lhs); return rv /= rhs;
		}

		////////////////////////////////////////////////////////
		//  Comparison operators
		////////////////////////////////////////////////////////

		// equality
		CPU_AND_GPU friend bool operator == (const Vector6<T> &lhs, const Vector6<T> &rhs) {
			return (lhs[0] == rhs[0]) && (lhs[1] == rhs[1]) && (lhs[2] == rhs[2]) && (lhs[3] == rhs[3]) && (lhs[4] == rhs[4]) && (lhs[5] == rhs[5]);
		}

		// inequality
		CPU_AND_GPU friend bool operator != (const Vector6<T> &lhs, const Vector6<T> &rhs) {
			return (lhs[0] != rhs[0]) || (lhs[1] != rhs[1]) || (lhs[2] != rhs[2]) || (lhs[3] != rhs[3]) || (lhs[4] != rhs[4]) || (lhs[5] != rhs[5]);
		}

		friend std::ostream& operator<<(std::ostream& os, const Vector6<T>& dt){
			os << dt[0] << ", " << dt[1] << ", " << dt[2] << ", " << dt[3] << ", " << dt[4] << ", " << dt[5];
			return os;
		}
	};


	template <class T, int s> class VectorX : public VectorX_ < T, s >
	{
	public:
		typedef T value_type;
		CPU_AND_GPU inline int size() const { return this->vsize; }

		////////////////////////////////////////////////////////
		//  Constructors
		////////////////////////////////////////////////////////

		CPU_AND_GPU VectorX() { this->vsize = s; } // Default constructor
		CPU_AND_GPU VectorX(const T &t) { for (int i = 0; i < s; i++) this->v[i] = t; } //Scalar constructor
		CPU_AND_GPU VectorX(const T *tp) { for (int i = 0; i < s; i++) this->v[i] = tp[i]; } // Construct from array


        CPU_AND_GPU static inline VectorX<T, s> make_zeros() {
            VectorX<T, s> x;
            x.setZeros();
            return x;
        }

        // indexing operators
        CPU_AND_GPU T &operator [](int i) { return this->v[i]; }
        CPU_AND_GPU const T &operator [](int i) const { return this->v[i]; }

		CPU_AND_GPU inline VectorX<int, s> toIntRound() const {
			VectorX<int, s> retv;
			for (int i = 0; i < s; i++) retv[i] = (int)ROUND(this->v[i]);
			return retv;
		}

		CPU_AND_GPU inline VectorX<unsigned char, s> toUChar() const {
			VectorX<int, s> vi = toIntRound();
			VectorX<unsigned char, s> retv;
			for (int i = 0; i < s; i++) retv[i] = (unsigned char)CLAMP(vi[0], 0, 255);
			return retv;
		}

		CPU_AND_GPU inline VectorX<float, s> toFloat() const {
			VectorX<float, s> retv;
			for (int i = 0; i < s; i++) retv[i] = (float) this->v[i];
			return retv;
		}

		CPU_AND_GPU const T *getValues() const { return this->v; }
		CPU_AND_GPU VectorX<T, s> &setValues(const T *rhs) { for (int i = 0; i < s; i++) this->v[i] = rhs[i]; return *this; }
		CPU_AND_GPU void Clear(T v){
			for (int i = 0; i < s; i++)
				this->v[i] = v;
		}


        CPU_AND_GPU void setZeros(){
            Clear(0);
        }

		// type-cast operators
		CPU_AND_GPU operator T *() { return this->v; }
		CPU_AND_GPU operator const T *() const { return this->v; }

		////////////////////////////////////////////////////////
		//  Math operators
		////////////////////////////////////////////////////////

		// scalar multiply assign
		CPU_AND_GPU friend VectorX<T, s> &operator *= (VectorX<T, s> &lhs, T d) {
			for (int i = 0; i < s; i++) lhs[i] *= d; return lhs;
		}

		// component-wise vector multiply assign
		CPU_AND_GPU friend VectorX<T, s> &operator *= (VectorX<T, s> &lhs, const VectorX<T, s> &rhs) {
			for (int i = 0; i < s; i++) lhs[i] *= rhs[i]; return lhs;
		}

		// scalar divide assign
		CPU_AND_GPU friend VectorX<T, s> &operator /= (VectorX<T, s> &lhs, T d){
			for (int i = 0; i < s; i++) lhs[i] /= d; return lhs;
		}

		// component-wise vector divide assign
		CPU_AND_GPU friend VectorX<T, s> &operator /= (VectorX<T, s> &lhs, const VectorX<T, s> &rhs) {
			for (int i = 0; i < s; i++) lhs[i] /= rhs[i]; return lhs;
		}

		// component-wise vector add assign
		CPU_AND_GPU friend VectorX<T, s> &operator += (VectorX<T, s> &lhs, const VectorX<T, s> &rhs)	{
			for (int i = 0; i < s; i++) lhs[i] += rhs[i]; return lhs;
		}

		// component-wise vector subtract assign
		CPU_AND_GPU friend VectorX<T, s> &operator -= (VectorX<T, s> &lhs, const VectorX<T, s> &rhs)	{
			for (int i = 0; i < s; i++) lhs[i] -= rhs[i]; return lhs;
		}

		// unary negate
		CPU_AND_GPU friend VectorX<T, s> operator - (const VectorX<T, s> &rhs)	{
			VectorX<T, s> rv; for (int i = 0; i < s; i++) rv[i] = -rhs[i]; return rv;
		}

		// vector add
		CPU_AND_GPU friend VectorX<T, s> operator + (const VectorX<T, s> &lhs, const VectorX<T, s> &rhs) {
			VectorX<T, s> rv(lhs); return rv += rhs;
		}

		// vector subtract
		CPU_AND_GPU friend VectorX<T, s> operator - (const VectorX<T, s> &lhs, const VectorX<T, s> &rhs) {
			VectorX<T, s> rv(lhs); return rv -= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend VectorX<T, s> operator * (const VectorX<T, s> &lhs, T rhs) {
			VectorX<T, s> rv(lhs); return rv *= rhs;
		}

		// scalar multiply
		CPU_AND_GPU friend VectorX<T, s> operator * (T lhs, const VectorX<T, s> &rhs) {
			VectorX<T, s> rv(lhs); return rv *= rhs;
		}

		// vector component-wise multiply
		CPU_AND_GPU friend VectorX<T, s> operator * (const VectorX<T, s> &lhs, const VectorX<T, s> &rhs) {
			VectorX<T, s> rv(lhs); return rv *= rhs;
		}

		// scalar divide
		CPU_AND_GPU friend VectorX<T, s> operator / (const VectorX<T, s> &lhs, T rhs) {
			VectorX<T, s> rv(lhs); return rv /= rhs;
		}

		// vector component-wise divide
		CPU_AND_GPU friend VectorX<T, s> operator / (const VectorX<T, s> &lhs, const VectorX<T, s> &rhs) {
			VectorX<T, s> rv(lhs); return rv /= rhs;
		}

		////////////////////////////////////////////////////////
		//  Comparison operators
		////////////////////////////////////////////////////////

		// equality
		CPU_AND_GPU friend bool operator == (const VectorX<T, s> &lhs, const VectorX<T, s> &rhs) {
			for (int i = 0; i < s; i++) if (lhs[i] != rhs[i]) return false;
			return true;
		}

		// inequality
		CPU_AND_GPU friend bool operator != (const VectorX<T, s> &lhs, const Vector6<T> &rhs) {
			for (int i = 0; i < s; i++) if (lhs[i] != rhs[i]) return true;
			return false;
		}

		friend std::ostream& operator<<(std::ostream& os, const VectorX<T, s>& dt){
			for (int i = 0; i < s; i++) os << dt[i] << "\n";
			return os;
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	// Generic vector operations
	////////////////////////////////////////////////////////////////////////////////

	template< class T> CPU_AND_GPU inline T sqr(const T &v) { return v*v; }

	// compute the dot product of two vectors
	template<class T> CPU_AND_GPU inline typename T::value_type dot(const T &lhs, const T &rhs) {
		typename T::value_type r = 0;
		for (int i = 0; i < lhs.size(); i++)
			r += lhs[i] * rhs[i];
		return r;
	}

    // return the squared length of the provided vector, i.e. the dot product with itself
    template< class T> CPU_AND_GPU inline typename T::value_type length2(const T &vec) {
        return dot(vec, vec);
    }

	// return the length of the provided vector
	template< class T> CPU_AND_GPU inline typename T::value_type length(const T &vec) {
        return sqrt(length2(vec));
	}

	// return the normalized version of the vector
	template< class T> CPU_AND_GPU inline T normalize(const T &vec)	{
		typename T::value_type sum = length(vec);
		return sum == 0 ? T(typename T::value_type(0)) : vec / sum;
	}

	//template< class T> CPU_AND_GPU inline T min(const T &lhs, const T &rhs) {
	//	return lhs <= rhs ? lhs : rhs;
	//}

	//template< class T> CPU_AND_GPU inline T max(const T &lhs, const T &rhs) {
	//	return lhs >= rhs ? lhs : rhs;
	//}

	//component wise min
	template< class T> CPU_AND_GPU inline T minV(const T &lhs, const T &rhs) {
		T rv;
		for (int i = 0; i < lhs.size(); i++)
			rv[i] = min(lhs[i], rhs[i]);
		return rv;
	}

	// component wise max
	template< class T>
	CPU_AND_GPU inline T maxV(const T &lhs, const T &rhs)	{
		T rv;
		for (int i = 0; i < lhs.size(); i++)
			rv[i] = max(lhs[i], rhs[i]);
		return rv;
	}


    // cross product
    template< class T>
    CPU_AND_GPU Vector3<T> cross(const Vector3<T> &lhs, const Vector3<T> &rhs) {
        Vector3<T> r;
        r.x = lhs.y * rhs.z - lhs.z * rhs.y;
        r.y = lhs.z * rhs.x - lhs.x * rhs.z;
        r.z = lhs.x * rhs.y - lhs.y * rhs.x;
        return r;
    }
};
