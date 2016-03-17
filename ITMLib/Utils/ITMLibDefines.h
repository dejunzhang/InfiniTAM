// Copyright 2014-2015 Isis Innovation Limited and the authors of InfiniTAM

#pragma once



#include <assert.h>
#include <cuda_runtime.h>

#ifndef ITMSafeCall
#define ITMSafeCall ORcudaSafeCall
#endif

#include "../../ORUtils/PlatformIndependence.h"
#include "ITMMath.h"


//////////////////////////////////////////////////////////////////////////
// Voxel Hashing definition and helper functions
//////////////////////////////////////////////////////////////////////////

#define SDF_BLOCK_SIZE 8
#define SDF_BLOCK_SIZE3 (SDF_BLOCK_SIZE * SDF_BLOCK_SIZE * SDF_BLOCK_SIZE)
#define SDF_LOCAL_BLOCK_NUM 0x40000		// Number of locally stored blocks

#define SDF_BUCKET_NUM 0x100000			// Number of Hash Bucket, must be 2^n and bigger than SDF_LOCAL_BLOCK_NUM, SDF_HASH_MASK = SDF_BUCKET_NUM - 1
#define SDF_HASH_MASK (SDF_BUCKET_NUM-1)// Used for get hashing value of the bucket index, "x & (uint)SDF_HASH_MASK" is the same as "x % SDF_BUCKET_NUM"
#define SDF_EXCESS_LIST_SIZE 0x20000	// Size of excess list, used to handle collisions. Also max offset (unsigned short) value.

#define SDF_GLOBAL_BLOCK_NUM (SDF_BUCKET_NUM+SDF_EXCESS_LIST_SIZE)	// Number of globally stored blocks == size of ordered + unordered part of hash table


//////////////////////////////////////////////////////////////////////////
// Voxel Hashing data structures
//////////////////////////////////////////////////////////////////////////

/** 
    A single entry in the hash table (hash table bucket).
*/
struct ITMHashEntry
{
	/** Position of the corner of the 8x8x8 volume, that identifies the entry. 
    In voxel-block coordinates. Multiply by SDF_BLOCK_SIZE to get voxel coordinates,
    and then by ITMSceneParams::voxelSize to get world coordinates.
    */
	Vector3s pos;
	/** 1-based position of the 'next'
    entry in the excess list. 
    Used as SDF_BUCKET_NUM + hashEntry.offset - 1
    to compute the next hashIdx.
    This value is at most SDF_EXCESS_LIST_SIZE.*/
	int offset;
    _CPU_AND_GPU_CODE_ bool hasExcessListOffset() { return offset >= 1; }
	_CPU_AND_GPU_CODE_ int getHashIndexOfNextExcessEntry() {
        return SDF_BUCKET_NUM + offset - 1;
    }
    
    /** Pointer to the voxel block array (TVoxel* voxelData).
	    - >= 0 identifies an actual allocated entry in the voxel block array
	    - -1 identifies an entry that has been removed (swapped out)
	    - <-1 identifies an unallocated block

        Multiply with SDF_BLOCK_SIZE3 and add offset to access individual TVoxels.

        "store a pointer to the location in a large vloxel block array, where the T-SDF data of all the
        blocks is serially stored"
	*/
    int ptr;

    _CPU_AND_GPU_CODE_ bool isAllocatedAndActive() { return ptr >= 0; }

    /// Was once allocated, but is maybe not in active memory.
    /// But this space is permanently reserved.
    _CPU_AND_GPU_CODE_ bool isAllocated() { return ptr >= -1; }

    _CPU_AND_GPU_CODE_ bool isUnallocated() { return ptr < -1; }

    // an unallocated entry, used for resetting
    static ITMHashEntry createIllegalEntry() {
        ITMHashEntry tmpEntry;
        memset(&tmpEntry, 0, sizeof(ITMHashEntry));
        tmpEntry.ptr = -2;
        return tmpEntry;
    }
};

#include "../Objects/ITMVoxelBlockHash.h"

/** \brief
    Stores the information of a single voxel in the volume
*/
struct ITMVoxel_f_rgb
{
	_CPU_AND_GPU_CODE_ static float SDF_initialValue() { return 1.0f; }
	_CPU_AND_GPU_CODE_ static float SDF_valueToFloat(float x) { return x; }
	_CPU_AND_GPU_CODE_ static float SDF_floatToValue(float x) { return x; }

	static const CONSTPTR(bool) hasColorInformation = true;

	/** Value of the truncated signed distance transformation. */
	float sdf;
	/** Number of fused observations that make up @p sdf. */
	uchar w_depth;
	/** RGB colour information stored for this voxel. */
	Vector3u clr;
	/** Number of observations that made up @p clr. */
	uchar w_color;

	_CPU_AND_GPU_CODE_ ITMVoxel_f_rgb()
	{
		sdf = SDF_initialValue();
		w_depth = 0;
		clr = (uchar)0;
		w_color = 0;
	}
};

