//============================================================================
// Name        : NavigationPath.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef NAVIGATIONPATH_H_
#define NAVIGATIONPATH_H_

namespace GEODISCOVERER {

typedef enum {NavigationPathVisualizationTypeStartFlag, NavigationPathVisualizationTypeEndFlag } NavigationPathVisualizationType;

class NavigationPath {

protected:

  std::vector<MapPosition> mapPositions;          // List of map positions the path consists of
  Int startIndex;                                 // Current start in mapPosition list
  Int endIndex;                                   // Current end in mapPosition list
  ThreadMutexInfo *accessMutex;                   // Mutex for accessing the path
  std::string name;                               // The name of the path
  std::string description;                        // The description of this path
  std::string gpxFilefolder;                      // Filename of the gpx file that stores this path
  std::string gpxFilename;                        // Folder where the gpx file is stored
  MapPosition lastPoint;                          // The last point added
  bool hasLastPoint;                              // Indicates if the path already has its last point
  MapPosition secondLastPoint;                    // The point added before the last point
  bool hasSecondLastPoint;                        // Indicates if the path already has its second last point
  MapPosition lastValidAltiudeMetersPoint;        // The last point that was used for altitude meter computation
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
  double minAltitudeChange;                       // Minimum change of altitude required to update altitude meters
  TimestampInMicroseconds flagAnimationDuration;  // Duration in microseconds that the flag animation shall last

  // Information about the path
  double length;                                  // Current length of the track in meters
  double altitudeUp;                              // Current total altitude of the track uphill in meters
  double altitudeDown;                            // Current total altitude of the track downhill in meters
  double minAltitude;                             // Minimum altitude of the track
  double maxAltitude;                             // Maximum altitude of the track

  // Visualization of the path for each zoom level
  std::vector<NavigationPathVisualization*> zoomLevelVisualizations;

  // Finds nodes in a xml tree
  std::list<XMLNode> findNodes(XMLDocument document, XMLXPathContext xpathCtx, std::string path);

  // Returns the text contents of a element node
  bool getText(XMLNode node, std::string &contents);

  // Extracts information about the path from the given node set
  void extractInformation(std::list<XMLNode> nodes);

  // Updates the visualization of the tile (path line and arrows)
  void updateTileVisualization(std::list<MapContainer*> *mapContainers, NavigationPathVisualization *visualization, MapPosition prevPos, MapPosition prevArrowPos, MapPosition currentPos);

  // Updates the visualization of the tile (start and end flag)
  void updateTileVisualization(NavigationPathVisualizationType type, std::list<MapContainer*> *mapContainers, NavigationPathVisualization *visualization, bool remove, bool animate);

  // Updates the crossing path segments in the map tiles of the given map containers for the new point
  void updateCrossingTileSegments(std::list<MapContainer*> *mapContainers, NavigationPathVisualization *visualization, Int pos);

  // Updates the metrics (altitude, length, duration, ...) of the path
  void updateMetrics();

  // Computes the metrics for the given map positions
  void updateMetrics(MapPosition prevPoint, MapPosition curPoint);

  // Updates the flag visualization for all zoom levels
  void updateFlagVisualization(NavigationPathVisualizationType type, bool remove, bool animate);

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

  // Computes navigation details for the given location
  void computeNavigationInfo(MapPosition locationPos, MapPosition &wayPoint, NavigationInfo &navigationInfo);

  // Sets the start flag at the given index
  void setStartFlag(Int index, const char *file, int line);

  // Sets the end flag at the given index
  void setEndFlag(Int index, const char *file, int line);

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

  void setBlinkMode(bool blinkMode, const char *file, int line)
  {
      this->blinkMode = blinkMode;
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

  void lockAccess(const char *file, int line) {
    core->getThread()->lockMutex(accessMutex, file, line);
  }

  void unlockAccess() {
    core->getThread()->unlockMutex(accessMutex);
  }

  Int getSelectedSize() const {
    Int startIndex=0;
    Int endIndex=mapPositions.size()-1;
    if (this->startIndex!=-1)
      startIndex=this->startIndex;
    if (this->endIndex!=-1)
      endIndex=this->endIndex;
    if (this->reverse)
      return startIndex-endIndex+1;
    else
      return endIndex-startIndex+1;
  }
};

}

#endif /* NAVIGATIONPATH_H_ */
