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
