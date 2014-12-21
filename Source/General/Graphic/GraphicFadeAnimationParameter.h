//============================================================================
// Name        : GraphicFadeAnimationParameter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
