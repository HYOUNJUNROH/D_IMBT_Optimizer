// -----------------------------------------------------------------------
// RTToolbox - DKFZ radiotherapy quantitative evaluation library
//
// Copyright (c) German Cancer Research Center (DKFZ),
// Software development for Integrated Diagnostics and Therapy (SIDT).
// ALL RIGHTS RESERVED.
// See rttbCopyright.txt or
// http://www.dkfz.de/en/sidt/projects/rttb/copyright.html
//
// This software is distributed WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE.  See the above copyright notices for more information.
//
//------------------------------------------------------------------------

#ifndef __MASK_VOXEL_NEW_H
#define __MASK_VOXEL_NEW_H

#include "rttbBaseType.h"

#include "RTTBCoreExports.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace rttb
{

	namespace core
	{
		/*! @class MaskVoxel
		*	@brief AMaskVoxel stores the VoxelGridID of the corresponding dose voxel and the corresponding volume fraction
			as defined by the given mask.
		*/
		class RTTBCore_EXPORT MaskVoxel
		{
		private:
			/*! @brief A 1D voxel grid index on dose grid */
			VoxelGridID _voxelGridID;

			/*! @brief The relevant volume fraction that is masked by the given structure: 0~1 */
			FractionType _volumeFraction;

		public:
			/*! @brief Constructor
				@pre aVoxelGridID needs to point to a valid grid position.
			*/
      explicit MaskVoxel(const VoxelGridID& aVoxelGridID);

			/*! @brief Constructor
				@pre aVoxelGridID needs to point to a valid grid position.
			*/
			MaskVoxel(const VoxelGridID& aVoxelGridID, FractionType aVolumeFraction);

			/*! @brief Operator ==
			@return Return true if the id and volumeFraction are equal to these of the maskVoxel
			*/
			bool operator==(const MaskVoxel& maskVoxel) const;

			/*! @brief Operator <
			@return Return true if the id < the id of the maskVoxel
			*/
			bool operator<(const MaskVoxel& maskVoxel) const;

			const VoxelGridID& getVoxelGridID() const;

			void setRelevantVolumeFraction(const FractionType aVolumeFraction);

			FractionType getRelevantVolumeFraction() const;

			friend std::ostream& operator<<(std::ostream& s, const MaskVoxel& maskVoxel)
			{
				s << "( " << maskVoxel.getVoxelGridID() << ": " << maskVoxel.getRelevantVolumeFraction() << " )";
				return s;
			};
		};
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
