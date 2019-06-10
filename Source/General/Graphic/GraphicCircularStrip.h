//============================================================================
// Name        : GraphicCircularStrip.h
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


#ifndef GRAPHICCIRCULARSTRIP_H_
#define GRAPHICCIRCULARSTRIP_H_

namespace GEODISCOVERER {

class GraphicCircularStrip : public GraphicRectangle {

protected:

  bool coordinatesOutdated;                   // Indicates that the texture and triangle coordinates need to be updated
  GraphicPointBuffer *textureCoordinates;     // Coordinates of the texture
  GraphicPointBuffer *triangleCoordinates;    // Coordinates of the triangles
  double radius;                              // Radius to the center of the circular strip
  double angle;                               // Angle in degrees to the middle of the strip
  bool inverse;                               // Inverse the texture

public:

  // Constructor
  GraphicCircularStrip(Screen *screen);

  // Let the primitive work
  virtual bool work(TimestampInMicroseconds currentTime);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Destructor
  virtual ~GraphicCircularStrip();

  // Recreates any textures or buffers
  virtual void invalidate();

  // Getters and setters
  double getAngle() const {
    return angle;
  }

  void setAngle(double angle) {
    if (angle!=this->angle) {
      coordinatesOutdated = true;
      this->angle = angle;
    }
  }

  double getRadius() const {
    return radius;
  }

  void setRadius(double radius) {
    if (radius!=this->radius) {
      coordinatesOutdated = true;
      this->radius = radius;
    }
  }

  virtual void setIconHeight(Int iconHeight)
  {
    if (iconHeight!=getIconHeight()) {
      coordinatesOutdated = true;
      GraphicRectangle::setIconHeight(iconHeight);
    }
  }

  virtual void setIconWidth(Int iconWidth)
  {
    if (iconWidth!=getIconWidth()) {
      coordinatesOutdated = true;
      GraphicRectangle::setIconWidth(iconWidth);
    }
  }

  virtual void setHeight(Int height)
  {
    if (height!=getHeight()) {
      coordinatesOutdated = true;
      GraphicRectangle::setHeight(height);
    }
  }

  virtual void setWidth(Int width)
  {
    if (width!=getWidth()) {
      coordinatesOutdated = true;
      GraphicRectangle::setWidth(width);
    }
  }

  void setInverse(bool inverse) {
    if (inverse!=this->inverse) {
      coordinatesOutdated = true;
      this->inverse = inverse;
    }
  }

  bool isInverse() const {
    return inverse;
  }

};

}

#endif /* GRAPHICRECTANGLE_H_ */
