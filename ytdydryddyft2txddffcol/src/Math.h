#pragma once
#include <d3d9.h>
#include <math.h>
#include "dffapi\RwTypes.h"

#ifndef FLT_EPSILON
#define FLT_EPSILON 1.19209290E-07F
#endif

struct Vector2
{
	float x, y;

	Vector2(float a, float b);

	Vector2();

	bool operator==(const Vector2& v) const;
};

struct Vector4
{
	 float x, y, z, w;

	 Vector4(float a, float b, float c, float d);
	 
	 Vector4();
	 
	 Vector4& operator*=(float v)
	 {
		 x *= v;
		 y *= v;
		 z *= v;
		 return *this;
	 }
};

struct Vector3
{
	float x, y, z;

	Vector3();

	Vector3(float a, float b, float c);

	Vector3(Vector4& v4)
	{
		x = v4.x; y = v4.y; z = v4.z;
	}

	float DistanceTo(Vector3 &vec);

	bool operator==(const Vector3& v) const;

	Vector3 operator-(const Vector3&v) const
    {
		return Vector3(x - v.x, y - v.y, z - v.z);
    }

	Vector3 operator/(float v) const
    {
		return Vector3(x / v, y / v, z / v);
    }

	Vector3& operator/=(float v)
    {
		x /= v;
		y /= v;
		z /= v;
		return *this;
    }

	Vector3& operator*=(float v)
    {
		x *= v;
		y *= v;
		z *= v;
		return *this;
    }

	void TransformPosition(Vector3 *v, D3DMATRIX *m);
	void TransformNormal(Vector3 *v, D3DMATRIX *m);

	void Normalise()
	{
		float len = x * x + y * y + z * z;
		if(len > 0.0)
		{
			float recLen = 1.0 / sqrt(len);
			x *= recLen;
			y *= recLen;
			z *= recLen;
		}
		else
		{
			x = 0.0f;
			y = 0.0f;
			z = 1.0f;
		}
	}

	static float Dot(Vector3 *v1, Vector3 *v2)
	{
		return v1->z * v2->z + v1->y * v2->y + v1->x * v2->x;
	}
};

struct Triangle
{
	unsigned short a, b, d, c;
};

struct BoundSphere
{
	Vector3 center;
	float radius;

	BoundSphere();

	BoundSphere(float x, float y, float z, float fRadius);

	BoundSphere(Vector3 &vCenter, float fRadius);

	BoundSphere(Vector3 *vCenter, float fRadius);
};

struct BoundBox
{
	Vector3 aabbMin;
	Vector3 aabbMax;

	BoundBox();

	void Set(Vector3 &min, Vector3 &max);

	BoundBox(Vector3 &min, Vector3 &max);

	BoundBox(Vector3 *min, Vector3 *max);

	void ToSphere(BoundSphere *sphere);
};

void MatrixMul(struct gtaRwMatrix *a, struct gtaRwMatrix *b, struct gtaRwMatrix *c);
void MatrixIdentity(struct gtaRwMatrix *m);

Vector2::Vector2(float a, float b)
{
	x = a; y = b;
}

Vector2::Vector2()
{
	x = y = 0.0f;
}

bool Vector2::operator==(const Vector2& v) const
{
    return ( ( fabs ( x - v.x ) < FLT_EPSILON ) &&
             ( fabs ( y - v.y ) < FLT_EPSILON ) );
}

Vector3::Vector3()
{
}

Vector3::Vector3(float a, float b, float c)
{
	x = a; y = b; z = c;
}

float Vector3::DistanceTo(Vector3 &vec)
{
	return sqrtf(powf(vec.x - x, 2.0f) + powf(vec.y - y, 2.0f) + powf(vec.z - z, 2.0f));
}

bool Vector3::operator==(const Vector3& v) const
{
    return ( ( fabs ( x - v.x ) < FLT_EPSILON ) &&
             ( fabs ( y - v.y ) < FLT_EPSILON ) &&
             ( fabs ( z - v.z ) < FLT_EPSILON ) );
}

void Vector3::TransformPosition(Vector3 *v, D3DMATRIX *m)
{
	x = m->_31 * v->z + m->_21 * v->y + m->_11 * v->x + m->_41;
	y = m->_32 * v->z + m->_12 * v->x + m->_22 * v->y + m->_42;
	z = m->_33 * v->z + m->_13 * v->x + m->_23 * v->y + m->_43;
}

