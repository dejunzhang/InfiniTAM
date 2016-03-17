// Copyright 2014-2015 Isis Innovation Limited and the authors of InfiniTAM

#include <math.h>
#include "ITMPose.h"

#include <stdio.h>

using namespace ITMLib::Objects;

ITMPose::ITMPose(void) { this->SetFrom(0, 0, 0, 0, 0, 0); }

ITMPose::ITMPose(float tx, float ty, float tz, float rx, float ry, float rz) 
{ this->SetFrom(tx, ty, tz, rx, ry, rz); }
ITMPose::ITMPose(const float pose[6]) { this->SetFrom(pose); }
ITMPose::ITMPose(const Matrix4f & src) { this->SetM(src); }
ITMPose::ITMPose(const Vector6f & tangent) { this->SetFrom(tangent); }
ITMPose::ITMPose(const ITMPose & src) { this->SetFrom(&src); }

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.707106781186547524401
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.5707963267948966192E0
#endif

void ITMPose::SetFrom(float tx, float ty, float tz, float rx, float ry, float rz)
{
	this->params.each.tx = tx;
	this->params.each.ty = ty;
	this->params.each.tz = tz;
	this->params.each.rx = rx;
	this->params.each.ry = ry;
	this->params.each.rz = rz;

	this->SetModelViewFromParams();
}

void ITMPose::SetFrom(const Vector3f &translation, const Vector3f &rotation)
{
	this->params.each.tx = translation.x;
	this->params.each.ty = translation.y;
	this->params.each.tz = translation.z;
	this->params.each.rx = rotation.x;
	this->params.each.ry = rotation.y;
	this->params.each.rz = rotation.z;

	this->SetModelViewFromParams();
}

void ITMPose::SetFrom(const Vector6f &tangent)
{
	this->params.each.tx = tangent[0];
	this->params.each.ty = tangent[1];
	this->params.each.tz = tangent[2];
	this->params.each.rx = tangent[3];
	this->params.each.ry = tangent[4];
	this->params.each.rz = tangent[5];

	this->SetModelViewFromParams();
}

void ITMPose::SetFrom(const float pose[6])
{
	SetFrom(pose[0], pose[1], pose[2], pose[3], pose[4], pose[5]);
}

void ITMPose::SetFrom(const ITMPose *pose)
{
	this->params.each.tx = pose->params.each.tx;
	this->params.each.ty = pose->params.each.ty;
	this->params.each.tz = pose->params.each.tz;
	this->params.each.rx = pose->params.each.rx;
	this->params.each.ry = pose->params.each.ry;
	this->params.each.rz = pose->params.each.rz;

	M = pose->M;
}

void ITMPose::SetModelViewFromParams()
{
	// w is an "Euler vector", i.e. the vector axis of rotation * theta
	const Vector3f w = params.r;
    const float theta_sq = dot(w,w), theta = sqrt(theta_sq);
	const float inv_theta = 1.0f / theta;

	const Vector3f t = params.t;

    float A, B, C;
	/*
	Limit for t approximating theta

	A = lim_{t -> theta} Sin[t]/t
	B = lim_{t -> theta} (1 - Cos[t])/t^2
	C = lim_{t -> theta} (1 - A)/t^2
	*/
    if (theta_sq < 1e-6f) // dont divide by very small theta - use taylor series expansion of involved functions instead
    {
        A = 1     - theta_sq / 6 + theta_sq*theta_sq / 120; // Series[a, {t, 0, 4}]
        B = 1/2.f - theta_sq / 24;  //  Series[b, {t, 0, 2}]
		C = 1/6.f - theta_sq / 120; // Series[c, {t, 0, 2}]
    }
    else {
        A = sinf(theta) * inv_theta;
        B = (1.0f - cosf(theta)) * (inv_theta * inv_theta);
        C = (1.0f - A) * (inv_theta * inv_theta);
    }
	const Vector3f crossV = cross(w, t);
	const Vector3f cross2 = cross(w, crossV);
	const Vector3f T = t +  B * crossV + C * cross2;
	
	// w = t u, u \in S^2
	// R = exp(w . L) = I + sin(t) (u . L) + (1 - cos(t)) (u . L)^2
	// c.f. https://en.wikipedia.org/wiki/Rotation_group_SO(3)#Exponential_map
	Matrix3f R;
#define setR(row, col) R.m[row + 3 * col]

	const float wx2 = w.x * w.x, wy2 = w.y * w.y, wz2 = w.z * w.z;
	setR(0, 0) = 1.0f - B*(wy2 + wz2);
	setR(1, 1) = 1.0f - B*(wx2 + wz2);
	setR(2, 2) = 1.0f - B*(wx2 + wy2);

	float a, b;
	a = A * w.z, b = B * (w.x * w.y);
	setR(0, 1) = b - a;
	setR(1, 0) = b + a;

	a = A * w.y, b = B * (w.x * w.z);
	setR(0, 2) = b + a;
	setR(2, 0) = b - a;

	a = A * w.x, b = B * (w.y * w.z);
	setR(1, 2) = b - a;
	setR(2, 1) = b + a;

	// Copy to M
	SetRPartOfM(R);
	M.setTranslate(T); 

	M.m[3 + 4*0] = 0.0f; M.m[3 + 4*1] = 0.0f; M.m[3 + 4*2] = 0.0f; M.m[3 + 4*3] = 1.0f;
}

