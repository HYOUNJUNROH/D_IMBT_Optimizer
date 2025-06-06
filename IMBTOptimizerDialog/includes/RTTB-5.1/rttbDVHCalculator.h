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

#ifndef __DVH_CALCULATOR_H
#define __DVH_CALCULATOR_H

#include "rttbBaseType.h"
#include "rttbDoseIteratorInterface.h"
#include "rttbMaskedDoseIteratorInterface.h"
#include "rttbDVHGeneratorInterface.h"

#include "RTTBCoreExports.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace rttb
{
	namespace core
	{

		/*! @class DVHCalculator
			@brief Calculates a DVH for a given DoseIterator.
		*/
        class RTTBCore_EXPORT DVHCalculator : public DVHGeneratorInterface
		{
		public:
			using DoseIteratorPointer = core::DoseIteratorInterface::Pointer;
			using MaskedDoseIteratorPointer = core::MaskedDoseIteratorInterface::Pointer;

			DoseIteratorPointer _doseIteratorPtr;
			IDType _structureID;
			IDType _doseID;
			DoseTypeGy _deltaD;
			int _numberOfBins;

			/*! @brief Constructor.
				@param aDeltaD the absolute dose value in Gy for dose_bin [i,i+1). Optional, if aDeltaD==0,
				it will be calculated using aDeltaD=max(aDoseIterator)*1.5/aNumberOfBins
				@exception InvalidParameterException throw if _numberOfBins<=0 or _deltaD<0
			*/
			DVHCalculator(DoseIteratorPointer aDoseIterator, const IDType& aStructureID, const IDType& aDoseID,
			              const DoseTypeGy aDeltaD = 0, const int aNumberOfBins = 201);

			~DVHCalculator();

			/*! @brief Generate DVH
				@return Return new shared pointer of DVH.
				@exception InvalidParameterException throw if _numberOfBins invalid:
				_numberOfBins must be > max(aDoseIterator)/aDeltaD!
			*/
			DVH::Pointer generateDVH() override;

		};
	}

}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
