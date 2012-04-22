//============================================================================
// Name        : UnitConverter.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
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


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
UnitConverter::UnitConverter() {

  // Get selected unit system
  std::string unitSystemString=core->getConfigStore()->getStringValue("General","unitSystem");
  if (unitSystemString=="metric") {
    unitSystem=MetricSystem;
  } else if (unitSystemString=="imperial") {
    unitSystem=ImperialSystem;
  } else {
    FATAL("unit system <%s> is not supported",unitSystemString.c_str());
    return;
  }

}

// Destructor
UnitConverter::~UnitConverter() {
}

// Converts a length in meters to the selected unit system and formats it for displaying purposes
void UnitConverter::formatMeters(double lengthInMeters, std::string &value, std::string &unit, Int precision, std::string lockedUnit) {

  double length;

  // Convert the value and set the unit
  switch(unitSystem) {
    case MetricSystem:
      length=lengthInMeters;
      if ((((length/1000.0/1000.0)>=1.0)&&(lockedUnit==""))||(lockedUnit=="Mm")) {
        length=length/1000.0/1000.0;
        unit="Mm";
      } else if ((((length/1000.0)>=1.0)&&(lockedUnit==""))||(lockedUnit=="km")) {
        length=length/1000.0;
        unit="km";
      } else {
        unit="m";
      }
      break;
    case ImperialSystem:
      length=lengthInMeters/0.9144;  // 1 yard = 0.9144 meters
      if ((((length/1760.0)>=1.0)&&(lockedUnit==""))||(lockedUnit=="mi")) {
        length=length/1760.0;
        unit="mi";
      } else {
        unit="yd";
      }
      break;
  }

  // Format the value
  std::stringstream s;
  s.precision(precision);
  s.setf(std::ios_base::fixed);
  s<<length;
  value=s.str();
}

// Converts a speed in meters per second to the selected unit system and formats it for displaying purposes
void UnitConverter::formatMetersPerSecond(double speedInMetersPerSecond, std::string &value, std::string &unit, Int precision) {

  double speed;

  // Convert the value and set the unit
  switch(unitSystem) {
    case MetricSystem:
      speed=speedInMetersPerSecond/1000.0*3600.0;
      unit="km/h";
      break;
    case ImperialSystem:
      speed=speedInMetersPerSecond/0.9144/1760.0*3600.0;  // 1 yard = 0.9144 meters
      unit="mph";
      break;
  }

  // Format the value
  std::stringstream s;
  s.precision(precision);
  s.setf(std::ios_base::fixed);
  s<<speed;
  value=s.str();
}

}
