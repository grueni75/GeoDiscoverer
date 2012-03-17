//============================================================================
// Name        : MapArea.h
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


#ifndef MAPAREA_H_
#define MAPAREA_H_

namespace GEODISCOVERER {

class MapArea {

protected:

  Int zoomLevel;                // zoom level of this area
  Int yNorth;                   // North picture border
  Int ySouth;                   // South picture border
  Int xEast;                    // East picture border
  Int xWest;                    // West picture border
  double latNorth;              // North geographic border
  double latSouth;              // South geographic border
  double lngEast;               // East geographic border
  double lngWest;               // West geographic border
  MapPosition refPos;           // Reference position

public:

  // Constructors and destructor
  MapArea();
  virtual ~MapArea();

  // Operators
  bool operator==(const MapArea &rhs);
  bool operator!=(const MapArea &rhs);

  // Getters and setters
  Int getXEast() const
  {
      return xEast;
  }

  Int getYNorth() const
  {
      return yNorth;
  }

  MapPosition getRefPos() const
  {
      return refPos;
  }

  Int getYSouth() const
  {
      return ySouth;
  }

  Int getXWest() const
  {
      return xWest;
  }

  void setXEast(Int xEast)
  {
      this->xEast = xEast;
  }

  void setYNorth(Int yNorth)
  {
      this->yNorth = yNorth;
  }

  void setRefPos(MapPosition refPos)
  {
      this->refPos = refPos;
  }

  void setYSouth(Int ySouth)
  {
      this->ySouth = ySouth;
  }

  void setXWest(Int xWest)
  {
      this->xWest = xWest;
  }

  double getLatNorth() const
  {
      return latNorth;
  }

  double getLatSouth() const
  {
      return latSouth;
  }

  double getLngEast() const
  {
      return lngEast;
  }

  double getLngWest() const
  {
      return lngWest;
  }

  void setLatNorth(double latNorth)
  {
      this->latNorth = latNorth;
  }

  void setLatSouth(double latSouth)
  {
      this->latSouth = latSouth;
  }

  void setLngEast(double lngEast)
  {
      this->lngEast = lngEast;
  }

  void setLngWest(double lngWest)
  {
      this->lngWest = lngWest;
  }

  void setZoomLevel(Int zoomLevel)
  {
      this->zoomLevel = zoomLevel;
  }

  Int getZoomLevel() const
  {
      return zoomLevel;
  }

};

}

#endif /* MAPAREA_H_ */
