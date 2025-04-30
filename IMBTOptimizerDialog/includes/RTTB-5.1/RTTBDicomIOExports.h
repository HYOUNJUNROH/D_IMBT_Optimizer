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

#ifndef RTTBDicomIO_EXPORTS_H
  #define RTTBDicomIO_EXPORTS_H
  #if defined(WIN32) && !defined(RTTB_STATIC)
    #ifdef RTTBDicomIO_EXPORTS
      #define RTTBDicomIO_EXPORT __declspec(dllexport)
    #else
      #define RTTBDicomIO_EXPORT __declspec(dllimport)
    #endif
  #else
    #define RTTBDicomIO_EXPORT
  #endif
  #ifndef _CMAKE_MODULENAME
    #ifdef RTTBDicomIO_EXPORTS
      #define _CMAKE_MODULENAME "RTTBDicomIO"
    #endif
  #endif
#endif


