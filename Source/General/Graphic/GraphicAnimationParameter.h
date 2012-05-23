//============================================================================
// Name        : GraphicAnimationParameter.h
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

#ifndef GRAPHICANIMATIONPARAMETER_H_
#define GRAPHICANIMATIONPARAMETER_H_

namespace GEODISCOVERER {

typedef enum { GraphicAnimationParameterTypeScale, GraphicAnimationParameterTypeTranslate, GraphicAnimationParameterTypeRotate } GraphicAnimationParameterType;

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
