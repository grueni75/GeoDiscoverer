//============================================================================
// Name        : NavigationPath.h
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

#include <MapPosition.h>
#include <GraphicPrimitive.h>
#include <GraphicObject.h>
#include <NavigationPathVisualization.h>
#include <MapContainer.h>
#include <NavigationInfo.h>
#include <GraphicEngine.h>

#ifndef NAVIGATIONPATH_H_
#define NAVIGATIONPATH_H_

namespace GEODISCOVERER {

typedef enum {NavigationPathVisualizationTypeStartFlag, NavigationPathVisualizationTypeEndFlag } NavigationPathVisualizationType;

typedef enum {NavigationPathImportWaypointsUndecided=0, NavigationPathImportWaypointsYes=1, NavigationPathImportWaypointsNo=2 } NavigationPatImportWaypointsType;

class NavigationPath {

protected:

  char *cacheData;                                // Pointer to the cache (if used)
  std::vector<MapPosition> mapPositions;          // List of map positions the path consists of
  Int startIndex;                                 // Current start in mapPosition list
  Int endIndex;                                   // Current end in mapPosition list
  std::string name;                               // The name of the path
  std::string description;                        // The description of this path
  std::string gpxFilefolder;                      // Folder where the gpx file is stored
  std::string gpxCacheFilefolder;                 // Folder where the gpx file cache is stored
  std::string gpxFilename;                        // Filename of the gpx file that stores this path
  MapPosition lastPoint;                          // The last point added
  bool hasLastPoint;                              // Indicates if the path already has its last point
  MapPosition secondLastPoint;                    // The point added before the last point
  bool hasSecondLastPoint;                        // Indicates if the path already has its second last point
  double lastValidAltiudeMeters;                  // The last (filtered) altitude that was used for altitude meter computation
  MapPosition lastValidAltiudePos;                // The last position that was used for altitude meter computation
  bool blinkMode;                                 // Indicates if the path shall blink
  GraphicColor normalColor;                       // Normal color of the path
  GraphicColor highlightColor;                    // Highlight color of the path
  GraphicPrimitive animator;                      // Color animator
  GraphicPrimitiveKey animatorKey;                // Key of the animator in the graphic engine
  bool hasChanged;                                // Indicates if the path has changed
  bool isNew;                                     // Indicates if the path has just been created
  bool isStored;                                  // Indicates if the path has been written to disk
  bool hasBeenLoaded;                             // Indicates if the path has just been read from disk
  bool reverse;                                   // Indicates that the path shall be inversed during loading
  Int pathMinSegmentLength;                       // Minimum segment length of tracks, routes and other paths
  Int pathMinDirectionDistance;                   // Minimum distance between two direction arrows of tracks, routes and other paths
  Int pathWidth;                                  // Width of tracks, routes and other paths
  bool isInit;                                    // Indicates if the track is initialized
  double minDistanceToRouteWayPoint;              // Minimum distance to a point on the route to consider it as the target for navigation
  double maxDistanceToTurnWayPoint;               // Maximum distance in meters to a turn before it is indicated
  double minDistanceToTurnWayPoint;               // Minimum distance in meters to a turn before it is indicated
  double minTurnAngle;                            // Minimum angle at which a turn in the path is detected
  double turnDetectionDistance;                   // Distance in meters to look forward and back for detecting a turn
  double minDistanceToBeOffRoute;                 // Minimum distance from nearest route point such that navigation considers location to be off route
  double averageTravelSpeed;                      // Speed in meters per second to use for calculating the duration of a route
  double minDistanceToCalculateAltitude;          // Minimum distance to last position to calculate the altitude
  double altitudeBufferMinNegativeValue;          // Minimum negative value the altitude buffer may have
  double trackRecordingMinDistance;               // Required minimum navigationDistance in meter to the last track point such that the point is added to the track
  NavigationPatImportWaypointsType importWaypoints; // Decides if the waypoints contained in the route shall be imported
  bool calculateAltitudeGainsFromDEM;             // Decides if track/route altitude is ignored and DEM data is used instead to calculate altitude gains

  // Information about the path
  double length;                                  // Current length of the track in meters
  double altitudeUp;                              // Current total altitude of the track uphill in meters
  double altitudeDown;                            // Current total altitude of the track downhill in meters
  double minAltitude;                             // Minimum altitude of the track
  double maxAltitude;                             // Maximum altitude of the track

  // Visualization of the path for each zoom level
  std::vector<NavigationPathVisualization*> zoomLevelVisualizations;
  
  // Filter to smooth altitude values
  double altitudeUpBuffer, altitudeDownBuffer;

  // Finds nodes in a xml tree
  std::list<XMLNode> findNodes(XMLDocument document, XMLXPathContext xpathCtx, std::string path);

  // Extracts information about the path from the given node set
  void extractInformation(std::list<XMLNode> nodes);

  // Updates the visualization of the tile (path line and arrows)
  void updateTileVisualization(std::list<MapContainer*> *mapContainers, NavigationPathVisualization *visualization, MapPosition prevPos, MapPosition prevArrowPos, MapPosition currentPos);

