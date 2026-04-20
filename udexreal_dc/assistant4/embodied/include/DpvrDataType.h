/************************************************************************************
Filename    :
Content     :
Created     :   April , 2024
Authors     :   Ziyi Xu
Copyright   :   Copyright 2024 DPVR. All Rights reserved.
************************************************************************************/
#ifndef __DPVR_DATA_TYPE__
#define __DPVR_DATA_TYPE__

typedef uint32_t dpvr_uint32;

struct dpvr_Vector3
{
	float x, y, z;
};

struct dpvr_double3
{
	double x, y, z;
};

struct dpvr_Quaternion
{
	float w, x, y, z;
};

struct dpvr_bool3
{
	bool x, y, z;
};

#endif // !defined(__DPVR_DATA_TYPE__)
