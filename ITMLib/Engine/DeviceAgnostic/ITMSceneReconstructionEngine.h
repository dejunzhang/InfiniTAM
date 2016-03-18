// Copyright 2014-2015 Isis Innovation Limited and the authors of InfiniTAM

#pragma once

#include "../../Utils/ITMLibDefines.h"
#include "ITMPixelUtils.h"
#include "ITMRepresentationAccess.h"

#include "../../Objects/ITMLocalVBA.h"


/// Fusion Stage - Camera Data Integration
/// \returns \f$\eta\f$, -1 on failure
// Note that the stored T-SDF values are normalized to lie
// in [-1,1] within the truncation band.
_CPU_AND_GPU_CODE_ inline float computeUpdatedVoxelDepthInfo(
    DEVICEPTR(ITMVoxel) &voxel, //!< X
    const THREADPTR(Vector4f) & pt_model, //!< voxel location X
    const CONSTPTR(Matrix4f) & M_d, //!< depth camera pose
    const CONSTPTR(Vector4f) & projParams_d, //!< intrinsic camera parameters \f$K_d\f$
    float mu, int maxW, const CONSTPTR(float) *depth, const CONSTPTR(Vector2i) & imgSize)
{

    float depth_measure, eta, oldF, newF;
    int oldW, newW;

    // project point into depth image
    /// X_d, depth camera coordinate system
    Vector4f pt_camera;
    /// \pi(K_dX_d), projection into the depth image
    Vector2f pt_image;
    if (!projectModel(projParams_d, M_d,
        imgSize, pt_model, pt_camera, pt_image)) return -1;

    // get measured depth from image, no interpolation
    /// I_d(\pi(K_dX_d))
    depth_measure = sampleNearest(depth, pt_image, imgSize);
    if (depth_measure <= 0.0) return -1;

    /// I_d(\pi(K_dX_d)) - X_d^(z)          (3)
    eta = depth_measure - pt_camera.z;
    // check whether voxel needs updating
    if (eta < -mu) return eta;

    // compute updated SDF value and reliability (number of observations)
    /// D(X), w(X)
    oldF = ITMVoxel::SDF_valueToFloat(voxel.sdf);
    oldW = voxel.w_depth;

    // newF, normalized for -1 to 1
    newF = MIN(1.0f, eta / mu);
    newW = 1;

    updateVoxelDepthInformation(
        voxel,
        oldF, oldW, newF, newW, maxW);

    return eta;
}

/// \returns early on failure
_CPU_AND_GPU_CODE_ inline void computeUpdatedVoxelColorInfo(DEVICEPTR(ITMVoxel) &voxel, const THREADPTR(Vector4f) & pt_model, const CONSTPTR(Matrix4f) & M_rgb,
    const CONSTPTR(Vector4f) & projParams_rgb, float mu, uchar maxW, float eta, const CONSTPTR(Vector4u) *rgb, const CONSTPTR(Vector2i) & imgSize)
{
    Vector4f pt_camera; Vector2f pt_image;
    Vector3f oldC, newC;
    int newW, oldW;

    if (!projectModel(projParams_rgb, M_rgb,
        imgSize, pt_model, pt_camera, pt_image)) return;

    oldW = (float)voxel.w_color;
    oldC = TO_FLOAT3(voxel.clr);

    /// Like formula (4) for depth
    newC = TO_VECTOR3(interpolateBilinear<Vector4f>(rgb, pt_image, imgSize));
    newW = 1;

    updateVoxelColorInformation(
        voxel,
        oldC, oldW, newC, newW, maxW);
}


_CPU_AND_GPU_CODE_ static void computeUpdatedVoxelInfo(DEVICEPTR(ITMVoxel) & voxel, const THREADPTR(Vector4f) & pt_model,
    const THREADPTR(Matrix4f) & M_d, const THREADPTR(Vector4f) & projParams_d,
    const THREADPTR(Matrix4f) & M_rgb, const THREADPTR(Vector4f) & projParams_rgb,
    float mu, int maxW,
    const CONSTPTR(float) *depth, const CONSTPTR(Vector2i) & imgSize_d,
    const CONSTPTR(Vector4u) *rgb, const THREADPTR(Vector2i) & imgSize_rgb)
{
    float eta = computeUpdatedVoxelDepthInfo(voxel, pt_model, M_d, projParams_d, mu, maxW, depth, imgSize_d);

    // Only the voxels withing +- 25% mu of the surface get color
    if ((eta > mu) || (fabs(eta / mu) > 0.25f)) return;
    computeUpdatedVoxelColorInfo(voxel, pt_model, M_rgb, projParams_rgb, mu, maxW, eta, rgb, imgSize_rgb);
}

// alloc types
#define AT_NEEDS_ALLOC_FITS 1 //needs allocation, fits in the ordered list
#define AT_NEEDS_ALLOC_EXCESS 2 //needs allocation in the excess list

// visible type
//#define VT_NOT_VISIBLE 0 // default
#define VT_VISIBLE 1 //make child visible and in memory
#define VT_VISIBLE_PREVIOUS 3 // visible at previous frame

