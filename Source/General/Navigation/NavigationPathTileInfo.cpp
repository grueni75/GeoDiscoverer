//============================================================================
// Name        : NavigationPathTileInfo.cpp
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
NavigationPathTileInfo::NavigationPathTileInfo() {
  pathLineKey=0;
  pathLine=NULL;
  pathArrowListKey=0;
  pathArrowList=NULL;
  pathStartFlag=NULL;
  pathStartFlagKey=0;
  pathEndFlag=NULL;
  pathEndFlagKey=0;
}

// Destructor
NavigationPathTileInfo::~NavigationPathTileInfo() {
}

}
