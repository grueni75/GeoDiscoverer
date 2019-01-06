//============================================================================
// Name        : MapSourceCalibratedPictures.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2018 Matthias Gruenewald
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


#ifndef MAPSOURCEREMOTE_H_
#define MAPSOURCEREMOTE_H_

namespace GEODISCOVERER {

class MapSourceRemote  : public MapSource {

protected:

  Int mapArchiveCacheSize;                          // Number of map archives to hold in the disk cache
  Int nextFreeArchiveNumber;                        // Next number to use to obtain a free map archive file

  // Loads all calibrated pictures in the given directory
  bool collectMapTiles(std::string directory, std::list<std::vector<std::string> > &mapFilebases);

public:

  // Constructurs and destructor
  MapSourceRemote();
  virtual ~MapSourceRemote();

  // Initialzes the source
  virtual bool init();

  // Clears the source
  virtual void deinit();

  // Returns the map tile in which the position lies
  virtual MapTile *findMapTileByGeographicCoordinate(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer=NULL);

  // Fills the given area with tiles
  virtual void fillGeographicAreaWithTiles(MapArea area, MapTile *preferredNeighbor, Int maxTiles, std::list<MapTile*> *tiles, bool *abort);

  // Returns the next free map archive file name
  virtual std::string getFreeArchiveFilePath();

  // Performs maintenance (e.g., recreate degraded search tree)
  virtual void maintenance();

  // Adds a new map archive
  virtual bool addArchive(std::string path);

  // Getters and setters

};

}

#endif /* MAPSOURCEREMOTE_H_ */
