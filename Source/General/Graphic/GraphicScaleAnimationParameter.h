//============================================================================
// Name        : GraphicScaleAnimationParameter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

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
