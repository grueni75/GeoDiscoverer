//============================================================================
// Name        : WidgetEngine.cpp
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

// Executes an command on the java side
void GDApp_executeAppCommand(std::string command);

namespace GEODISCOVERER {

void WidgetEngine::showContextMenu() {
  GDApp_executeAppCommand("showContextMenu()");
}

void WidgetEngine::setTargetAtAddress() {
  GDApp_executeAppCommand("askForAddress()");
}
}



