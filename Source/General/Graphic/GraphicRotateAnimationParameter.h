//============================================================================
// Name        : GraphicRotateAnimationParameter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef GRAPHICROTATEANIMATIONPARAMETER_H_
#define GRAPHICROTATEANIMATIONPARAMETER_H_

#include <Core.h>

namespace GEODISCOVERER {

typedef enum { GraphicRotateAnimationTypeLinear, GraphicRotateAnimationTypeAccelerated } GraphicRotateAnimationType;

class GraphicRotateAnimationParameter: public GEODISCOVERER::GraphicAnimationParameter {

  GraphicRotateAnimationType animationType; // Type of rotation
  double startAngle;                        // Start factor of scale operation
  double endAngle;                          // End factor of scale operation

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

  GraphicRotateAnimationType getAnimationType() const {
    return animationType;
  }

  void setAnimationType(GraphicRotateAnimationType animationType) {
    this->animationType = animationType;
  }
};

} /* namespace GEODISCOVERER */
#endif /* GRAPHICROTATEANIMATIONPARAMETER_H_ */
