//============================================================================
// Name        : GraphicFadeAnimationParameter.h
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

#ifndef GRAPHICFADEANIMATIONPARAMETER_H_
#define GRAPHICFADEANIMATIONPARAMETER_H_

#include <Core.h>

namespace GEODISCOVERER {

class GraphicFadeAnimationParameter: public GEODISCOVERER::GraphicAnimationParameter {

  GraphicColor startColor;      // Start color of fade operation
  GraphicColor endColor;        // End color of fade operation

public:

  // Constructor
  GraphicFadeAnimationParameter();

  // Destructor
  virtual ~GraphicFadeAnimationParameter();

  // Getters and setters
  GraphicColor getEndColor() const {
    return endColor;
  }

  void setEndColor(GraphicColor endColor) {
    this->endColor = endColor;
  }

  GraphicColor getStartColor() const {
    return startColor;
  }

  void setStartColor(GraphicColor startColor) {
    this->startColor = startColor;
  }

};

} /* namespace GEODISCOVERER */
#endif /* GRAPHICFADEANIMATIONPARAMETER_H_ */