  // Updates the crossing path segments in the map tiles of the given map containers for the new point
  void updateCrossingTileSegments(std::list<MapContainer*> *mapContainers, NavigationPathVisualization *visualization, Int pos);

  // Updates the metrics (altitude, length, duration, ...) of the path
  void updateMetrics();

  // Computes the metrics for the given map positions
  void updateMetrics(MapPosition prevPoint, MapPosition curPoint);

  // Store the contents of the object in a binary file
  void store(std::ofstream *ofs);

  // Reads the contents of the object from a binary file
  static bool retrieve(NavigationPath *navigationPath, char *&cacheData, Int &cacheSize);

  // Writes the cache to a file
  void writeCache();

public:

  // Constructor
  NavigationPath();

  // Destructor
  virtual ~NavigationPath();

  // Adds a point to the path
  void addEndPosition(MapPosition pos);

  // Stores a the object contents in a gpx file
  void writeGPXFile(bool forceStorage = false, bool skipBackup = false, bool onlySelectedPath = false, bool skipExtensions = false, std::string name = "", std::string filepath = "");

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

  // Computes navigation details for the given location
  void computeNavigationInfo(MapPosition locationPos, MapPosition &wayPoint, NavigationInfo &navigationInfo);

  // Sets the start flag at the given index
  void setStartFlag(Int index, const char *file, int line);

  // Sets the end flag at the given index
  void setEndFlag(Int index, const char *file, int line);

  // Enable or disable the blinking of the route
  void setBlinkMode(bool blinkMode, const char *file, int line);

  // Sets the gpx file name of the path
  void setGpxFilename(std::string gpxFilename);

  // Computes the distance from the start flag to the given point
  double computeDistance(MapPosition pos, double overlapInMeters, MapPosition &selectedPos);

  // Getters and setters
  void setGpxFilefolder(std::string gpxFilefolder);

  std::string getGpxFilefolder() const
  {
      return gpxFilefolder;
  }

  std::string getGpxFilename() const
  {
      return gpxFilename;
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

  double getDuration() const
  {
      return getLength() / averageTravelSpeed;
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

  void setHighlightColor(GraphicColor highlightColor, const char *file, int line)
  {
      this->highlightColor = highlightColor;
      core->getDefaultGraphicEngine()->lockPathAnimators(file, line);
      if (blinkMode)
        animator.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),normalColor,highlightColor,true,core->getDefaultGraphicEngine()->getBlinkDuration());
      else
        animator.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),normalColor,normalColor,false,0);
      core->getDefaultGraphicEngine()->unlockPathAnimators();
  }

  void setNormalColor(GraphicColor normalColor, const char *file, int line)
  {
      this->normalColor = normalColor;
      core->getDefaultGraphicEngine()->lockPathAnimators(file, line);
      if (blinkMode)
        animator.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),normalColor,highlightColor,true,core->getDefaultGraphicEngine()->getBlinkDuration());
      else
        animator.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),normalColor,normalColor,false,0);
      core->getDefaultGraphicEngine()->unlockPathAnimators();
  }

  bool getIsInit() {
    bool isInit;
    isInit=this->isInit;
    return isInit;
  }

  void setIsInit(bool isInit) {
    this->isInit = isInit;
    if (isInit) {
      if (reverse)
        updateMetrics();
    }
  }

  bool getReverse() const {
    return reverse;
  }

  void setReverse(bool reverse) {
    this->reverse = reverse;
  }

  void setImportWaypoints(NavigationPatImportWaypointsType importWaypoints) {
    this->importWaypoints = importWaypoints;
  }

  MapPosition getPoint(Int index) {
    return mapPositions[index];
  }

  std::vector<MapPosition> getSelectedPoints();

  double getAltitudeDown() const {
    return altitudeDown;
  }

  double getAltitudeUp() const {
    return altitudeUp;
  }

  double getMaxAltitude() const {
    return maxAltitude;
  }

  double getMinAltitude() const {
    return minAltitude;
  }

  Int getSelectedSize() const {
    if (mapPositions.size()==0)
      return 0;
    Int startIndex=0;
    Int endIndex=mapPositions.size()-1;
    if (this->reverse) {
      startIndex=endIndex;
      endIndex=0;
    }
    if (this->startIndex!=-1)
      startIndex=this->startIndex;
    if (this->endIndex!=-1)
      endIndex=this->endIndex;
    if (this->reverse)
      return startIndex-endIndex+1;
    else
      return endIndex-startIndex+1;
  }

  MapPosition getStartFlagPos() const {
    Int startIndex=reverse ? mapPositions.size()-1 : 0;
    if (this->startIndex!=-1)
      startIndex=this->startIndex;
    return mapPositions[startIndex];
  }

  MapPosition getEndFlagPos() const {
    Int endIndex=reverse ? 0 : mapPositions.size()-1;
    if (this->endIndex!=-1)
      endIndex=this->endIndex;
    return mapPositions[endIndex];
  }
};

}

#endif /* NAVIGATIONPATH_H_ */
