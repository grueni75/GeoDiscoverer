//============================================================================
// Name        : NavigationPath.h
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


#ifndef NAVIGATIONPATH_H_
#define NAVIGATIONPATH_H_

namespace GEODISCOVERER {

class NavigationPath {

protected:

  ThreadMutexInfo *accessMutex;                   // Mutex for accessing the path
  std::string name;                               // The name of the path
  std::string description;                        // The description of this path
  std::string gpxFilefolder;                      // Filename of the gpx file that stores this path
  std::string gpxFilename;                        // Folder where the gpx file is stored
  MapPosition lastPoint;                          // The last point added
  bool hasLastPoint;                              // Indicates if the path already has its last point
  MapPosition secondLastPoint;                    // The point added before the last point
  bool hasSecondLastPoint;                        // Indicates if the path already has its second last point
  std::list<MapPosition> mapPositions;            // List of map positions the path consists of
  bool blinkMode;                                 // Indicates if the path shall blink
  GraphicColor normalColor;                       // Normal color of the path
  GraphicColor highlightColor;                    // Highlight color of the path
  GraphicPrimitive animator;                      // Color animator
  GraphicPrimitiveKey animatorKey;                // Key of the animator in the graphic engine
  bool hasChanged;                                // Indicates if the path has changed
  bool isNew;                                     // Indicates if the path has just been created
  bool isStored;                                  // Indicates if the path has been written to disk
  bool hasBeenLoaded;                             // Indicates if the path has just been read from disk
  Int pathMinSegmentLength;                       // Minimum segment length of tracks, routes and other paths
  Int pathMinDirectionDistance;                   // Minimum distance between two direction arrows of tracks, routes and other paths
  Int pathWidth;                                  // Width of tracks, routes and other paths
  double length;                                  // Current length of the track in meters
  ThreadMutexInfo *isInitMutex;                   // Mutex for accessing the isInit variable
  bool isInit;                                    // Indicates if the track is initialized

  // Visualization of the path for each zoom level
  std::vector<NavigationPathVisualization*> zoomLevelVisualizations;

  // Finds nodes in a xml tree
  std::list<XMLNode> findNodes(XMLDocument document, XMLXPathContext xpathCtx, std::string path);

  // Returns the text contents of a element node
  bool getText(XMLNode node, std::string &contents);

  // Extracts information about the path from the given node set
  void extractInformation(std::list<XMLNode> nodes);

  // Updates the visualization of the tile
  void updateTileVisualization(std::list<MapContainer*> *mapContainers, NavigationPathVisualization *visualization, MapPosition prevPos, MapPosition prevArrowPos, MapPosition currentPos);

public:

  // Constructor
  NavigationPath();

  // Destructor
  virtual ~NavigationPath();

  // Adds a point to the path
  void addEndPosition(MapPosition pos);

  // Stores a the object contents in a gpx file
  void writeGPXFile();

  // Reads the path contents from a gpx file
  bool readGPXFile();

  // Clears the graphical representation
  void deinit();

  // Clears all points and sets a new gpx filname
  void init();

  // Indicates that textures and buffers should be cleared
  void destroyGraphic();

  // Indicates that textures and buffers should be recreated
  void createGraphic();

  // Recreate the graphic objects to reduce the number of graphic point buffers
  void optimizeGraphic();

  // Adds the visualization for the given containers
  void addVisualization(std::list<MapContainer*> *containers);

  // Remove the visualization for the given container
  void removeVisualization(MapContainer *container);

  // Starts the background loader
  void startBackgroundLoader();

  // Loads the path in the background
  void backgroundLoader();

  // Getters and setters
  void setGpxFilefolder(std::string gpxFilefolder)
  {
      this->gpxFilefolder = gpxFilefolder;
  }

  std::string getGpxFilefolder() const
  {
      return gpxFilefolder;
  }

  std::string getGpxFilename() const
  {
      return gpxFilename;
  }

  void setGpxFilename(std::string gpxFilename)
  {
      this->gpxFilename = gpxFilename;
  }

  bool getHasSecondLastPoint() const
  {
      return hasSecondLastPoint;
  }

  MapPosition getSecondLastPoint() const
  {
      return secondLastPoint;
  }

  std::string getDescription() const
  {
      return description;
  }

  std::string getName() const
  {
      return name;
  }

  void setDescription(std::string description)
  {
      this->description = description;
  }

  void setName(std::string name)
  {
      this->name = name;
  }

  bool getHasLastPoint() const
  {
      return hasLastPoint;
  }

  MapPosition getLastPoint() const
  {
      return lastPoint;
  }

  bool getHasChanged() const
  {
      return hasChanged;
  }

  double getLength() const
  {
      return length;
  }

  void resetHasChanged()
  {
      this->hasChanged = false;
  }

  static const MapPosition getPathInterruptedPos()
  {
      return MapPosition();
  }

  bool getHasBeenLoaded() const
  {
      return hasBeenLoaded;
  }

  void setHasBeenLoaded(bool hasBeenLoaded)
  {
      this->hasBeenLoaded = hasBeenLoaded;
  }

  bool getIsNew() const
  {
      return isNew;
  }

  void setIsNew(bool isNew)
  {
      this->isNew = isNew;
  }

  void setHighlightColor(GraphicColor highlightColor)
  {
      this->highlightColor = highlightColor;
      core->getGraphicEngine()->lockPathAnimators();
      animator.setColor(normalColor);
      animator.setBlinkAnimation(blinkMode,highlightColor);
      core->getGraphicEngine()->unlockPathAnimators();
  }

  void setNormalColor(GraphicColor normalColor)
  {
      this->normalColor = normalColor;
      core->getGraphicEngine()->lockPathAnimators();
      animator.setColor(normalColor);
      animator.setBlinkAnimation(blinkMode,highlightColor);
      core->getGraphicEngine()->unlockPathAnimators();
  }

  void setBlinkMode(bool blinkMode)
  {
      this->blinkMode = blinkMode;
      core->getGraphicEngine()->lockPathAnimators();
      animator.setColor(normalColor);
      animator.setBlinkAnimation(blinkMode,highlightColor);
      core->getGraphicEngine()->unlockPathAnimators();
  }

  bool getIsInit() const {
    bool isInit;
    core->getThread()->lockMutex(isInitMutex);
    isInit=this->isInit;
    core->getThread()->unlockMutex(isInitMutex);
    return isInit;
  }

  void setIsInit(bool isInit) {
    core->getThread()->lockMutex(isInitMutex);
    this->isInit = isInit;
    core->getThread()->unlockMutex(isInitMutex);
  }

};

}

#endif /* NAVIGATIONPATH_H_ */