void ITMPose::SetParamsFromModelView()
{
	Vector3f resultRot;
	Matrix3f R = GetR();
	Vector3f T = GetT();

	float cos_angle = (R.m00  + R.m11 + R.m22 - 1.0f) * 0.5f;
	resultRot.x = (R.m[2 + 3 * 1] - R.m[1 + 3 * 2]) * 0.5f;
	resultRot.y = (R.m[0 + 3 * 2] - R.m[2 + 3 * 0]) * 0.5f;
	resultRot.z = (R.m[1 + 3 * 0] - R.m[0 + 3 * 1]) * 0.5f;

	float sin_angle_abs = sqrt(dot(resultRot, resultRot));

	if (cos_angle > M_SQRT1_2)
	{
		if (sin_angle_abs) 
		{
			float p = asinf(sin_angle_abs) / sin_angle_abs;
			resultRot *= p;
		}
	}
	else
	{
		if (cos_angle > -M_SQRT1_2)
		{
			float p = acosf(cos_angle) / sin_angle_abs;
			resultRot *= p;
		}
		else
		{
			float angle = (float)M_PI - asinf(sin_angle_abs);
			float d0 = R.m[0 + 3 * 0] - cos_angle;
			float d1 = R.m[1 + 3 * 1] - cos_angle;
			float d2 = R.m[2 + 3 * 2] - cos_angle;

			Vector3f r2;

			if(fabsf(d0) > fabsf(d1) && fabsf(d0) > fabsf(d2))
			{ r2.x = d0; r2.y = (R.m[1 + 3 * 0] + R.m[0 + 3 * 1]) * 0.5f; r2.z = (R.m[0 + 3 * 2] + R.m[2 + 3 * 0]) * 0.5f; } 
			else 
			{
				if(fabsf(d1) > fabsf(d2)) 
				{ r2.x = (R.m[1 + 3 * 0] + R.m[0 + 3 * 1]) * 0.5f; r2.y = d1; r2.z = (R.m[2 + 3 * 1] + R.m[1 + 3 * 2]) * 0.5f; }
				else { r2.x = (R.m[0 + 3 * 2] + R.m[2 + 3 * 0]) * 0.5f; r2.y = (R.m[2 + 3 * 1] + R.m[1 + 3 * 2]) * 0.5f; r2.z = d2; }
			}

			if (dot(r2, resultRot) < 0.0f) { r2 *= -1.0f; }

			r2 = normalize(r2);

			resultRot = angle * r2; 
		}
	}

	float shtot = 0.5f;
	float theta = length(resultRot);

	if (theta > 0.00001f) shtot = sinf(theta * 0.5f) / theta;

	ITMPose halfrotor(0.0f, 0.0f, 0.0f, resultRot.x * -0.5f, resultRot.y * -0.5f, resultRot.z * -0.5f);

	Vector3f rottrans = halfrotor.GetR() * T;

	if (theta > 0.001f)
	{
		float denom = dot(resultRot, resultRot);
		float param = dot(T, resultRot) * (1 - 2 * shtot) / denom;
		
		rottrans -= resultRot * param;
	}
	else
	{
		float param = dot(T, resultRot) / 24;
		rottrans -= resultRot * param;
	}

	rottrans /= 2 * shtot;

	this->params.r = resultRot;
	this->params.t = rottrans;
}

ITMPose ITMPose::exp(const Vector6f& tangent)
{
	return ITMPose(tangent);
}

void ITMPose::MultiplyWith(const ITMPose *pose)
{
	M = M * pose->M;
	this->SetParamsFromModelView();
}

Matrix3f ITMPose::GetR(void) const
{
	Matrix3f R;
	R.m[0 + 3*0] = M.m[0 + 4*0]; R.m[1 + 3*0] = M.m[1 + 4*0]; R.m[2 + 3*0] = M.m[2 + 4*0];
	R.m[0 + 3*1] = M.m[0 + 4*1]; R.m[1 + 3*1] = M.m[1 + 4*1]; R.m[2 + 3*1] = M.m[2 + 4*1];
	R.m[0 + 3*2] = M.m[0 + 4*2]; R.m[1 + 3*2] = M.m[1 + 4*2]; R.m[2 + 3*2] = M.m[2 + 4*2];

	return R;
}

Vector3f ITMPose::GetT(void) const
{
	Vector3f T;
	T.v[0] = M.m[0 + 4*3]; T.v[1] = M.m[1 + 4*3]; T.v[2] = M.m[2 + 4*3];

	return T;
}

void ITMPose::GetParams(Vector3f &translation, Vector3f &rotation)
{
	translation.x = this->params.each.tx;
	translation.y = this->params.each.ty;
	translation.z = this->params.each.tz;

	rotation.x = this->params.each.rx;
	rotation.y = this->params.each.ry;
	rotation.z = this->params.each.rz;
}

void ITMPose::SetM(const Matrix4f & src)
{
	M = src;
	SetParamsFromModelView();
}

void ITMPose::SetR(const Matrix3f & R)
{
	SetRPartOfM(R);
	SetParamsFromModelView();
}

void ITMPose::SetT(const Vector3f & t)
{
	M.setTranslate(t);

	SetParamsFromModelView();
}

void ITMPose::SetRT(const Matrix3f & R, const Vector3f & t)
{
	SetRPartOfM(R);
	M.setTranslate(t);

	SetParamsFromModelView();
}

Matrix4f ITMPose::GetInvM(void) const
{
	Matrix4f ret;
	M.inv(ret);
	return ret;
}

void ITMPose::SetInvM(const Matrix4f & invM)
{
	invM.inv(M);
	SetParamsFromModelView();
}

void ITMPose::Coerce(void)
{
	SetParamsFromModelView();
	SetModelViewFromParams();
}
