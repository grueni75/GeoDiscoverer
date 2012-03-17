//============================================================================
// Name        : Dialog.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
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

// Executes an command on the java side
void GDApp_executeAppCommand(std::string command);

namespace GEODISCOVERER {

typedef Int DialogKey;

class Dialog {

protected:

public:

  // Constructor and destructor
  Dialog();
  virtual ~Dialog();

  // Creates a progress dialog
  DialogKey createProgress(std::string message, Int max);

  // Updates a progress dialog
  void updateProgress(DialogKey key, std::string message, Int current);

  // Closes a progress dialog
  void closeProgress(DialogKey key);

};

}

#endif /* DIALOG_H_ */