/// For allocation and visibility determination. 
///
/// Determine the blocks around a given depth sample that are currently visible
/// and need to be allocated.
/// Builds hashVisibility and entriesAllocType.
/// \param x,y [in] loop over depth image.
_CPU_AND_GPU_CODE_ inline void buildHashAllocAndVisibleTypePP(
    DEVICEPTR(uchar) *entriesAllocType, //!< [out] allocation type (AT_*) for each hash table bucket, indexed by values computed from hashIndex, or in excess part
    DEVICEPTR(uchar) *entriesVisibleType,//!< [out] visibility type (VT_*) for each hash table bucket, indexed by values computed from hashIndex, or in excess part
    int x, int y,
    DEVICEPTR(Vector4s) *blockCoords, //!< [out] blockPos coordinate of each voxel block that needs allocation, indexed by values computed from hashIndex, or in excess part
    const CONSTPTR(float) *depth,
    Matrix4f invM_d, //!< depth to world transformation
    Vector4f invProjParams_d, //!< Note: Inverse projection parameters to avoid division by fx, fy.
    float mu, 
    Vector2i imgSize,
    float oneOverVoxelBlockWorldspaceSize, //!< 1 / (voxelSize * SDF_BLOCK_SIZE)
    const CONSTPTR(ITMHashEntry) *hashTable, //<! [in] hash table buckets, indexed by values computed from hashIndex
    float viewFrustum_min, //!< znear
    float viewFrustum_max  //!< zfar
    )
{
    float depth_measure; unsigned int hashIdx; int noSteps;
    Vector4f pt_camera_f; Vector3f point_e, point, direction; Vector3s blockPos;

    // Find 3d position of depth pixel xy
    depth_measure = depth[x + y * imgSize.x];
    if (depth_measure <= 0 || (depth_measure - mu) < 0 || (depth_measure - mu) < viewFrustum_min || (depth_measure + mu) > viewFrustum_max) return;

    pt_camera_f = depthTo3DInvProjParams(invProjParams_d, x, y, depth_measure);

    // distance from camera
    float norm = length(pt_camera_f.toVector3());

    // Transform into block coordinates the found point +- mu
    // TODO why /norm? An adhoc fix to not allocate too much when far away and allocate more when nearby?
    point = TO_VECTOR3(invM_d * (pt_camera_f * (1.0f - mu / norm))) * oneOverVoxelBlockWorldspaceSize;
    point_e = TO_VECTOR3(invM_d * (pt_camera_f * (1.0f + mu / norm))) * oneOverVoxelBlockWorldspaceSize;

    // We will step along point -> point_e and add all voxel blocks we encounter to the visible list
    // "Create a segment on the line of sight in the range of the T-SDF truncation band"
    direction = point_e - point;
    norm = length(direction);
    noSteps = (int)ceil(2.0f*norm);

    direction /= (float)(noSteps - 1);

    //add neighbouring blocks
    for (int i = 0; i < noSteps; i++)
    {
        // "take the block coordinates of voxels on this line segment"
        blockPos = TO_SHORT_FLOOR3(point);

        //compute index in hash table
        hashIdx = hashIndex(blockPos);

        //check if hash table contains entry (block has already been allocated)
        bool isFound = false;

        ITMHashEntry hashEntry;

        // whether we find blockPos at the current hashIdx
#define check_found(BREAK) \
            hashEntry = hashTable[hashIdx]; \
            if (IS_EQUAL3(hashEntry.pos, blockPos) && hashEntry.isAllocated()) \
            {\
                entriesVisibleType[hashIdx] = VT_VISIBLE; \
                isFound = true; \
                BREAK;\
            }

        check_found(NULL);

        if (!isFound)
        {
            bool isExcess = false;
            if (hashEntry.isAllocated()) //seach excess list only if there is no room in ordered part
            {
                isExcess = true;
                while (hashEntry.hasExcessListOffset())
                {
                    hashIdx = hashEntry.getHashIndexOfNextExcessEntry();
                    check_found(break);
                }
            }

            if (!isFound) //still not found: needs allocation 
            {
                entriesAllocType[hashIdx] = isExcess ? AT_NEEDS_ALLOC_EXCESS : AT_NEEDS_ALLOC_FITS; //needs allocation 

                blockCoords[hashIdx] = Vector4s(blockPos.x, blockPos.y, blockPos.z, 1);
            }
        }

        point += direction;
    }
    #undef check_found
}

#include <cuda_runtime.h>
struct AllocationTempData {

    int noVisibleEntries;
};

/// \returns false when the list is full

