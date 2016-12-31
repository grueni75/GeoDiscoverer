//============================================================================
// Name        : UnitConverter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2016 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// GeoDiscoverer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GeoDiscoverer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GeoDiscoverer.  If not, see <http://www.gnu.org/licenses/>.
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
