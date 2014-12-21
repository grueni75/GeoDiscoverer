//============================================================================
// Name        : GraphicTextureAnimationParameter.cpp
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

GraphicTextureAnimationParameter::GraphicTextureAnimationParameter() : GraphicAnimationParameter() {
  type=GraphicAnimationParameterTypeTexture;
}

GraphicTextureAnimationParameter::~GraphicTextureAnimationParameter() {
}

} /* namespace GEODISCOVERER */
