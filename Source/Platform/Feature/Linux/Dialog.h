//============================================================================
// Name        : Dialog.h
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
