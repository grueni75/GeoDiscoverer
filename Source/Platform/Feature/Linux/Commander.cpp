//============================================================================
// Name        : Commander.cpp
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

// Called when the initialization is finished
void Commander::dispatch(std::string command) {
  //DEBUG("command for parent app: %s",command.c_str());
  if (command=="decideContinueOrNewTrack()") {
    execute("setRecordTrack(1)");
  }
}

}
