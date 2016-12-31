//============================================================================
// Name        : GraphicPoint.h
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

#ifndef GRAPHICPOINT_H_
#define GRAPHICPOINT_H_

namespace GEODISCOVERER {

class GraphicPoint {

protected:

  // X position
  Short x;

  // Y position
  Short y;

public:

  // Constructor
  GraphicPoint();

  // Constructor
  GraphicPoint(Short x, Short y);

  // Destructor
  virtual ~GraphicPoint();

  // Getters and setters
  Short getX() const {
    return x;
  }

  void setX(Short x) {
    this->x = x;
  }

  Short getY() const {
    return y;
  }

  void setY(Short y) {
    this->y = y;
  }

};

} /* namespace GEODISCOVERER */
#endif /* GRAPHICPOINT_H_ */
