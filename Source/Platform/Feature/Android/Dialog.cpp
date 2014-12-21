//============================================================================
// Name        : Dialog.cpp
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
