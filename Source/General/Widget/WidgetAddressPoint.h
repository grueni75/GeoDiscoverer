//============================================================================
// Name        : WidgetAddressPoint.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2019 Matthias Gruenewald
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

#include <WidgetPrimitive.h>
#include <FontString.h>
#include <MapPosition.h>

#ifndef WIDGETADDRESSPOINT_H_
#define WIDGETADDRESSPOINT_H_

namespace GEODISCOVERER {

class WidgetAddressPoint: public WidgetPrimitive {

protected:

  // Contains the status of the nearest address point
  FontString *statusFontString;

  // Update interval
  TimestampInMicroseconds updateInterval;

  // Next timestamp when to update the widget
  TimestampInMicroseconds nextUpdateTime;

  // Indicates that this is the first time the widget runs
  bool firstRun;

  // Last update of the clock
  TimestampInSeconds lastClockUpdate;

  // Indicates if widget is active
  bool active;

  // Indicates if the widget shall be hidden if no address point is near by
  bool hideIfNoAddressPointNear;

  // Name of the nearby address point
  std::string nearestAddressPointName;

  // Last known location
  MapPosition locationPos;

  // Last position used for getting the nearest POI
  MapPosition poiUpdatePos;

  // Mininum distance to update the POI
  double poiUpdateRadius;

  // Identifies the nearest POI
  std::string poiName;
  MapPosition poiPos;
  double poiDistance;

public:

  // Constructor
  WidgetAddressPoint(WidgetContainer *widgetContainer);

  // Destructor
  virtual ~WidgetAddressPoint();

  // Let the button work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Called when some data has changed
  virtual void onDataChange();

  // Called when the location changes
  virtual void onLocationChange(bool widgetVisible, MapPosition pos);

  // Getters and setters
};

}

#endif /* WIDGETADDRESSPOINT_H_ */
