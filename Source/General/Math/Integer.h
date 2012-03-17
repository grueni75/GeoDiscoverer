//============================================================================
// Name        : Integer.h
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
