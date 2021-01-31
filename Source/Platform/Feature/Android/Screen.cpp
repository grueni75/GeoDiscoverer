//============================================================================
// Name        : Screen.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2016 Matthias Gruenewald
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
#include <Screen.h>

// Executes an command on the java side
std::string GDApp_executeAppCommand(std::string command);

namespace GEODISCOVERER {

// If set to one, the screen is not turned off
void Screen::setWakeLock(bool state, const char *file, int line, bool persistent) {
  std::stringstream out;
  wakeLock=state;
  if (persistent)
    core->getConfigStore()->setIntValue("General","wakeLock",state,file,line);
  out << "updateWakeLock()";
  GDApp_executeAppCommand(out.str().c_str());
  if (state) {
    INFO("screen timeout is disabled",NULL);
  } else {
    INFO("screen timeout is enabled",NULL);
  }
}

}
