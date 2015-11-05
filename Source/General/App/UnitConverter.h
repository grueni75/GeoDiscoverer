//============================================================================
// Name        : UnitConverter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef UNITCONVERTER_H_
#define UNITCONVERTER_H_

namespace GEODISCOVERER {

// Supported unit systems
typedef enum {MetricSystem, ImperialSystem} UnitSystem;

class UnitConverter {

protected:

  // Selected system of unit
  UnitSystem unitSystem;

public:

  // Constructor and destructor
  UnitConverter();
  virtual ~UnitConverter();

  // Converts a length in meters to the selected unit system and formats it for displaying purposes
  void formatMeters(double lengthInMeters, std::string &value, std::string &unit, Int precision=2, std::string lockedUnit="");

  // Converts a size in bytes to the selected unit system and formats it for displaying purposes
  void formatBytes(double bytes, std::string &value, std::string &unit, Int precision=2, std::string lockedUnit="");

  // Converts a speed in meters per second to the selected unit system and formats it for displaying purposes
  void formatMetersPerSecond(double speedInMetersPerSecond, std::string &value, std::string &unit, Int precision=2);

  // Converts a time in seconds to the selected unit system and formats it for displaying purposes
  void formatTime(double timeInSeconds, std::string &value, std::string &unit, Int precision=2);

};

}

#endif /* UNITCONVERTER_H_ */
