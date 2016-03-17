// Copyright 2014-2015 Isis Innovation Limited and the authors of InfiniTAM

#pragma once


#include <stdlib.h>

#include "../Utils/ITMLibDefines.h"
#include "../../ORUtils/Image.h"

namespace ITMLib
{
	namespace Objects
	{
		/** \brief
			Stores the render state used by the SceneReconstruction
			and Visualisation engines.
			*/
		class ITMRenderState
		{
		public:
			/** @brief
			Gives the raycasting operations an idea of the
			depth range to cover

			Each pixel contains an expected minimum and maximum
			depth. The raycasting step would use this
			information to reduce the range for searching an
			intersection with the actual surface. Should be
			updated by a ITMLib::Engine::ITMVisualisationEngine
			before any raycasting operation.

            Initialized to (vf_min, vf_max)
			*/
			ORUtils::Image<Vector2f> *renderingRangeImage;

			/** the 3D intersection locations generated by the latest raycast
            in voxel-fractional-world-coordinates
			*/
			ORUtils::Image<Vector4f> *raycastResult;

			ORUtils::Image<Vector4f> *forwardProjection;
			ORUtils::Image<int> *fwdProjMissingPoints;
			int noFwdProjMissingPoints;

			ORUtils::Image<Vector4u> *raycastImage;

			ITMRenderState(const Vector2i &imgSize, float vf_min, float vf_max, MemoryDeviceType memoryType)
			{
				renderingRangeImage = new ORUtils::Image<Vector2f>(imgSize, memoryType);
				raycastResult = new ORUtils::Image<Vector4f>(imgSize, memoryType);
				forwardProjection = new ORUtils::Image<Vector4f>(imgSize, memoryType);
				fwdProjMissingPoints = new ORUtils::Image<int>(imgSize, memoryType);
				raycastImage = new ORUtils::Image<Vector4u>(imgSize, memoryType);

                // Initialize renderingRangeImage to (vf_min, vf_max)
                Vector2f v_lims(vf_min, vf_max);
				ORUtils::Image<Vector2f> *buffImage = new ORUtils::Image<Vector2f>(imgSize, MEMORYDEVICE_CPU);
				for (int i = 0; i < imgSize.x * imgSize.y; i++) buffImage->GetData(MEMORYDEVICE_CPU)[i] = v_lims;

				if (memoryType == MEMORYDEVICE_CUDA)
                    renderingRangeImage->SetFrom(buffImage, ORUtils::MemoryBlock<Vector2f>::CPU_TO_CUDA);
				else
                    renderingRangeImage->SetFrom(buffImage, ORUtils::MemoryBlock<Vector2f>::CPU_TO_CPU);

				delete buffImage;

				noFwdProjMissingPoints = 0;
			}

			virtual ~ITMRenderState()
			{
				delete renderingRangeImage;
				delete raycastResult;
				delete forwardProjection;
				delete fwdProjMissingPoints;
				delete raycastImage;
			}
		};
	}
}