/** \brief
    Stores the information of a single voxel in the volume
*/
struct ITMVoxel_s_rgb
{
	_CPU_AND_GPU_CODE_ static short SDF_initialValue() { return 32767; }
	_CPU_AND_GPU_CODE_ static float SDF_valueToFloat(float x) { return (float)(x) / 32767.0f; }
	_CPU_AND_GPU_CODE_ static short SDF_floatToValue(float x) { return (short)((x) * 32767.0f); }

	static const CONSTPTR(bool) hasColorInformation = true;

	/** Value of the truncated signed distance transformation. */
	short sdf;
	/** Number of fused observations that make up @p sdf. */
	uchar w_depth;
	/** Padding that may or may not improve performance on certain GPUs */
	//uchar pad;
	/** RGB colour information stored for this voxel. */
	Vector3u clr;
	/** Number of observations that made up @p clr. */
	uchar w_color;

	_CPU_AND_GPU_CODE_ ITMVoxel_s_rgb()
	{
		sdf = SDF_initialValue();
		w_depth = 0;
		clr = (uchar)0;
		w_color = 0;
	}
};

struct ITMVoxel_s
{
	_CPU_AND_GPU_CODE_ static short SDF_initialValue() { return 32767; }
	_CPU_AND_GPU_CODE_ static float SDF_valueToFloat(float x) { return (float)(x) / 32767.0f; }
	_CPU_AND_GPU_CODE_ static short SDF_floatToValue(float x) { return (short)((x) * 32767.0f); }

	static const CONSTPTR(bool) hasColorInformation = false;

	/** Value of the truncated signed distance transformation. */
	short sdf;
	/** Number of fused observations that make up @p sdf. */
	uchar w_depth;
	/** Padding that may or may not improve performance on certain GPUs */
	//uchar pad;

	_CPU_AND_GPU_CODE_ ITMVoxel_s()
	{
		sdf = SDF_initialValue();
		w_depth = 0;
	}
};

struct ITMVoxel_f
{
	_CPU_AND_GPU_CODE_ static float SDF_initialValue() { return 1.0f; }
	_CPU_AND_GPU_CODE_ static float SDF_valueToFloat(float x) { return x; }
	_CPU_AND_GPU_CODE_ static float SDF_floatToValue(float x) { return x; }

	static const CONSTPTR(bool) hasColorInformation = false;

	/** Value of the truncated signed distance transformation. */
	float sdf;
	/** Number of fused observations that make up @p sdf. */
	uchar w_depth;
	/** Padding that may or may not improve performance on certain GPUs */
	//uchar pad;

	_CPU_AND_GPU_CODE_ ITMVoxel_f()
	{
		sdf = SDF_initialValue();
		w_depth = 0;
	}
};

/** This chooses the information stored at each voxel. At the moment, valid
    options are ITMVoxel_s, ITMVoxel_f, ITMVoxel_s_rgb and ITMVoxel_f_rgb 
*/
#define VOXELTYPE ITMVoxel_s_rgb // ITMVoxel_s
#define _str(s) #s // note that s will not be macro expanded prior to stringification
#define _xstr(s) _str(s) // force evaluation of s prior to 'calling' str
#define sVOXELTYPE _xstr(VOXELTYPE) 
struct ITMVoxel : public VOXELTYPE {
    typedef struct { VOXELTYPE blockVoxels[SDF_BLOCK_SIZE3]; }  VoxelBlock;
};
/** This chooses the way the voxels are addressed and indexed.
*/
typedef ITMLib::Objects::ITMVoxelBlockHash ITMVoxelIndex;



/// The tracker iteration type used to define the tracking iteration regime
enum TrackerIterationType
{
    /// Update only the current rotation estimate. This is preferable for the coarse solution stages.
    TRACKER_ITERATION_ROTATION = 1,
    TRACKER_ITERATION_TRANSLATION = 2,
    TRACKER_ITERATION_BOTH = 3,
    TRACKER_ITERATION_NONE = 4
};

#include "../../ORUtils/Image.h"

#define ITMFloatImage ORUtils::Image<float>
#define ITMFloat2Image ORUtils::Image<Vector2f>
#define ITMFloat4Image ORUtils::Image<Vector4f>
#define ITMShortImage ORUtils::Image<short>
#define ITMShort3Image ORUtils::Image<Vector3s>
#define ITMShort4Image ORUtils::Image<Vector4s>
#define ITMUShortImage ORUtils::Image<ushort>
#define ITMUIntImage ORUtils::Image<uint>
#define ITMIntImage ORUtils::Image<int>
#define ITMUCharImage ORUtils::Image<uchar>
#define ITMUChar4Image ORUtils::Image<Vector4u>
#define ITMBoolImage ORUtils::Image<bool>

#include "ITMLibSettings.h" // must be included after tracker iteration type is defined