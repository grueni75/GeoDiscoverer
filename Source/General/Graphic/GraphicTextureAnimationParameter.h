//============================================================================
// Name        : GraphicTextureAnimationParameter.h
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

#ifndef GRAPHICTEXTUREANIMATIONPARAMETER_H_
#define GRAPHICTEXTUREANIMATIONPARAMETER_H_

#include <Core.h>

namespace GEODISCOVERER {

class GraphicTextureAnimationParameter: public GEODISCOVERER::GraphicAnimationParameter {

  GraphicTextureInfo startTexture;    // Start texture of the animation
  GraphicTextureInfo endTexture;      // End texture of the animation

public:

  // Constructor
  GraphicTextureAnimationParameter();

  // Destructor
  virtual ~GraphicTextureAnimationParameter();

  // Getters and setters
  GraphicTextureInfo getEndTexture() const
  {
    return endTexture;
  }

  void setEndTexture(GraphicTextureInfo endTexture)
  {
    this->endTexture = endTexture;
  }

  GraphicTextureInfo getStartTexture() const
  {
    return startTexture;
  }

  void setStartTexture(GraphicTextureInfo startTexture)
  {
    this->startTexture = startTexture;
  }
};

} /* namespace GEODISCOVERER */
#endif /* GRAPHICTEXTUREANIMATIONPARAMETER_H_ */
