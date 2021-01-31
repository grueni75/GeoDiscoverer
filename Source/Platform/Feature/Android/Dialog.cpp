//============================================================================
// Name        : Dialog.cpp
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
#include <Dialog.h>

namespace GEODISCOVERER {

// Constructor
Dialog::Dialog() {
}

// Destructor
Dialog::~Dialog() {
}

// Creates a progress dialog
DialogKey Dialog::createProgress(std::string message, Int max) {
  std::stringstream out;
  out << "createProgressDialog(\"" << message << "\"," << max << ")";
  GDApp_executeAppCommand(out.str());
  return 0;
}

// Updates a progress dialog
void Dialog::updateProgress(DialogKey key, std::string message, Int value) {
  std::stringstream out;
  out << "updateProgressDialog(\"" << message << "\"," << value << ")";
  GDApp_executeAppCommand(out.str());
}

// Closes a progress dialog
void Dialog::closeProgress(DialogKey key) {
  GDApp_executeAppCommand("closeProgressDialog()");
}

}
