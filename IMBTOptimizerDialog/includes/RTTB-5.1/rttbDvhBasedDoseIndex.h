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

#ifndef __DVH_BASED_DOSE_INDEX_H
#define __DVH_BASED_DOSE_INDEX_H

#include "rttbBaseType.h"
#include "rttbDVHSet.h"
#include "rttbDoseIndex.h"

#include "RTTBIndicesExports.h"

namespace rttb
{
	namespace indices
	{
		/*! @class DvhBasedDoseIndex
		@brief This is the interface for dose/plan comparison indices calculated by DVh set of the dose.
		*/
        class RTTBIndices_EXPORT DvhBasedDoseIndex : public DoseIndex
		{
		protected:

			core::DVHSet::Pointer _dvhSet;

			/*! @brief Check inputs*/
			bool checkInputs() override;

		public:
			/*! @brief Constructor*/
			DvhBasedDoseIndex(core::DVHSet::Pointer aDVHSet, DoseTypeGy aDoseReference);
		};
	}
}


#endif
