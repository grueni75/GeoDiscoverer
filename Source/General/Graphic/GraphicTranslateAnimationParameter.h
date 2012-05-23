//============================================================================
// Name        : GraphicTranslateAnimationParameter.h
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

#ifndef GRAPHICTRANSLATEANIMATIONPARAMETER_H_
#define GRAPHICTRANSLATEANIMATIONPARAMETER_H_

#include <Core.h>

namespace GEODISCOVERER {

class GraphicTranslateAnimationParameter: public GEODISCOVERER::GraphicAnimationParameter {

  double startX;          // Start x coordinate of translate operation
  double startY;          // Start y coordinate of translate operation
  double endX;            // End x coordinate of translate operation
  double endY;            // End y coordinate of translate operation

public:

  // Constructor
  GraphicTranslateAnimationParameter();

  // Destructor
  virtual ~GraphicTranslateAnimationParameter();

  // Getters and setters
  double getEndX() const {
    return endX;
  }

  void setEndX(double endX) {
    this->endX = endX;
  }

  double getEndY() const {
    return endY;
  }

  void setEndY(double endY) {
    this->endY = endY;
  }

  double getStartX() const {
    return startX;
  }

  void setStartX(double startX) {
    this->startX = startX;
  }

  double getStartY() const {
    return startY;
  }

  void setStartY(double startY) {
    this->startY = startY;
  }

};

} /* namespace GEODISCOVERER */
#endif /* GRAPHICTRANSLATEANIMATIONPARAMETER_H_ */
