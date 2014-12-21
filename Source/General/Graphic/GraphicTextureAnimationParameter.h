//============================================================================
// Name        : GraphicTextureAnimationParameter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