void Vector3::TransformNormal(Vector3 *v, D3DMATRIX *m)
{
	x = m->_31 * v->z + m->_21 * v->y + m->_11 * v->x;
	y = m->_32 * v->z + m->_12 * v->x + m->_22 * v->y;
	z = m->_33 * v->z + m->_13 * v->x + m->_23 * v->y;
}

Vector4::Vector4(float a, float b, float c, float d)
{
	x = a; y = b; z = c; w = d;
}

Vector4::Vector4()
{
}

BoundSphere::BoundSphere()
{
	center.x = center.y = center.z = 0.0f;
	radius = 1.0f;
}

BoundSphere::BoundSphere(float x, float y, float z, float fRadius)
{
	center.x = x;
	center.y = y;
	center.z = z;
	radius = fRadius;
}

BoundSphere::BoundSphere(Vector3 &vCenter, float fRadius)
{
	center = vCenter;
	radius = fRadius;
}

BoundSphere::BoundSphere(Vector3 *vCenter, float fRadius)
{
	center = *vCenter;
	radius = fRadius;
}

BoundBox::BoundBox()
{
	aabbMin.x = aabbMin.y = aabbMin.z = -1.0f;
	aabbMax.x = aabbMax.y = aabbMax.z = 1.0f;
}

void BoundBox::Set(Vector3 &min, Vector3 &max)
{
	aabbMin = min;
	aabbMax = max;
}

BoundBox::BoundBox(Vector3 &min, Vector3 &max)
{
	Set(min, max);
}

BoundBox::BoundBox(Vector3 *min, Vector3 *max)
{
	Set(*min, *max);
}

void BoundBox::ToSphere(BoundSphere *sphere) // BoundBox::ToSphere
{
	Vector3 distances = aabbMax - aabbMin;
	float a, b;
	if(distances.x > distances.y)
	{
		a = distances.x;
		if(distances.y > distances.z)
			b = distances.y;
		else
			b = distances.z;
	}
	else
	{
		a = distances.y;
		if(distances.x > distances.z)
			b = distances.x;
		else
			b = distances.z;
	}
	sphere->radius = sqrtf(a * a + b * b) / 2.0f;
	sphere->center.x = (aabbMin.x + aabbMax.x) / 2.0f;
	sphere->center.y = (aabbMin.y + aabbMax.y) / 2.0f;
	sphere->center.z = (aabbMin.z + aabbMax.z) / 2.0f;
}

void MatrixMul(gtaRwMatrix *a, gtaRwMatrix *b, gtaRwMatrix *c)
{
	gtaRwMatrix m;
	m.right.x = b->up.x * c->right.y + b->right.x * c->right.x + c->right.z * b->at.x;
	m.right.y = b->at.y * c->right.z + b->right.y * c->right.x + b->up.y * c->right.y;
	m.right.z = b->up.z * c->right.y + b->at.z * c->right.z + c->right.x * b->right.z;
	m.up.x = c->up.z * b->at.x + b->up.x * c->up.y + c->up.x * b->right.x;
	m.up.y = b->at.y * c->up.z + b->up.y * c->up.y + b->right.y * c->up.x;
	m.up.z = c->up.x * b->right.z + b->at.z * c->up.z + b->up.z * c->up.y;
	m.at.x = b->up.x * c->at.y + c->at.z * b->at.x + c->at.x * b->right.x;
	m.at.y = b->at.y * c->at.z + b->up.y * c->at.y + c->at.x * b->right.y;
	m.at.z = c->at.x * b->right.z + b->at.z * c->at.z + b->up.z * c->at.y;
	m.pos.x = b->up.x * c->pos.y + c->pos.z * b->at.x + c->pos.x * b->right.x + b->pos.x;
	m.pos.y = c->pos.z * b->at.y + c->pos.y * b->up.y + c->pos.x * b->right.y + b->pos.y;
	m.pos.z = c->pos.x * b->right.z + b->up.z * c->pos.y + b->at.z * c->pos.z + b->pos.z;
	memcpy(a, &m, 64);
}

void MatrixIdentity(gtaRwMatrix *m)
{
	memset(m, 0, sizeof(gtaRwMatrix));
	m->right.x = 1.0f;
	m->up.y = 1.0f;
	m->at.z = 1.0f;
}

float Saturate(float a)
{
	if(a < 0.0f)
		return 0.0f;
	if(a > 1.0f)
		return 1.0f;
	return a;
}