inline
#ifdef __CUDACC__
__device__
#endif
void allocateVoxelBlock(
    int targetIdx,

    typename ITMLocalVBA::VoxelAllocationList* voxelAllocationList,
    ITMVoxelBlockHash::ExcessAllocationList* excessAllocationList,
    ITMHashEntry *hashTable,

    uchar *entriesAllocType,
    uchar *entriesVisibleType,
    Vector4s *blockCoords)
{
    unsigned char hashChangeType = entriesAllocType[targetIdx];
    if (hashChangeType == 0) return;
    int ptr = voxelAllocationList->Allocate();
    if (ptr < 0) return; //there is no room in the voxel block array


    ITMHashEntry hashEntry;
    hashEntry.pos = TO_SHORT3(blockCoords[targetIdx]);
    hashEntry.ptr = ptr;
    hashEntry.offset = 0;

    int exlOffset;
    if (hashChangeType ==  AT_NEEDS_ALLOC_EXCESS) { //needs allocation in the excess list
        exlOffset = excessAllocationList->Allocate();

        if (exlOffset >= 0) //there is room in the excess list
        {
            hashTable[targetIdx].offset = exlOffset + 1; //connect parent to child

            targetIdx = SDF_BUCKET_NUM + exlOffset; // target index is in excess part
        }
    }

    hashTable[targetIdx] = hashEntry;
    entriesVisibleType[targetIdx] = VT_VISIBLE; //every new entry is visible
}

_CPU_AND_GPU_CODE_ inline void checkPointVisibility(THREADPTR(bool) &isVisible,
    const THREADPTR(Vector4f) &pt_model, const CONSTPTR(Matrix4f) & M_d, const CONSTPTR(Vector4f) &projParams_d,
    const CONSTPTR(Vector2i) &imgSize)
{
    Vector4f pt_camera; Vector2f pt_image;
    if (projectModel(projParams_d, M_d, imgSize, pt_model, pt_camera, pt_image)) {
        isVisible = true;
    }
}

#define indicator(x) (x ? 1.f : 0.f)
/// project the eight corners of the given voxel block
/// into the camera viewpoint and check their visibility
_CPU_AND_GPU_CODE_ inline void checkBlockVisibility(THREADPTR(bool) &isVisible,
    const THREADPTR(Vector3s) &hashPos, const CONSTPTR(Matrix4f) & M_d, const CONSTPTR(Vector4f) &projParams_d,
    const CONSTPTR(float) &voxelSize, const CONSTPTR(Vector2i) &imgSize)
{
    Vector4f pt_model;
    const float voxelBlockWorldSize = (float)SDF_BLOCK_SIZE * voxelSize;

    isVisible = false;

    pt_model = Vector4f(hashPos.toFloat() * voxelBlockWorldSize, 1);
    // loop over corners
    for (int xyz = 0; xyz <= 7; xyz++) {
        checkPointVisibility(isVisible, 
            pt_model + voxelBlockWorldSize * Vector4f(indicator(xyz & 4), indicator(xyz & 2), indicator(xyz & 1), 0),
            M_d, projParams_d, imgSize);
        if (isVisible) return;
    }
}

/// \returns hashVisibleType > 0
_CPU_AND_GPU_CODE_ inline bool visibilityTestIfNeeded(
    int targetIdx,
    uchar *entriesVisibleType, 
    ITMHashEntry *hashTable,
    Matrix4f M_d, Vector4f projParams_d, Vector2i depthImgSize, float voxelSize
    ) {
    unsigned char hashVisibleType = entriesVisibleType[targetIdx];
    const ITMHashEntry &hashEntry = hashTable[targetIdx];

    //  -- perform visibility check for voxel blocks that where visible in the last frame
    // but not yet detected in the current depth frame
    // (many of these will actually not be visible anymore)
    if (hashVisibleType == VT_VISIBLE_PREVIOUS)
    {
        bool isVisible;        
        checkBlockVisibility(isVisible, hashEntry.pos, M_d, projParams_d, voxelSize, depthImgSize);
        if (!isVisible) hashVisibleType = entriesVisibleType[targetIdx] = 0; // no longer visible

    }

    return hashVisibleType > 0;
}

_CPU_AND_GPU_CODE_ inline void integrateVoxel(int x, int y, int z,
    Vector3i globalPos, 
    ITMVoxel *localVoxelBlock, 
    float voxelSize,

    const CONSTPTR(Matrix4f) & M_d, const CONSTPTR(Vector4f) & projParams_d,
    const CONSTPTR(Matrix4f) & M_rgb, const CONSTPTR(Vector4f) & projParams_rgb,
    float mu, int maxW,
    const CONSTPTR(float) *depth, const CONSTPTR(Vector2i) & depthImgSize,
    const CONSTPTR(Vector4u) *rgb, const CONSTPTR(Vector2i) & rgbImgSize
    ) {
    const int locId = x + y * SDF_BLOCK_SIZE + z * SDF_BLOCK_SIZE * SDF_BLOCK_SIZE;

    // Voxel's world coordinates, for later projection into depth and color image
    const Vector4f pt_model = Vector4f(
        (globalPos.toFloat() + Vector3f((float)x, (float)y, (float)z)) * voxelSize, 1.f);

    computeUpdatedVoxelInfo(
        localVoxelBlock[locId],
        pt_model,
        M_d,
        projParams_d, M_rgb, projParams_rgb, mu, maxW, depth, depthImgSize, rgb, rgbImgSize);
}