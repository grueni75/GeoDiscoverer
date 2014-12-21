//============================================================================
// Name        : GraphicPoint.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
