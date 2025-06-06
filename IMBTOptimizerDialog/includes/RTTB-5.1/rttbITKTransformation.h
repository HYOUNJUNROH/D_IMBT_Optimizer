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

#ifndef __ITK_MAPPABLE_DOSE_ACCESSOR_H
#define __ITK_MAPPABLE_DOSE_ACCESSOR_H

#include <boost/shared_ptr.hpp>

#include "itkTransform.h"

#include "rttbTransformationInterface.h"

#include "RTTBInterpolationITKTransformationExports.h"

namespace rttb
{
	namespace interpolation
	{
		/*! @class ITKTransformation
		@brief This class can deal with dose information that has to be transformed into another geometry than the original dose image (transformation specified by ITK transformation object).
		*/
        class RTTBInterpolationITKTransformation_EXPORT ITKTransformation : public TransformationInterface
		{
		public:
			static const unsigned int InputDimension3D = 3;
			static const unsigned int OutputDimension3D = 3;
			using TransformScalarType = double;
			typedef itk::Transform<TransformScalarType, InputDimension3D, OutputDimension3D> Transform3D3DType;
			using InputPointType = Transform3D3DType::InputPointType;
			using OutputPointType = Transform3D3DType::OutputPointType;

		private:
			//! Has to be a Pointer type because of inheritance issues with itkSmartPointer (that doesn't recognize the inheritance)
			const Transform3D3DType* _pTransformation;

		protected:
			void convert(const WorldCoordinate3D& aWorldCoordinate, InputPointType& aInputPoint) const;
			void convert(const OutputPointType& aOutputPoint, WorldCoordinate3D& aWorldCoordinate) const;

		public:
			/*! @brief Constructor.
				@param aTransformation transformation in ITK format.
				@sa MappableDoseAccessorBase
				@pre all input parameters have to be valid
				@exception core::NullPointerException if one input parameter is nullptr
				@exception core::PaddingException if the transformation is undefined and if _acceptPadding==false
			*/
      explicit ITKTransformation(const Transform3D3DType* aTransformation);

			~ITKTransformation() override = default;

			/*! @brief performs a transformation targetImage --> movingImage
			*/
			bool transformInverse(const WorldCoordinate3D& worldCoordinateTarget,
			                      WorldCoordinate3D& worldCoordinateMoving) const override;
			/*! @brief performs a transformation movingImage --> targetImage
			*/
			bool transform(const WorldCoordinate3D& worldCoordinateMoving,
			               WorldCoordinate3D& worldCoordinateTarget) const override;
		};
	}
}

#endif
