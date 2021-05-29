//============================================================================
// Name        : NavigationPathTileInfo.h
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

#include <GraphicLine.h>
#include <GraphicPrimitive.h>
#include <GraphicRectangleList.h>
#include <GraphicObject.h>

#ifndef NAVIGATIONPATHTILEINFO_H_
#define NAVIGATIONPATHTILEINFO_H_

namespace GEODISCOVERER {

class NavigationPathTileInfo {

  // Info about the graphic line object
  GraphicPrimitiveKey pathLineKey;
  GraphicLine *pathLine;

  // Info about the graphic rectangle list object
  GraphicPrimitiveKey pathArrowListKey;
  GraphicRectangleList *pathArrowList;

  // List of visualized points
  std::list<MapPosition> visualizedPoints;

public:

  // Constructor
  NavigationPathTileInfo();

  // Destructor
  virtual ~NavigationPathTileInfo();

  // Adds a new point to the visualization only if it is not yet in
  bool addPoint(MapPosition point);

  // Getters and setters
  GraphicLine *getPathLine() const
  {
      return pathLine;
  }

  GraphicPrimitiveKey getPathLineKey() const
  {
      return pathLineKey;
  }

  void setPathLine(GraphicLine *pathLine)
  {
      this->pathLine = pathLine;
  }

  void setPathLineKey(GraphicPrimitiveKey pathLineKey)
  {
      this->pathLineKey = pathLineKey;
  }

  GraphicRectangleList *getPathArrowList() const
  {
      return pathArrowList;
  }

  GraphicPrimitiveKey getPathArrowListKey() const
  {
      return pathArrowListKey;
  }

  void setPathArrowList(GraphicRectangleList *pathArrowList)
  {
      this->pathArrowList = pathArrowList;
  }

  void setPathArrowListKey(GraphicPrimitiveKey pathArrowListKey)
  {
      this->pathArrowListKey = pathArrowListKey;
  }

};

}

#endif /* NAVIGATIONPATHTILEINFO_H_ */
