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

#ifndef __STR_VECTOR_STRUCTURE_SET_GENERATOR_H
#define __STR_VECTOR_STRUCTURE_SET_GENERATOR_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include "rttbStructureSetGeneratorInterface.h"
#include "rttbStructureSet.h"

#include "RTTBCoreExports.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace rttb
{
	namespace core
	{

		/*! @class StrVectorStructureSetGenerator
		@brief Generate a structure set with a vector of Structures.
		*/
		class RTTBCore_EXPORT StrVectorStructureSetGenerator : public core::StructureSetGeneratorInterface
		{
		public:
			using StructTypePointer = core::Structure::Pointer;

			using StructureSetPointer = StructureSet::Pointer;

		protected:
			IDType _patientUID;
			std::vector<StructTypePointer> _strVector;

			StrVectorStructureSetGenerator() = default;


		public:

			/*! @brief Constructor
			@param aStructureVector the vector of structure shared pointer
			@param aPatientUID the patient UID.
			*/
			StrVectorStructureSetGenerator(std::vector<StructTypePointer>& aStructureVector,
			                               IDType aPatientUID = "");

			/*! @brief Generate StructureSet
			@return Return shared pointer of StructureSet.
			*/
			StructureSetPointer generateStructureSet() override;
		};
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
