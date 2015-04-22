//============================================================================
// Name        : Screen.cpp
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

// If set to one, the screen is not turned off
void Screen::setWakeLock(bool state, const char *file, int line, bool persistent) {
  std::stringstream out;
  wakeLock=state;
  if (persistent)
    core->getConfigStore()->setIntValue("General","wakeLock",state,file,line);
  if (state) {
    INFO("screen timeout is disabled",NULL);
  } else {
    INFO("screen timeout is enabled",NULL);
  }
}

}
