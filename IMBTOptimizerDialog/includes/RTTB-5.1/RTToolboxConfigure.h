// -----------------------------------------------------------------------
// RTToolbox - DKFZ SIDT RT Interface
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

//----------------------------------------------------------
// !!!EXPERIMENTAL CODE!!!
//
// This code may not be used for release.
// Add #define SIDT_ENFORCE_MATURE_CODE to any release module
// to ensure this policy.
//----------------------------------------------------------
#ifdef SIDT_ENFORCE_MATURE_CODE
	#error "This code is marked as experimental code. It must not be used because this build enforces mature code."
#endif
#ifndef SIDT_CONTAINS_EXPERIMENTAL_CODE
  #define SIDT_CONTAINS_EXPERIMENTAL_CODE 1
#endif

/*! @def RTTB_BUILD_SHARED_LIBS
 * This define indicates if RTTB will be build as static library (define RTTB_STATIC) or as DLL (define RTTB_DLL).
 * By default RTTB_BUILD_SHARED_LIBS is not defined.
*/
/* #undef RTTB_BUILD_SHARED_LIBS */

#ifdef RTTB_BUILD_SHARED_LIBS
#define RTTB_DLL
#else
#define RTTB_STATIC
#endif

#ifndef __RTTB_CONFIGURE_H
	#define __RTTB_CONFIGURE_H

/*! @def RTTB_BUILD_SHARED_LIBS
 * This define indicates if RTTB will be build as static library (define RTTB_STATIC) or as DLL (define RTTB_DLL).
 * By default RTTB_BUILD_SHARED_LIBS is not defined.
*/
/* #undef RTTB_BUILD_SHARED_LIBS */

#ifdef RTTB_BUILD_SHARED_LIBS
	#define RTTB_DLL
#else
	#define RTTB_STATIC
#endif


/*! @def RTTB_ENFORCE_MATURE_CODE
 * This define controls if RTToolbox should force the whole build to be mature code.
 * Mature code convention is part of the SIDT coding styles.
 * Projects that use RTToolbox are able to ensure with SIDT_ENFORCE_MATURE_CODE
 * that used code is guaranteed to be tested and reviewed regarding the strict
 * SIDT coding styles.\n
 * RTTB_ENFORCE_MATURE_CODE can be used to ensure that strictness when prebuilding
 * static or dynamic libraries.
 * @remark This definition should by configured via the advance options in CMake.
*/
/* #undef RTTB_ENFORCE_MATURE_CODE */
#ifdef RTTB_ENFORCE_MATURE_CODE
	#define SIDT_ENFORCE_MATURE_CODE
#endif

/*! @def RTTB_DISABLE_ITK_IO_FACTORY_AUTO_REGISTER
 * This define controls if RTToolbox should disable the auto
 * register functionality of the itk io factory, when RTToolbox
 * io reader and writer classes are used.
 * This is needed in cases where RTToolbox is build "dynamic" and
 * used in an application that also uses the ITK shared objects
 * under Windows systems (e.g. MITK). Loading and unloading RTToolbox
 * shared objects in such an application would lead to an corrupted
 * itk factory stack, because its implementation is not fail safe
 * in this scenario.
 * @remark This definition should by configured via the advance options in CMake.
*/
/* #undef RTTB_DISABLE_ITK_IO_FACTORY_AUTO_REGISTER */

#define RTTB_VERSION_MAJOR 5
#define RTTB_VERSION_MINOR 1
#define RTTB_VERSION_PATCH 0
#define RTTB_VERSION_STRING "5.1"
#define RTTB_FULL_VERSION_STRING "5.1.0"


#endif
