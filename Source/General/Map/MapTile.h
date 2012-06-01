//============================================================================
// Name        : MapTile.h
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


#ifndef MAP_TILE_H_
#define MAP_TILE_H_

namespace GEODISCOVERER {

typedef enum { PictureBorderTop, PictureBorderBottom, PictureBorderRight, PictureBorderLeft } PictureBorder;

class MapContainer;
class MapPosition;
class MapArea;

class MapTile {

protected:

  // Properties
  bool                doNotDelete;              // Indicates if the object has been alloacted by an own memory handler
  Int                 mapX[2];                  // x Position in map (both sides)
  Int                 mapY[2];                  // y Position in map (both sides)
  Int                 visX;                     // x Position on the screen
  Int                 visY;                     // y Position on the screen
  Int                 visZ;                     // z Position on the screen
  Int                 width;                    // Width in Pixels
  Int                 height;                   // Height in Pixels
  Int                 distance;                 // Distance to a position
  MapContainer        *parent;                  // Map the tile belongs to
  bool                isDummy;                  // Tile is a dummy tile (i.e., has no map graphics)
  bool                isCached;                 // Tile is cached (has a map image texture)
  bool                isProcessed;              // Indicates that the tile has already been used during tile search
  bool                isHidden;                 // Indicates if the tile is not visible
  TimestampInSeconds  lastAccess;               // Time of last access

  // Generated variables
  GraphicRectangle  rectangle;            // Rectangle used for drawing the tile
  GraphicObject visualization;            // Visualization of the tile (texture rectangle + overlays such as pathes)
  GraphicPrimitiveKey visualizationKey;   // Key of the tile visualization in the graphic object
  double latNorthMax;                     // Maximum north border
  double latNorthMin;                     // Minimum north border
  double latSouthMax;                     // Maximum south border
  double latSouthMin;                     // Minimum south border
  double lngEastMin;                      // Minimum east border
  double lngEastMax;                      // Maximum east border
  double lngWestMax;                      // Maximum west border
  double lngWestMin;                      // Minimum west border
  double latScale;                        // Scale factor for latitude [pixel/degree]
  double lngScale;                        // Scale factor for longitude [pixel/degree]
  MapTile *leftChild;                     // Left child in the kd tree
  MapTile *rightChild;                    // Right child in the kd tree
  double lngX[2];                         // x Position in world (both sides)
  double latY[2];                         // y Position in world (both sides)
  double northAngle;                      // Angle to true north in degrees

  // Compares the latitude and longitude positions with the one of the given tile
  bool isNeighbor(MapTile *t, Int ownEdgeX, Int ownEdgeY, Int foreignEdgeX, Int foreignEdgeY) const;

  // Does the computationly intensive part of the initialization
  void init();

public:

  // Constructor
  MapTile(Int mapX, Int mapY, MapContainer *parent, bool doNotInit=false, bool doNotDelete=false);

  // Destructor
  virtual ~MapTile();

  // Destructs the objects correctly (i.e., if memory has not been allocated by new)
  static void destruct(MapTile *object);

  // Compares two tiles according to their distance
  static bool distanceSortPredicate(const MapTile *lhs, const MapTile *rhs)
  {
    return lhs->getDistance() < rhs->getDistance();
  }

  // Compares two tiles according to their parent
  static bool parentSortPredicate(const MapTile *lhs, const MapTile *rhs)
  {
    return lhs->getParentMapContainer() < rhs->getParentMapContainer();
  }

  // Compares two tiles according to their timestamp
  static bool lastAccessSortPredicate(const MapTile *lhs, const MapTile *rhs)
  {
    return lhs->getLastAccess() < rhs->getLastAccess();
  }

  // Checks if the tile is considered during drawing
  bool isDrawn() const
  {
    if (getVisualizationKey()!=0)
      return true;
    else
      return false;
  }

  // Checks if the tile has a texture
  bool hasTexture()
  {
    if (rectangle.getTexture()!=core->getScreen()->getTextureNotDefined())
      return true;
    else
      return false;
  }

  // Called when the tile is removed from the screen
  void removeGraphic();

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs);

  // Reads the contents of the object from a binary file
  static MapTile *retrieve(char *&cacheData, Int &cacheSize, MapContainer *parent);

  // Checks if the area is a neighbor of the tile and computes the center position of the neighbor
  bool getNeighborPos(MapArea area, MapPosition &neighborPos);

  // Checks if t is located at the upper left of this tile
  bool isNorthWestNeighbor(MapTile *t) const {
    return isNeighbor(t,0,0,1,1);
  }

  // Checks if t is located at the top of this tile
  bool isNorthNeighbor(MapTile *t) const {
    return isNeighbor(t,0,0,0,1) && isNeighbor(t,1,0,1,1);
  }

  // Checks if t is located at the upper right of this tile
  bool isNorthEastNeighbor(MapTile *t) const {
    return isNeighbor(t,1,0,0,1);
  }

  // Checks if t is located at the right of this tile
  bool isEastNeighbor(MapTile *t) const {
    return isNeighbor(t,1,0,0,0) && isNeighbor(t,1,1,0,1);
  }

