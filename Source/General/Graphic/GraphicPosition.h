//============================================================================
// Name        : GraphicPosition.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
