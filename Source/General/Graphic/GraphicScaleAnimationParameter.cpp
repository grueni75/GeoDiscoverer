//============================================================================
// Name        : GraphicScaleAnimationParameter.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <Core.h>

namespace GEODISCOVERER {

// Constructor
GraphicScaleAnimationParameter::GraphicScaleAnimationParameter() : GraphicAnimationParameter() {
  type=GraphicAnimationParameterTypeScale;
}

// Destructor
GraphicScaleAnimationParameter::~GraphicScaleAnimationParameter() {
}

} /* namespace GEODISCOVERER */
