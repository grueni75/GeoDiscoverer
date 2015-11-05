//============================================================================
// Name        : UnitConverter.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
UnitConverter::UnitConverter() {

  // Get selected unit system
  std::string unitSystemString=core->getConfigStore()->getStringValue("General","unitSystem",__FILE__, __LINE__);
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

// Converts a length in meters to the selected unit system and formats it for displaying purposes
void UnitConverter::formatBytes(double bytes, std::string &value, std::string &unit, Int precision, std::string lockedUnit) {

  // Convert the value and set the unit
  if ((((bytes/1024.0/1024.0/1024.0/1024.0)>=1.0)&&(lockedUnit==""))||(lockedUnit=="TB")) {
    bytes=bytes/1024.0/1024.0/1024.0/1024.0;
    unit="TB";
  } else if ((((bytes/1024.0/1024.0/1024.0)>=1.0)&&(lockedUnit==""))||(lockedUnit=="GB")) {
    bytes=bytes/1024.0/1024.0/1024.0;
    unit="GB";
  } else if ((((bytes/1024.0/1024.0)>=1.0)&&(lockedUnit==""))||(lockedUnit=="MB")) {
    bytes=bytes/1024.0/1024.0;
    unit="MB";
  } else if ((((bytes/1024.0)>=1.0)&&(lockedUnit==""))||(lockedUnit=="KB")) {
    bytes=bytes/1024.0;
    unit="KB";
  } else {
    unit="B";
  }

  // Format the value
  std::stringstream s;
  s.precision(precision);
  s.setf(std::ios_base::fixed);
  s<<bytes;
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

// Converts a speed in meters per second to the selected unit system and formats it for displaying purposes
void UnitConverter::formatTime(double timeInSeconds, std::string &value, std::string &unit, Int precision) {

  double time;

  // Convert the value and set the unit
  if ((timeInSeconds/60.0/60.0)>=1.0) {
    time=timeInSeconds/60.0/60.0;
    unit="h";
  } else if ((timeInSeconds/60.0)>=1.0) {
    time=timeInSeconds/60.0;
    unit="m";
  } else {
    time=timeInSeconds;
    unit="s";
  }

  // Format the value
  std::stringstream s;
  s.precision(precision);
  s.setf(std::ios_base::fixed);
  s<<time;
  value=s.str();
}

}
