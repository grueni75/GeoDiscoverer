//============================================================================
// Name        : Integer.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef INTEGER_H_
#define INTEGER_H_

namespace GEODISCOVERER {

class Integer {
public:

  // Constructor
  Integer();

  // Destructor
  virtual ~Integer();

  // Executes c=a+b and checks for overflow
  static bool add(Int a, Int b, Int &c);

};

}

#endif /* INTEGER_H_ */
