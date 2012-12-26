//============================================================================
// Name        : NavigationPathTileInfo.h
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


#ifndef NAVIGATIONPATHTILEINFO_H_
#define NAVIGATIONPATHTILEINFO_H_

namespace GEODISCOVERER {

class NavigationPathTileInfo {

  // Info about the graphic line object
  GraphicPrimitiveKey graphicLineKey;
  GraphicLine *graphicLine;

  // Info about the graphic rectangle list object
  GraphicPrimitiveKey graphicRectangleListKey;
  GraphicRectangleList *graphicRectangleList;

public:

  // Constructor
  NavigationPathTileInfo();

  // Destructor
  virtual ~NavigationPathTileInfo();

  // Getters and setters
  GraphicLine *getGraphicLine() const
  {
      return graphicLine;
  }

  GraphicPrimitiveKey getGraphicLineKey() const
  {
      return graphicLineKey;
  }

  void setGraphicLine(GraphicLine *graphicLine)
  {
      this->graphicLine = graphicLine;
  }

  void setGraphicLineKey(GraphicPrimitiveKey graphicLineKey)
  {
      this->graphicLineKey = graphicLineKey;
  }

  GraphicRectangleList *getGraphicRectangeList() const
  {
      return graphicRectangleList;
  }

  GraphicPrimitiveKey getGraphicRectangleListKey() const
  {
      return graphicRectangleListKey;
  }

  void setGraphicRectangleList(GraphicRectangleList *graphicRectangleList)
  {
      this->graphicRectangleList = graphicRectangleList;
  }

  void setGraphicRectangleListKey(GraphicPrimitiveKey graphicRectangleListKey)
  {
      this->graphicRectangleListKey = graphicRectangleListKey;
  }

};

}

#endif /* NAVIGATIONPATHTILEINFO_H_ */
