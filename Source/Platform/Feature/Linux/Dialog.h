//============================================================================
// Name        : Dialog.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef DIALOG_H_
#define DIALOG_H_

namespace GEODISCOVERER {

typedef Int DialogKey;

// State of a dialog
struct DialogProgressState {
  std::string message;     // Message to display
  Int max;                 // Maximum value
  Int current;             // Current value
};

typedef std::map<DialogKey, DialogProgressState*> DialogProgressMap;
typedef std::pair<DialogKey, DialogProgressState*> DialogProgressPair;

class Dialog {

protected:

  // Holds all progress dialogs
  DialogProgressMap dialogProgressMap;

  // Next key to use for a rectangle
  DialogKey nextDialogKey;

public:

  // Constructor and destructor
  Dialog();
  ~Dialog();

  // Creates a progress dialog
  DialogKey createProgress(std::string message, Int max);

  // Updates a progress dialog
  void updateProgress(DialogKey key, std::string message, Int current);

  // Closes a progress dialog
  void closeProgress(DialogKey key);

};

}

#endif /* DIALOG_H_ */
