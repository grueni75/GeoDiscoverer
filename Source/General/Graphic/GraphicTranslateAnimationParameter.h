//============================================================================
// Name        : GraphicTranslateAnimationParameter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef GRAPHICTRANSLATEANIMATIONPARAMETER_H_
#define GRAPHICTRANSLATEANIMATIONPARAMETER_H_

#include <Core.h>

namespace GEODISCOVERER {

typedef enum { GraphicTranslateAnimationTypeLinear, GraphicTranslateAnimationTypeAccelerated } GraphicTranslateAnimationType;

class GraphicTranslateAnimationParameter: public GEODISCOVERER::GraphicAnimationParameter {

  GraphicTranslateAnimationType animationType;  // Type of translation
  double startX;                                // Start x coordinate of translate operation
  double startY;                                // Start y coordinate of translate operation
  double endX;                                  // End x coordinate of translate operation
  double endY;                                  // End y coordinate of translate operation

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

  GraphicTranslateAnimationType getAnimationType() const {
    return animationType;
  }

  void setAnimationType(GraphicTranslateAnimationType animationType) {
    this->animationType = animationType;
  }
};

} /* namespace GEODISCOVERER */
#endif /* GRAPHICTRANSLATEANIMATIONPARAMETER_H_ */
