//============================================================================
// Name        : Core.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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

#include <Core.h>
#include <proj.h>

namespace GEODISCOVERER {

// Init libproj
void Core::initProj() {

  // Configure where the data files of libproj are stored
#ifdef __ANDROID__  
  std::string dataPath=core->getHomePath();
  const char *paths[] = { dataPath.c_str() };
  proj_context_set_search_paths(NULL,1,paths);
#endif
}


} /* namespace GEODISCOVERER */
