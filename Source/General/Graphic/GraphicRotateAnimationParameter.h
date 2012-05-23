//============================================================================
// Name        : GraphicRotateAnimationParameter.h
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

#ifndef GRAPHICROTATEANIMATIONPARAMETER_H_
#define GRAPHICROTATEANIMATIONPARAMETER_H_

#include <Core.h>

namespace GEODISCOVERER {

class GraphicRotateAnimationParameter: public GEODISCOVERER::GraphicAnimationParameter {

  double startAngle;                       // Start factor of scale operation
  double endAngle;                         // End factor of scale operation

public:

  // Constructor
  GraphicRotateAnimationParameter();

  // Destructor
  virtual ~GraphicRotateAnimationParameter();

  // Getters and setters
  double getEndAngle() const {
    return endAngle;
  }

  void setEndAngle(double endAngle) {
    this->endAngle = endAngle;
  }

  double getStartAngle() const {
    return startAngle;
  }

  void setStartAngle(double startAngle) {
    this->startAngle = startAngle;
  }

};

} /* namespace GEODISCOVERER */
#endif /* GRAPHICROTATEANIMATIONPARAMETER_H_ */
