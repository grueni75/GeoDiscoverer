//============================================================================
// Name        : GraphicScaleAnimationParameter.h
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

#include <GraphicAnimationParameter.h>

#ifndef GRAPHICSCALEANIMATIONPARAMETER_H_
#define GRAPHICSCALEANIMATIONPARAMETER_H_

namespace GEODISCOVERER {

class GraphicScaleAnimationParameter : public GraphicAnimationParameter {

  double startFactor;                       // Start factor of scale operation
  double endFactor;                         // End factor of scale operation

public:

  // Constructor
  GraphicScaleAnimationParameter();

  // Destructor
  virtual ~GraphicScaleAnimationParameter();

  // Getters and setters
  double getEndFactor() const {
    return endFactor;
  }

  void setEndFactor(double endFactor) {
    this->endFactor = endFactor;
  }

  double getStartFactor() const {
    return startFactor;
  }

  void setStartFactor(double startFactor) {
    this->startFactor = startFactor;
  }

};

} /* namespace GEODISCOVERER */
#endif /* GRAPHICSCALEANIMATIONPARAMETER_H_ */
