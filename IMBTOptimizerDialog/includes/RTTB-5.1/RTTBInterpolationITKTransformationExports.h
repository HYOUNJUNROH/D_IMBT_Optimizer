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

#include "RTToolboxConfigure.h"

#ifndef RTTBInterpolationITKTransformation_EXPORTS_H
  #define RTTBInterpolationITKTransformation_EXPORTS_H
  #if defined(WIN32) && !defined(RTTB_STATIC)
    #ifdef RTTBInterpolationITKTransformation_EXPORTS
      #define RTTBInterpolationITKTransformation_EXPORT __declspec(dllexport)
    #else
      #define RTTBInterpolationITKTransformation_EXPORT __declspec(dllimport)
    #endif
  #else
    #define RTTBInterpolationITKTransformation_EXPORT
  #endif
  #ifndef _CMAKE_MODULENAME
    #ifdef RTTBInterpolationITKTransformation_EXPORTS
      #define _CMAKE_MODULENAME "RTTBInterpolationITKTransformation"
    #endif
  #endif
#endif


