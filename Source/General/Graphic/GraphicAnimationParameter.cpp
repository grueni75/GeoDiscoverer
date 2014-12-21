//============================================================================
// Name        : GraphicAnimationParameter.cpp
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
GraphicAnimationParameter::GraphicAnimationParameter() {
  duration=0;
  infinite=false;
  startTime=0;
}

// Destructor
GraphicAnimationParameter::~GraphicAnimationParameter() {
}

} /* namespace GEODISCOVERER */
