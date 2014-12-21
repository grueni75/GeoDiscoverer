//============================================================================
// Name        : GraphicAnimationParameter.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef GRAPHICANIMATIONPARAMETER_H_
#define GRAPHICANIMATIONPARAMETER_H_

namespace GEODISCOVERER {

typedef enum { GraphicAnimationParameterTypeTexture, GraphicAnimationParameterTypeFade, GraphicAnimationParameterTypeScale, GraphicAnimationParameterTypeTranslate, GraphicAnimationParameterTypeRotate } GraphicAnimationParameterType;

class GraphicAnimationParameter {

protected:

  GraphicAnimationParameterType type;           // Type of animation parameter
  TimestampInMicroseconds duration;             // Duration of the animation
  TimestampInMicroseconds startTime;            // Start of the animation
  bool infinite;                                // Indicates that this animation shall last forever

public:

  // Constructor
  GraphicAnimationParameter();

  // Destructor
  virtual ~GraphicAnimationParameter();

  // Getters and setters
  GraphicAnimationParameterType getType() const {
    return type;
  }

  TimestampInMicroseconds getDuration() const {
    return duration;
  }

  void setDuration(TimestampInMicroseconds duration) {
    this->duration = duration;
  }

  bool getInfinite() const {
    return infinite;
  }

  void setInfinite(bool infinite) {
    this->infinite = infinite;
  }

  TimestampInMicroseconds getStartTime() const {
    return startTime;
  }

  void setStartTime(TimestampInMicroseconds startTime) {
    this->startTime = startTime;
  }

};

} /* namespace GEODISCOVERER */
#endif /* GRAPHICANIMATIONPARAMETER_H_ */