  // Checks if t is located at the lower right of this tile
  bool isSouthEastNeighbor(MapTile *t) const {
    return isNeighbor(t,1,1,0,0);
  }

  // Checks if t is located at the bottom of this tile
  bool isSouthNeighbor(MapTile *t) const {
    return isNeighbor(t,0,1,0,0) && isNeighbor(t,1,1,1,0);
  }

  // Checks if t is located at the lower left of this tile
  bool isSouthWestNeighbor(MapTile *t) const {
    return isNeighbor(t,0,1,1,0);
  }

  // Checks if t is located at the left of this tile
  bool isWestNeighbor(MapTile *t) const {
    return isNeighbor(t,0,0,1,0) && isNeighbor(t,0,1,1,1);
  }

  // Transfers the current visual position to the rectangle
  void activateVisPos();

  // Getters and setters
  Int getMapBorder(PictureBorder border);

  GraphicRectangle *getRectangle()
  {
      return &rectangle;
  }

  Int getHeight() const
  {
      return height;
  }

  Int getWidth() const
  {
    return width;
  }

  Int getMapX(Int edge=0) const
  {
      return mapX[edge];
  }

  double getLngX(Int edge=0) const
  {
      return lngX[edge];
  }

  Int getMapCenterX() const
  {
      return getMapX()+getWidth()/2-1;
  }

  Int getMapY(Int edge=0) const
  {
      return mapY[edge];
  }

  Int getMapCenterY() const
  {
      return getMapY()+getHeight()/2-1;
  }

  double getLatY(Int edge=0) const
  {
      return latY[edge];
  }

  void setVisX(Int x)
  {
      visX=x;
  }

  Int getVisX(Int edge=0)
  {
    if (edge==0)
      return visX;
    else
      return visX+rectangle.getWidth()-1;
  }

  void setVisZ(Int z)
  {
      visZ=z;
  }

  void setVisName(std::list<std::string> name)
  {
      rectangle.setName(name);
  }

  Int getVisZ() const
  {
      return visZ;
  }

  std::list<std::string> getVisName() const
  {
      return rectangle.getName();
  }

  Int getVisCenterX() const
  {
      return visX+rectangle.getWidth()/2-1;
  }

  void setVisY(Int y)
  {
      visY=y;
  }

  Int getVisY(Int edge=0) const
  {
    if (edge==0)
      return visY;
    else
      return visY+rectangle.getHeight()-1;
  }

  Int getVisCenterY() const
  {
      return visY+rectangle.getHeight()/2-1;
  }

  Int getDistance() const
  {
      return distance;
  }

  void setDistance(Int distance)
  {
      this->distance = distance;
  }

  double getLatScale() const
  {
      return latScale;
  }

  double getLngScale() const
  {
      return lngScale;
  }

  GraphicPrimitiveKey getVisualizationKey() const
  {
      return visualizationKey;
  }

  void setVisualizationKey(GraphicPrimitiveKey visualizationKey)
  {
      this->visualizationKey = visualizationKey;
  }

  MapContainer *getParentMapContainer() const
  {
      return parent;
  }

  MapPosition getMapCenterPosition(void);

  bool getIsHidden() const
  {
      return isHidden;
  }

  bool getIsProcessed() const
  {
      return isProcessed;
  }

  void setIsProcessed(bool isProcessed)
  {
      this->isProcessed = isProcessed;
  }

  double getLatNorthMax() const
{
    return latNorthMax;
}

  double getLatNorthMin() const
  {
      return latNorthMin;
  }

  double getLatSouthMax() const
  {
      return latSouthMax;
  }

  double getLatSouthMin() const
  {
      return latSouthMin;
  }

  double getLngEastMax() const
  {
      return lngEastMax;
  }

  double getLngEastMin() const
  {
      return lngEastMin;
  }

  double getLngWestMax() const
  {
      return lngWestMax;
  }

  double getLngWestMin() const
  {
      return lngWestMin;
  }

  double getNorthAngle() const
  {
      return northAngle;
  }

  double getLatCenter() const
  {
    return latSouthMin+(latNorthMax-latSouthMin)/2;
  }

  double getLngCenter() const
  {
    return lngWestMin+(lngEastMax-lngWestMin)/2;
  }

  void setIsHidden(bool isHidden);

  TimestampInSeconds getLastAccess() const
  {
      return lastAccess;
  }

  void setLastAccess(TimestampInSeconds lastAccess)
  {
      this->lastAccess = lastAccess;
  }

  bool getIsDummy() const
  {
      return isDummy;
  }

  void setIsDummy(bool isDummy)
  {
      this->isDummy = isDummy;
  }

  bool getIsCached() const
  {
      return isCached;
  }

  void setIsCached(bool isCached)
  {
      this->isCached = isCached;
  }

  MapTile *getLeftChild() const
  {
      return leftChild;
  }

  MapTile *getRightChild() const
  {
      return rightChild;
  }

  void setLeftChild(MapTile *leftChild)
  {
      this->leftChild = leftChild;
  }

  void setRightChild(MapTile *rightChild)
  {
      this->rightChild = rightChild;
  }

  GraphicObject *getVisualization() {
    return &visualization;
  }

};

}

#endif /* MAP_TILE_H_ */
