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

#ifndef RTTBCore_EXPORTS_H
  #define RTTBCore_EXPORTS_H
  #if defined(WIN32) && !defined(RTTB_STATIC)
    #ifdef RTTBCore_EXPORTS
      #define RTTBCore_EXPORT __declspec(dllexport)
    #else
      #define RTTBCore_EXPORT __declspec(dllimport)
    #endif
  #else
    #define RTTBCore_EXPORT
  #endif
  #ifndef _CMAKE_MODULENAME
    #ifdef RTTBCore_EXPORTS
      #define _CMAKE_MODULENAME "RTTBCore"
    #endif
  #endif
#endif


