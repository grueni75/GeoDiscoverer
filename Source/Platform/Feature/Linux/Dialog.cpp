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
  nextDialogKey=1;
}

// Destructor
Dialog::~Dialog() {

  // Delete all dialogs
  DialogProgressMap::iterator i;
  for(i = dialogProgressMap.begin(); i!=dialogProgressMap.end(); i++) {
    DialogKey key;
    DialogProgressState *state;
    key=i->first;
    state=i->second;
    delete state;
  }

}

// Creates a progress dialog
DialogKey Dialog::createProgress(std::string message, Int max) {

  DialogProgressState *state;

  // Create new state
  if (!(state=new DialogProgressState())) {
    FATAL("Can not create progress state struct",NULL);
    return 0;
  }
  state->max=max;

  // And remember it in the map
  DialogKey currentDialogKey=nextDialogKey;
  nextDialogKey++;
  if (nextDialogKey==0)
    nextDialogKey=1;
  DialogProgressPair p=DialogProgressPair(currentDialogKey,state);
  dialogProgressMap.insert(p);

  // Output initial state
  updateProgress(currentDialogKey,message,0);

  // Return result
  return currentDialogKey;
}

// Updates a progress dialog
void Dialog::updateProgress(DialogKey key, std::string message, Int value) {
  DialogProgressState *state=dialogProgressMap[key];
  std::stringstream out;
  out << message << " (" << value << "/" << state->max << ")";
  puts(out.str().c_str());
}

// Closes a progress dialog
void Dialog::closeProgress(DialogKey key) {
  DialogProgressState *state=dialogProgressMap[key];
  delete state;
  dialogProgressMap.erase(key);
}

}
