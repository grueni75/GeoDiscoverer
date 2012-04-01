//============================================================================
// Name        : GraphicPosition.h
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


#ifndef GRAPHICPOSITION_H_
#define GRAPHICPOSITION_H_

namespace GEODISCOVERER {

class GraphicPosition {

protected:

  // Timestamp of the last modification by the user
  TimestampInMicroseconds lastUserModification;

  // The current angle
  double valueAngle;

  // The current zoom
  double valueZoom;

  // The current x and y position
  double valueX, valueY;

  // Indicates that the position has changed
  bool changed;

public:

  // Constructor
  GraphicPosition();

  // Destructor
  virtual ~GraphicPosition();

  // Operators
  bool operator==(const GraphicPosition &rhs);
  bool operator!=(const GraphicPosition &rhs);

  // Rotate
  void rotate(double degree);

  // Zoom
  void zoom(double scale);

  // Pan
  void pan(Int x, Int y);

  // Sets the position
  void set(Int valueX, Int valueY, double valueZoom, double valueAngle);

  // Indicates that postion has changed
  bool hasChanged() const
  {
      return changed;
  }

  // Resets the changed flag
  void resetChanged()
  {
      this->changed = false;
  }

  // Getters and setters
  double getAngle() const
  {
     return valueAngle;
  }
  double getAngleRad();

  void setAngle(double valueAngle)
  {
      this->valueAngle = valueAngle;
  }

  void setZoom(double valueZoom)
  {
      this->valueZoom = valueZoom;
  }

  double getZoom() const
  {
     return valueZoom;
  }
  Int getX() const
  {
     return round(valueX);
  }
  Int getY() const
  {
     return round(valueY);
  }

  TimestampInMicroseconds getLastUserModification() const
  {
      return lastUserModification;
  }

  void updateLastUserModification()
  {
      this->lastUserModification = core->getClock()->getMicrosecondsSinceStart();
  }
};

}

#endif /* GRAPHICPOSITION_H_ */
