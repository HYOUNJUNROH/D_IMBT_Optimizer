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

#ifndef __DOSE_STATISTICS_XML_WRITER_H
#define __DOSE_STATISTICS_XML_WRITER_H

#include "rttbDoseStatistics.h"

#include <boost/property_tree/ptree.hpp>

#include "RTTBOtherIOExports.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace rttb
{
	namespace io
	{
		namespace other
		{
			class RTTBOtherIO_EXPORT DoseStatisticsXMLWriter
			{
				public:

					using DoseStatisticsPtr = boost::shared_ptr<rttb::algorithms::DoseStatistics>;

					/*! @brief Write statistics to boost::property_tree::ptree.
					@param aDoseStatistics DoseStatistics to write
					@pre aReferenceDose must >0
					@exception rttb::core::InvalidParameterException Thrown if aReferenceDose<=0 or xml_parse_error
					@note The precision is float
					*/
					boost::property_tree::ptree writeDoseStatistics(DoseStatisticsPtr aDoseStatistics);

					/*! @brief Write statistics to String.
					@param aDoseStatistics DoseStatistics to write
					@pre aReferenceDose must >0
					@exception rttb::core::InvalidParameterException Thrown if aReferenceDose<=0 or xml_parse_error
					@note The precision is float
					*/
					XMLString writerDoseStatisticsToString(DoseStatisticsPtr aDoseStatistics);

					/*! @brief Write statistics to xml file.
					@details includes the following statistics:
					- numberOfVoxels,
					- volume,
					- minimum,
					- maximum,
					- mean,
					- standard deviation,
					- variance,
					- D2,D5,D10,D90,D95,D98,
					- V2,V5,V10,V90,V95,V98,
					- MOH2,MOH5,MOH10,
					- MOC2,MOC5,MOC10,
					- MaxOH2,MaxOH5,MaxOH10,
					- MinOC2,MinOC5,MinOC10.
					@param aDoseStatistics DoseStatistics to write
					@param aFileName To write dose statistics
					@pre aReferenceDose must >0
					@exception rttb::core::InvalidParameterException Thrown if aReferenceDose<=0 or xml_parse_error
					@note The precision is float
					*/
					void writeDoseStatistics(DoseStatisticsPtr aDoseStatistics, FileNameString aFileName);

					boost::property_tree::ptree createNodeWithNameAttribute(DoseTypeGy doseValue, const std::string& attributeName);
					boost::property_tree::ptree createNodeWithNameAndXAttribute(DoseTypeGy doseValue, const std::string& attributeName, int xValue);

					/*! @brief Write statistics to String to generate a table
					@details The table is: "Volume mm3@Max@Min@Mean@Std.Dev.@Variance@D2@D5@D10@D90@D95@D98@V2@V5@V10@V90@V95@V98@MOH2@MOH5@MOH10@MOC2@MOC5@MOC10@MaxOH2@MaxOH5@MaxOH10@MinOC2@MinOC5@MinOC10"
					@param aDoseStatistics DoseStatistics to write
					@pre aReferenceDose must >0
					@exception rttb::core::InvalidParameterException Thrown if aReferenceDose<=0 or xml_parse_error
					@note is used for the Mevislab-Linking of RTTB
					@note The precision is float
					*/
					StatisticsString writerDoseStatisticsToTableString(DoseStatisticsPtr aDoseStatistics);

					double convertToPercent(double value, double maximum);
			};
		}
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
