//============================================================================
// Name        : NavigationPath.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
NavigationPath::NavigationPath() : animator(core->getDefaultScreen()) {

  // Init variables
  accessMutex=core->getThread()->createMutex("navigation path access mutex");
  gpxFilefolder=core->getNavigationEngine()->getTrackPath();
  pathMinSegmentLength=core->getConfigStore()->getIntValue("Graphic","pathMinSegmentLength", __FILE__, __LINE__);
  pathMinDirectionDistance=core->getConfigStore()->getIntValue("Graphic","pathMinDirectionDistance", __FILE__, __LINE__);
  pathWidth=core->getConfigStore()->getIntValue("Graphic","pathWidth", __FILE__, __LINE__);
  flagAnimationDuration=core->getConfigStore()->getIntValue("Graphic","flagAnimationDuration", __FILE__, __LINE__);
  minDistanceToRouteWayPoint=core->getConfigStore()->getDoubleValue("Navigation","minDistanceToRouteWayPoint", __FILE__, __LINE__);
  minTurnAngle=core->getConfigStore()->getDoubleValue("Navigation","minTurnAngle", __FILE__, __LINE__);
  minDistanceToTurnWayPoint=core->getConfigStore()->getDoubleValue("Navigation","minDistanceToTurnWayPoint", __FILE__, __LINE__);
  maxDistanceToTurnWayPoint=core->getConfigStore()->getDoubleValue("Navigation","maxDistanceToTurnWayPoint", __FILE__, __LINE__);
  turnDetectionDistance=core->getConfigStore()->getDoubleValue("Navigation","turnDetectionDistance", __FILE__, __LINE__);
  minDistanceToBeOffRoute=core->getConfigStore()->getDoubleValue("Navigation","minDistanceToBeOffRoute", __FILE__, __LINE__);
  minAltitudeChange=core->getConfigStore()->getDoubleValue("Navigation","minAltitudeChange", __FILE__, __LINE__);
  averageTravelSpeed=core->getConfigStore()->getDoubleValue("Navigation","averageTravelSpeed", __FILE__, __LINE__);
  isInit=false;
  reverse=false;
  lastValidAltiudeMetersPoint=NavigationPath::getPathInterruptedPos();

  // Do the dynamic initialization
  init();

  // Add the animator to the graphic engine
  GraphicObject *pathAnimators=core->getDefaultGraphicEngine()->lockPathAnimators(__FILE__, __LINE__);
  animatorKey=pathAnimators->addPrimitive(&animator);
  core->getDefaultGraphicEngine()->unlockPathAnimators();
}

// Destructor
NavigationPath::~NavigationPath() {

  // Deinit everything
  deinit();

  // Remove the animator
  GraphicObject *pathAnimators=core->getDefaultGraphicEngine()->lockPathAnimators(__FILE__, __LINE__);
  pathAnimators->removePrimitive(animatorKey,false);
  core->getDefaultGraphicEngine()->unlockPathAnimators();

  // Free variables
  core->getThread()->destroyMutex(accessMutex);
}

// Updates the visualization of the tile (path line and arrows)
void NavigationPath::updateTileVisualization(std::list<MapContainer*> *mapContainers, NavigationPathVisualization *visualization, MapPosition prevPos, MapPosition prevArrowPos, MapPosition currentPos) {

  // Compute the search boundary
  double lngEast,lngWest;
  lngEast=currentPos.getLng();
  if (prevPos.getLng()>lngEast) {
    lngWest=lngEast;
    lngEast=prevPos.getLng();
  } else {
    lngWest=prevPos.getLng();
  }
  double latNorth,latSouth;
  latNorth=currentPos.getLat();
  if (prevPos.getLat()>latNorth) {
    latSouth=latNorth;
    latNorth=prevPos.getLat();
  } else {
    latSouth=prevPos.getLat();
  }

  // Find all map tiles that overlap with this segment
  std::list<MapContainer*> foundContainers;
  MapArea area;
  area.setZoomLevel(visualization->getZoomLevel());
  double latExtend=pathWidth/visualization->getLatScale();
  double lngExtend=pathWidth/visualization->getLngScale();
  area.setLatNorth(latNorth+latExtend);
  area.setLatSouth(latSouth-latExtend);
  area.setLngWest(lngWest-lngExtend);
  area.setLngEast(lngEast+lngExtend);
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
  if (!mapContainers) {
    foundContainers=core->getMapSource()->findMapContainersByGeographicArea(area);
  } else {
    for (std::list<MapContainer*>::iterator i=mapContainers->begin();i!=mapContainers->end();i++) {
      if ((area.getLngEast()>=(*i)->getLngWest())&&(area.getLngWest()<=(*i)->getLngEast())&&
          (area.getLatNorth()>=(*i)->getLatSouth())&&(area.getLatSouth()<=(*i)->getLatNorth())) {
        foundContainers.push_back(*i);
      }
    }
  }

  // Go through all of them
  for(std::list<MapContainer*>::iterator j=foundContainers.begin();j!=foundContainers.end();j++) {

    // Compute the coordinates of the segment within the container
    MapContainer *mapContainer=*j;
    MapPosition prevArrowPos=prevPos;
    bool overflowOccured=false;
    if (!mapContainer->getMapCalibrator()->setPictureCoordinates(prevPos)) {
      overflowOccured=true;
    }
    if (!mapContainer->getMapCalibrator()->setPictureCoordinates(prevArrowPos)) {
      overflowOccured=true;
    }
    if (!mapContainer->getMapCalibrator()->setPictureCoordinates(currentPos)) {
      overflowOccured=true;
    }
    if (!overflowOccured) {

      // Find all map tiles that overlap the area defined by the line segment
      if (prevPos.getX()>currentPos.getX()) {
        area.setXEast(prevPos.getX());
        area.setXWest(currentPos.getX());
      } else {
        area.setXWest(prevPos.getX());
        area.setXEast(currentPos.getX());
      }
      if (prevPos.getY()>currentPos.getY()) {
        area.setYSouth(prevPos.getY());
        area.setYNorth(currentPos.getY());
      } else {
        area.setYNorth(prevPos.getY());
        area.setYSouth(currentPos.getY());
      }
      area.setXEast(area.getXEast()+pathWidth);
      area.setXWest(area.getXWest()-pathWidth);
      area.setYNorth(area.getYNorth()-pathWidth);
      area.setYSouth(area.getYSouth()+pathWidth);
      std::list<MapTile*> mapTiles=mapContainer->findMapTilesByPictureArea(area);

      // Go through all of them
      for(std::list<MapTile*>::iterator k=mapTiles.begin();k!=mapTiles.end();k++) {

        // Check if the map tile has already graphic objects for this path
        NavigationPathTileInfo *info=visualization->findTileInfo(*k);
        GraphicLine *line=info->getPathLine();
        GraphicObject *tileVisualization=(*k)->getVisualization();
        if (!line) {

          // Create a new line and add it to the tile
          line=new GraphicLine(core->getDefaultScreen(),0,pathWidth);
          if (!line) {
            FATAL("can not create graphic line object",NULL);
            return;
          }
          line->setAnimator(&animator);
          line->setZ(1); // ensure that line is drawn after tile texture
          line->setCutEnabled(true);
          line->setCutWidth((*k)->getWidth());
          line->setCutHeight((*k)->getHeight());
          info->setPathLine(line);
          core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
          info->setPathLineKey(tileVisualization->addPrimitive(line));
          core->getDefaultGraphicEngine()->unlockDrawing();

        }

        // Add the stroke to the line
        Int x1=prevPos.getX()-(*k)->getMapX(0);
        Int y1=(*k)->getHeight()-(prevPos.getY()-(*k)->getMapY(0));
        Int x2=currentPos.getX()-(*k)->getMapX(0);
        Int y2=(*k)->getHeight()-(currentPos.getY()-(*k)->getMapY(0));
        core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
        line->addStroke(x1,y1,x2,y2);
        core->getDefaultGraphicEngine()->unlockDrawing();

        // Add arrow if necessary
        if (currentPos.getHasBearing()) {

          // Get the existing rectangle list or create a new one
          GraphicRectangleList *rectangleList=info->getPathArrowList();
          if (!rectangleList) {

            // Create a new rectangle list and add it to the tile
            rectangleList=new GraphicRectangleList(core->getDefaultScreen(),0);
            if (!rectangleList) {
              FATAL("can not create graphic rectangle list object",NULL);
              return;
            }
            rectangleList->setAnimator(&animator);
            rectangleList->setZ(2); // ensure that arrow is drawn after line texture
            rectangleList->setTexture(core->getDefaultGraphicEngine()->getPathDirectionIcon()->getTexture());
            rectangleList->setDestroyTexture(false);
            rectangleList->setCutEnabled(true);
            rectangleList->setCutWidth((*k)->getWidth());
            rectangleList->setCutHeight((*k)->getHeight());
            Int arrowWidth=core->getDefaultGraphicEngine()->getPathDirectionIcon()->getWidth();
            Int arrowHeight=core->getDefaultGraphicEngine()->getPathDirectionIcon()->getHeight();
            Int arrowXToCenter=arrowWidth/2-core->getDefaultGraphicEngine()->getPathDirectionIcon()->getIconWidth()/2;
            Int arrowYToCenter=arrowHeight/2-core->getDefaultGraphicEngine()->getPathDirectionIcon()->getIconHeight()/2;
            double totalRadius=sqrt((double)(arrowWidth*arrowWidth/4+arrowHeight*arrowHeight/4));
            double distanceToCenter=sqrt((double)(arrowXToCenter*arrowXToCenter+arrowYToCenter*arrowYToCenter));
            double angleToCenter;
            if (arrowXToCenter==0) {
              angleToCenter=M_PI/2;
            } else {
              angleToCenter=atan((double)arrowYToCenter/(double)arrowXToCenter);
            }
            rectangleList->setParameter(totalRadius,distanceToCenter,angleToCenter);
            info->setPathArrowList(rectangleList);
            core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
            info->setPathArrowListKey(tileVisualization->addPrimitive(rectangleList));
            core->getDefaultGraphicEngine()->unlockDrawing();
          }

          // Add the arrow
          if (reverse) {
            Int t=x1;
            x1=x2;
            x2=t;
            t=y1;
            y1=y2;
            y2=t;
          }
          Int distX=x2-x1;
          Int distY=y2-y1;
          double dist=sqrt((double)(distX*distX+distY*distY));
          double angle=FloatingPoint::computeAngle(distX,distY);
          double x=x1+dist/2*cos(angle);
          double y=y1+dist/2*sin(angle);
          core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
          rectangleList->addRectangle(x,y,angle);
          core->getDefaultGraphicEngine()->unlockDrawing();

        }
      }
    }
  }
  core->getMapSource()->unlockAccess();
}

// Updates the visualization of the tile (start and end flag)
void NavigationPath::updateTileVisualization(NavigationPathVisualizationType type, std::list<MapContainer*> *mapContainers, NavigationPathVisualization *visualization, bool remove, bool animate) {

  // Get the position to search
  MapPosition pos;
  switch(type) {
    case NavigationPathVisualizationTypeStartFlag:
      if (startIndex!=-1)
        pos = mapPositions[startIndex];
      else
        return;
      break;
    case NavigationPathVisualizationTypeEndFlag:
      if (endIndex!=-1)
        pos = mapPositions[endIndex];
      else
        return;
      break;
    default:
      FATAL("unsupported visualization type",NULL);
      return;
  }

  // Find all map tiles that overlap the given
  std::list<MapContainer*> foundContainers;
  MapArea area;
  area.setZoomLevel(visualization->getZoomLevel());
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
  if (!mapContainers) {
    foundContainers=core->getMapSource()->findMapContainersByGeographicCoordinate(pos,visualization->getZoomLevel());
  } else {
    for (std::list<MapContainer*>::iterator i=mapContainers->begin();i!=mapContainers->end();i++) {
      if ((pos.getLng()>=(*i)->getLngWest())&&(pos.getLng()<=(*i)->getLngEast())&&
          (pos.getLat()>=(*i)->getLatSouth())&&(pos.getLat()<=(*i)->getLatNorth())) {
        foundContainers.push_back(*i);
      }
    }
  }

  // Go through all of them
  TimestampInMicroseconds t = core->getClock()->getMicrosecondsSinceStart();
  for(std::list<MapContainer*>::iterator j=foundContainers.begin();j!=foundContainers.end();j++) {

    // Compute the coordinates of the segment within the container
    MapContainer *mapContainer=*j;
    bool overflowOccured=false;
    if (!mapContainer->getMapCalibrator()->setPictureCoordinates(pos)) {
      overflowOccured=true;
    }
    if (!overflowOccured) {

      // Find map tile that contains the position
      MapTile *k=mapContainer->findMapTileByPictureCoordinate(pos);
      if (k) {

        // Check if the map tile has already graphic objects for this path
        NavigationPathTileInfo *info=visualization->findTileInfo(k);
        GraphicRectangle *flag=NULL;
        switch(type) {
          case NavigationPathVisualizationTypeStartFlag:
            flag=info->getPathStartFlag();
            break;
          case NavigationPathVisualizationTypeEndFlag:
            flag=info->getPathEndFlag();
            break;
        }
        GraphicObject *tileVisualization=k->getVisualization();
        if (!remove) {
          if (!flag) {

            // Create a new flag and add it to the tile
            flag=new GraphicRectangle(core->getDefaultScreen());
            if (!flag) {
              FATAL("can not create graphic rectangle object",NULL);
              return;
            }
            flag->setAnimator(&animator);
            GraphicRectangle *ref = NULL;
            switch(type) {
              case NavigationPathVisualizationTypeStartFlag:
                ref = core->getDefaultGraphicEngine()->getPathStartFlagIcon();
                info->setPathStartFlag(flag);
                flag->setZ(3); // ensure that start flag is drawn after tile and direction texture
                break;
              case NavigationPathVisualizationTypeEndFlag:
                ref = core->getDefaultGraphicEngine()->getPathEndFlagIcon();
                info->setPathEndFlag(flag);
                flag->setZ(4); // ensure that end flag is drawn after start flag, tile and direction texture
                break;
            }
            flag->setColor(GraphicColor(255,255,255,255));
            flag->setTexture(ref->getTexture());
            flag->setDestroyTexture(false);
            flag->setIconWidth(ref->getIconWidth());
            flag->setIconHeight(ref->getIconHeight());
            flag->setWidth(ref->getWidth());
            flag->setHeight(ref->getHeight());
            Int endX=pos.getX()-k->getMapX(0)-flag->getIconWidth()/2;
            Int endY=k->getHeight()-(pos.getY()-k->getMapY(0))-flag->getIconHeight()/2;
            Int startX=endX;
            Int startY=endY+core->getDefaultScreen()->getHeight();
            if (animate) {
              flag->setX(startX);
              flag->setY(startY);
              flag->setTranslateAnimation(t,startX,startY,endX,endY,false,flagAnimationDuration,GraphicTranslateAnimationTypeAccelerated);
            } else {
              flag->setX(endX);
              flag->setY(endY);
            }
            core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
            switch(type) {
              case NavigationPathVisualizationTypeStartFlag:
                info->setPathStartFlagKey(tileVisualization->addPrimitive(flag));
                break;
              case NavigationPathVisualizationTypeEndFlag:
                info->setPathEndFlagKey(tileVisualization->addPrimitive(flag));
                break;
            }
            core->getDefaultGraphicEngine()->unlockDrawing();
          }
        } else {
          if (flag) {
            GraphicPrimitiveKey key;
            GraphicRectangle *flag;
            switch(type) {
              case NavigationPathVisualizationTypeStartFlag:
                key=info->getPathStartFlagKey();
                flag=info->getPathStartFlag();
                info->setPathStartFlag(NULL);
                info->setPathStartFlagKey(0);
                break;
              case NavigationPathVisualizationTypeEndFlag:
                key=info->getPathEndFlagKey();
                flag=info->getPathEndFlag();
                info->setPathEndFlag(NULL);
                info->setPathEndFlagKey(0);
                break;
            }
            core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
            tileVisualization->removePrimitive(key,true);
            core->getDefaultGraphicEngine()->unlockDrawing();
          }
        }
      }
    }
  }
  core->getMapSource()->unlockAccess();
}

// Updates the crossing path segments in the map tiles of the given map containers for the new point
void NavigationPath::updateCrossingTileSegments(std::list<MapContainer*> *mapContainers, NavigationPathVisualization *visualization, Int pos) {

  std::list<MapContainer*> foundContainers;

  // Find the map containers if necessary
  if (mapContainers==NULL) {
    foundContainers = core->getMapSource()->findMapContainersByGeographicCoordinate(visualization->getPoint(pos),visualization->getZoomLevel());
    mapContainers = &foundContainers;
  }

  // Add this point to the path segments of all tiles it lies within
  for(std::list<MapContainer*>::iterator i=mapContainers->begin();i!=mapContainers->end();i++) {
    MapContainer* mapContainer=*i;
    MapPosition t=visualization->getPoint(pos);
    if (mapContainer->getMapCalibrator()->setPictureCoordinates(t)) {
      MapTile* mapTile=mapContainer->findMapTileByPictureCoordinate(t);
      if (mapTile) {

        // Check if the point continues a previous segment
        std::list<NavigationPathSegment*>* pathSegments = mapTile->findCrossingNavigationPathSegments(this);
        bool addNewSegment=false;
        if (pathSegments==NULL) {
          addNewSegment=true;
        }  else {
          addNewSegment=true;
          for (std::list<NavigationPathSegment*>::iterator j=pathSegments->begin();j!=pathSegments->end();j++) {
            if ((pos>=(*j)->getStartIndex())&&(pos<=(*j)->getEndIndex())) {
              addNewSegment=false;
              break; // already present
            }
            if (pos==(*j)->getEndIndex()+1) {
              (*j)->setEndIndex(pos);
              addNewSegment=false;
              break; // existing segment extended
            }
          }
        }

        // If not, add a new segment
        if (addNewSegment) {
          NavigationPathSegment *pathSegment;
          if (!(pathSegment=new NavigationPathSegment())) {
            FATAL("can not create navigation path segment object",NULL);
            break;
          }
          pathSegment->setPath(this);
          pathSegment->setVisualization(visualization);
          pathSegment->setStartIndex(pos);
          pathSegment->setEndIndex(pathSegment->getStartIndex());
          mapTile->addCrossingNavigationPathSegment(this, pathSegment);
        }

      }
    }
  }
}

// Adds a point to the path
void NavigationPath::addEndPosition(MapPosition pos) {

  lockAccess(__FILE__, __LINE__);

  // Decide whether to add a new point or use the last one
  if (!hasLastPoint) {
    hasLastPoint=true;
  } else {
    secondLastPoint=lastPoint;
    hasSecondLastPoint=true;
  }
  lastPoint=pos;

  // Add the new pair
  mapPositions.push_back(pos);
  pos.setIndex(mapPositions.size()-1);

  // Update the length and altitude meters
  if (endIndex==-1) {
    updateMetrics(secondLastPoint,lastPoint);
  }

  // Path was modified
  hasChanged=true;
  isStored=false;

  // Update the visualization for each zoom level
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    NavigationPathVisualization *visualization=*i;

    // Get the calibrator for this point
    core->getMapSource()->lockAccess(__FILE__,__LINE__);
    bool deleteCalibrator=false;
    MapCalibrator *calibrator=NULL;
    if (pos!=NavigationPath::getPathInterruptedPos())
      calibrator=visualization->findCalibrator(pos,deleteCalibrator);

    // Just add this point if the prev point or the current point interrupted the line
    if ((visualization->getPrevLinePoint()==NavigationPath::getPathInterruptedPos())||(pos==NavigationPath::getPathInterruptedPos())) {
      if ((pos==NavigationPath::getPathInterruptedPos())||(calibrator!=NULL)) {
        visualization->addPoint(pos);
      }
      if (calibrator!=NULL) {
        updateCrossingTileSegments(NULL, visualization, visualization->getPointsSize()-1);
      }
      core->getMapSource()->unlockAccess();

    } else {

      // Check if the point is far enough away from the previous point for this zoom level
      if (!calibrator) {

        // Position can not be found in map, interrupt it
        visualization->addPoint(NavigationPath::getPathInterruptedPos());
        core->getMapSource()->unlockAccess();

      } else {
        double length=calibrator->computePixelDistance(visualization->getPrevLinePoint(),pos);
        if (length>=pathMinSegmentLength) {

          // Check if an arrow needs to be added
          bool addArrow=false;
          double arrowDistance=calibrator->computePixelDistance(visualization->getPrevArrowPoint(),pos);
          core->getMapSource()->unlockAccess();
          if (arrowDistance>=pathMinDirectionDistance) {
            pos.setHasBearing(true);  // bearing flag is used to indicate that an arrow must be added
          } else {
            pos.setHasBearing(false);
          }

          // Add a line stroke to all matching tiles
          updateTileVisualization(NULL,visualization,visualization->getPrevLinePoint(),visualization->getPrevArrowPoint(),pos);

          // Add the visualization of the flags
          updateTileVisualization(NavigationPathVisualizationTypeStartFlag,NULL,visualization,false,false);
          updateTileVisualization(NavigationPathVisualizationTypeEndFlag,NULL,visualization,false,false);

          // Update the previous point
          visualization->addPoint(pos);
          updateCrossingTileSegments(NULL, visualization, visualization->getPointsSize()-1);

        } else {
          core->getMapSource()->unlockAccess();
        }
      }
    }

    // Delete the calibrator if required
    if ((deleteCalibrator)&&(calibrator))
      delete calibrator;

  }
  unlockAccess();

  // Inform the widgets
  core->onPathChange(this,NavigationPathChangeTypeEndPositionAdded);
}

// Clears the graphical representation
void NavigationPath::deinit() {

  // Delete the zoom level visualization
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    delete *i;
  }
  zoomLevelVisualizations.clear();

  // Remove from all tiles the path segments
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
  std::vector<MapContainer*> *containers=core->getMapSource()->getMapContainers();
  for (std::vector<MapContainer*>::iterator i=containers->begin();i!=containers->end();i++) {
    std::vector<MapTile*> *tiles=(*i)->getMapTiles();
    for (std::vector<MapTile*>::iterator j=tiles->begin();j!=tiles->end();j++) {
      (*j)->removeCrossingNavigationPathSegments(this);
    }
  }
  core->getMapSource()->unlockAccess();

  // Force a redraw
  GraphicObject *pathAnimators=core->getDefaultGraphicEngine()->lockPathAnimators(__FILE__, __LINE__);
  pathAnimators->setIsUpdated(true);
  core->getDefaultGraphicEngine()->unlockPathAnimators();

  // Is not initialized
  setIsInit(false);
}

// Clears all points and sets a new gpx filname
void NavigationPath::init() {

  // Reset variables
  hasLastPoint=false;
  hasSecondLastPoint=false;
  lastPoint=NavigationPath::getPathInterruptedPos();
  secondLastPoint=NavigationPath::getPathInterruptedPos();
  gpxFilename="track-" + core->getClock()->getFormattedDate() + ".gpx";
  setName("Track-" + core->getClock()->getFormattedDate());
  setDescription("Track recorded with GeoDiscoverer.");
  hasChanged=true;
  isStored=true;
  hasBeenLoaded=false;
  isNew=true;
  mapPositions.clear();
  blinkMode=false;
  startIndex=-1;
  endIndex=-1;
  updateMetrics();

  // Configure the animator
  core->getDefaultGraphicEngine()->lockPathAnimators(__FILE__, __LINE__);
  animator.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),normalColor,normalColor,false,0);
  core->getDefaultGraphicEngine()->unlockPathAnimators();

  // Create a visualization object for each zoom level
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
  for(Int zoomLevel=1;zoomLevel<core->getMapSource()->getZoomLevelCount();zoomLevel++) {
    NavigationPathVisualization *visualization=new NavigationPathVisualization();
    if (!visualization) {
      FATAL("can not create navigation path visualization object",NULL);
    }
    visualization->setZoomLevel(zoomLevel);
    double latScale, lngScale;
    core->getMapSource()->getScales(zoomLevel,latScale,lngScale);
    visualization->setLatScale(latScale);
    visualization->setLngScale(lngScale);
    zoomLevelVisualizations.push_back(visualization);
    //DEBUG("z=%d latScale=%e lngScale=%e",zoomLevel,latScale,lngScale);
  }
  core->getMapSource()->unlockAccess();
}

// Indicates that textures and buffers have been cleared
void NavigationPath::destroyGraphic() {

  // Invalidate all zoom levels
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    (*i)->destroyGraphic();
  }

}

// Indicates that textures and buffers shall be created
void NavigationPath::createGraphic() {

  // Invalidate all zoom levels
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    (*i)->createGraphic();
  }

}

// Recreate the graphic objects to reduce the number of graphic point buffers
void NavigationPath::optimizeGraphic() {

  // Go through all zoom levels
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    (*i)->optimizeGraphic();
  }

}

// Updates the visualization of the given containers
void NavigationPath::addVisualization(std::list<MapContainer*> *mapContainers) {

  std::list<MapContainer*> mapContainersOfSameZoomLevel;
  Int zoomLevel=-1;

  // Go through all containers
  // The mapContainers list is sorted according to the zoom level
  std::list<MapContainer*>::iterator j=mapContainers->begin();
  bool more_entries=true;
  while(more_entries) {

    // New zoom level?
    if ((j==mapContainers->end())||(zoomLevel!=(*j)->getZoomLevelMap())) {
      if (zoomLevel!=-1) {

        // Process the so far collected containers
        MapPosition prevPoint=NavigationPath::getPathInterruptedPos();
        MapPosition prevArrowPoint;
        NavigationPathVisualization *visualization = zoomLevelVisualizations[zoomLevel-1];
        std::vector<MapPosition> *points = visualization->getPoints();
        for(Int i=0;i<points->size();i++) {

          // Handle path interrupted positions
          MapPosition p=(*points)[i];
          if ((p==NavigationPath::getPathInterruptedPos())||(prevPoint==NavigationPath::getPathInterruptedPos())) {
            prevArrowPoint=p;
            prevPoint=p;
          } else {

            // Add visualization
            updateTileVisualization(&mapContainersOfSameZoomLevel,visualization,prevPoint,prevArrowPoint,p);

            /// Update crossing tile segments
            updateCrossingTileSegments(&mapContainersOfSameZoomLevel, visualization, i);

            // Remember the last point
            prevPoint=p;
            if (p.getHasBearing())
              prevArrowPoint=p;
          }
        }

        // Add the visualization of the flags
        updateTileVisualization(NavigationPathVisualizationTypeStartFlag,&mapContainersOfSameZoomLevel,visualization,false,false);
        updateTileVisualization(NavigationPathVisualizationTypeEndFlag,&mapContainersOfSameZoomLevel,visualization,false,false);
      }

      // Create a new empty list
      mapContainersOfSameZoomLevel.clear();
    }

    // Add the current container to the list
    if (j==mapContainers->end()) {
      more_entries=false;
    } else {
      mapContainersOfSameZoomLevel.push_back(*j);
      zoomLevel=(*j)->getZoomLevelMap();
      j++;
    }
  }
}

// Removes the visualization of the given container
void NavigationPath::removeVisualization(MapContainer* mapContainer) {

  std::list<MapContainer*> mapContainersOfSameZoomLevel;
  Int zoomLevel=-1;

  // Remove the tile from the visualization
  NavigationPathVisualization *visualization=zoomLevelVisualizations[mapContainer->getZoomLevelMap()-1];
  std::vector<MapTile*> *mapTiles=mapContainer->getMapTiles();
  for(std::vector<MapTile*>::iterator i=mapTiles->begin();i!=mapTiles->end();i++) {
    visualization->removeTileInfo(*i);
    (*i)->removeCrossingNavigationPathSegments(this);
  }
}

// Computes navigation details for the given location
void NavigationPath::computeNavigationInfo(MapPosition locationPos, MapPosition &wayPoint, NavigationInfo &navigationInfo) {

  // Lock the path until we have enough information
  lockAccess(__FILE__, __LINE__);

  // Do not calculate if path is not initialized
  if (!isInit) {
    unlockAccess();
    return;
  }

  // Init variables
  std::vector<MapPosition> mapPositions=getSelectedPoints();
  double distanceToRoute=std::numeric_limits<double>::max();
  std::vector<MapPosition>::iterator nearestIterator;
  Int startIndex, endIndex;
  startIndex=reverse ? mapPositions.size()-1 : 0;
  endIndex=reverse ? 0 : mapPositions.size()-1;

  // All code after this point is using local variables or read-only class members
  // We can safely unlock the general access to the path object
  unlockAccess();

  // Find the nearest point on the route
  for (Int i=startIndex; reverse?i>=endIndex:i<=endIndex;reverse?i--:i++) {
    MapPosition curPoint = mapPositions[i];
    double distance = curPoint.computeDistance(locationPos);
    if (distance<distanceToRoute)  {
      distanceToRoute=distance;
      nearestIterator=mapPositions.begin()+i;
    }
  }
  //DEBUG("minDistance=%.2f index=%d",minDistance,nearestIterator-mapPositions.begin());

  // Check if we are already on route
  bool offRoute=true;
  if (distanceToRoute<minDistanceToBeOffRoute)
    offRoute=false;

  // In case there are multiple points on the route,
  // take the one that lies closest to the bearing of the location
  bool firstPos=true;
  double routeBearing=0;
  double minBearingDiff=std::numeric_limits<double>::max();
  std::vector<MapPosition>::iterator iterator,prevIterator,newNearestIterator=nearestIterator;
  iterator=mapPositions.begin()+startIndex;
  while (true) {

    // Compute the current bearing of the route
    if (!firstPos) {
      routeBearing=(*prevIterator).computeBearing(*iterator);

      // If the route point is around the nearest found point,
      // remember the one that is closest to the location bearing
      double distance=(*iterator).computeNormalDistance(*prevIterator,locationPos,true);
      if (distance<minDistanceToBeOffRoute) {
        offRoute=false; // in case the nearest point is too far away due to route with minimized points
        double bearingDiff;
        if (locationPos.getHasBearing()) {
          bearingDiff=fabs(routeBearing-locationPos.getBearing());
          if (bearingDiff<minBearingDiff) {
            newNearestIterator=iterator;
            minBearingDiff=bearingDiff;
          }
        }
      }
    } else {
      firstPos=false;
    }

    // Select the next route point
    prevIterator=iterator;
    if (iterator==mapPositions.begin()+endIndex)
      break;
    if (reverse) {
      iterator--;
    } else {
      iterator++;
    }
  }
  nearestIterator=newNearestIterator;
  /*if (offRoute) {
    DEBUG("off route: minDistance=%.2f index=%d",minDistance,nearestIterator-mapPositions.begin());
  } else {
    DEBUG("on route:  minDistance=%.2f index=%d",minDistance,nearestIterator-mapPositions.begin());
  }
  core->getNavigationEngine()->setTargetAtGeographicCoordinate(nearestIterator->getLng(),nearestIterator->getLat(),false);
  GraphicPosition *visPos=core->getGraphicEngine()->lockPos(__FILE__, __LINE__);
  visPos->updateLastUserModification();
  core->getGraphicEngine()->unlockPos();*/

  // From the nearest point, find the point at the predefined distance
  bool wayPointSet = false;
  double distanceToRouteEnd=distanceToRoute;
  double turnAngle=NavigationInfo::getUnknownAngle();
  MapPosition pos;
  MapPosition lastValidPos=NavigationPath::getPathInterruptedPos();
  MapPosition prevPos=NavigationPath::getPathInterruptedPos();
  MapPosition bestTurnLookForwardPos;
  MapPosition turnPoint;
  iterator=std::vector<MapPosition>::iterator(nearestIterator);
  bool firstFrontPosFound = false;
  bool turnPointSet = false;
  bool prevPointWasTurnPoint = true;
  if (distanceToRoute!=std::numeric_limits<double>::max()) {
    while (true) {
      pos = *iterator;
      if (pos!=NavigationPath::getPathInterruptedPos()) {
        lastValidPos=pos;

        // Compute the distance
        if (prevPos!=NavigationPath::getPathInterruptedPos()) {
          distanceToRouteEnd+=prevPos.computeDistance(pos);
        }
        //core->getNavigationEngine()->setTargetAtGeographicCoordinate(pos.getLng(),pos.getLat(),false);

        // Ignore points that lie behind the current bearing for way point and turn point computation
        double bearing = locationPos.computeBearing(pos);
        if ((fabs(bearing-locationPos.getBearing())<90.0)||(firstFrontPosFound)) {

          // Determine the way point for target computation
          double distanceFromLocation = pos.computeDistance(locationPos);
          if ((!wayPointSet)&&(distanceFromLocation>minDistanceToRouteWayPoint)) {
            wayPoint=pos;
            wayPointSet=true;
          }

          // Update the look back and look forward points for turn detection
          std::vector<MapPosition>::iterator turnIterator=iterator;
          MapPosition turnLookBackPos=pos;
          MapPosition prevPos2=pos;
          double distance=0;
          while (true) {
            MapPosition pos2 = *turnIterator;
            if (pos2==NavigationPath::getPathInterruptedPos()) {
              break;
            } else {
              distance+=pos2.computeDistance(prevPos2);
              prevPos2=pos2;
              turnLookBackPos=pos2;
              if (distance>turnDetectionDistance) {
                break;
              }
            }
            if (turnIterator==mapPositions.begin()+startIndex)
              break;
            if (reverse) {
              turnIterator++;
            } else {
              turnIterator--;
            }
          }
          turnIterator=iterator;
          MapPosition turnLookForwardPos=pos;
          prevPos2=pos;
          distance=0;
          while (true) {
            MapPosition pos2 = *turnIterator;
            if (pos2==NavigationPath::getPathInterruptedPos()) {
              break;
            } else {
              distance+=pos2.computeDistance(prevPos2);
              prevPos2=pos2;
              turnLookForwardPos=pos2;
              if (distance>turnDetectionDistance) {
                break;
              }
            }
            if (turnIterator==mapPositions.begin()+endIndex)
              break;
            if (reverse) {
              turnIterator--;
            } else {
              turnIterator++;
            }
          }

          // If we are off route and this is the first route point:
          // Mark it as a turn
          if ((offRoute)&&(!firstFrontPosFound)) {
            //DEBUG("marking first route point as turn",NULL);
            turnPointSet=true;
            turnPoint=pos;
            bestTurnLookForwardPos=turnLookForwardPos;
            prevPointWasTurnPoint=false;
          }
          firstFrontPosFound=true;

          // Turn detection
          double turnLookBackAngle=pos.computeBearing(turnLookBackPos);
          double turnLookForwardAngle=pos.computeBearing(turnLookForwardPos);
          double angle=turnLookForwardAngle-turnLookBackAngle;
          if (angle<0)
            angle+=360;
          if (angle>360)
            angle-=360;
          angle=180-angle;
          /*if ((!turnPointSet)||(prevPointWasTurnPoint)) {
            DEBUG("lookBackAngle=%f loockForwardAngle=%f angle=%f",turnLookBackAngle,turnLookForwardAngle,angle);
            core->getNavigationEngine()->setTargetAtGeographicCoordinate(pos.getLng(),pos.getLat(),false);
            sleep(1);
          }*/
          if (fabs(angle)>minTurnAngle) {
            if (prevPointWasTurnPoint) {
              //DEBUG("turn point candidate found: lat=%f, lng=%f, angle=%f",pos.getLat(),pos.getLng(),angle);
              if (!turnPointSet) {
                turnPoint=pos;
                bestTurnLookForwardPos=turnLookForwardPos;
                turnAngle=angle;
                //DEBUG("candidate set",NULL);
              } else {
                if (turnAngle<0) {
                  if ((angle<0)&&(angle<turnAngle)) {
                    turnPoint=pos;
                    bestTurnLookForwardPos=turnLookForwardPos;
                    turnAngle=angle;
                    //DEBUG("candidate set",NULL);
                  } else {
                    prevPointWasTurnPoint=false;
                  }
                } else {
                  if ((angle>0)&&(angle>turnAngle)) {
                    turnPoint=pos;
                    bestTurnLookForwardPos=turnLookForwardPos;
                    turnAngle=angle;
                    //DEBUG("candidate set",NULL);
                  } else {
                    prevPointWasTurnPoint=false;
                  }
                }
              }
              turnPointSet=true;
            }
          } else {
            if (turnPointSet) {
              prevPointWasTurnPoint=false;
            }
          }
        }
      }
      prevPos=pos;
      if (iterator==mapPositions.begin()+endIndex)
        break;
      if (reverse) {
        iterator--;
      } else {
        iterator++;
      }
    }
  }
  if (!wayPointSet) {
    if (lastValidPos!=NavigationPath::getPathInterruptedPos())
      wayPoint=pos;
    else
      wayPoint.invalidate();
  }
  double turnDistance=locationPos.computeDistance(turnPoint);
  if ((!turnPointSet)||(turnDistance>maxDistanceToTurnWayPoint)||(turnDistance<minDistanceToTurnWayPoint)) {
    turnPoint.invalidate();
    turnAngle=NavigationInfo::getUnknownAngle();
    turnDistance=NavigationInfo::getUnknownDistance();
  } else {
    double turnLookBackAngle=turnPoint.computeBearing(locationPos);
    double turnLookForwardAngle=turnPoint.computeBearing(bestTurnLookForwardPos);
    turnAngle=turnLookForwardAngle-turnLookBackAngle;
    if (turnAngle<0)
      turnAngle+=360;
    if (turnAngle>360)
      turnAngle-=360;
    turnAngle=180-turnAngle;
    /*if (turnAngle>0) {
      DEBUG("turn to the left in %f meters",distanceToTurnPoint);
    } else {
      DEBUG("turn to the right in %f meters",distanceToTurnPoint);
    }*/
  }

  // Set the navigation infos
  navigationInfo.setOffRoute(offRoute);
  if (offRoute)
    navigationInfo.setRouteDistance(distanceToRoute);
  navigationInfo.setTargetDistance(distanceToRouteEnd);
  navigationInfo.setTurnAngle(turnAngle);
  navigationInfo.setTurnDistance(turnDistance);
}

// Computes the metrics for the given map positions
void NavigationPath::updateMetrics(MapPosition prevPoint, MapPosition curPoint) {
  if ((prevPoint!=NavigationPath::getPathInterruptedPos())&&(curPoint!=NavigationPath::getPathInterruptedPos())) {
    length+=prevPoint.computeDistance(curPoint);
    if (lastValidAltiudeMetersPoint==NavigationPath::getPathInterruptedPos()) {
      lastValidAltiudeMetersPoint=prevPoint;
    }
    if (curPoint.getHasAltitude()) {

      // Check if the altitude difference is sane
      double altitudeDiff = curPoint.getAltitude() - lastValidAltiudeMetersPoint.getAltitude();
      if (fabs(altitudeDiff)>=minAltitudeChange) {

        // Update the altitude meters
        if (altitudeDiff>0) {
          altitudeUp+=altitudeDiff;
        } else {
          altitudeDown+=-altitudeDiff;
        }
        lastValidAltiudeMetersPoint=curPoint;
      }
    }
  }
  if (curPoint.getHasAltitude()) {
    if (curPoint.getAltitude()<minAltitude) {
      minAltitude=curPoint.getAltitude();
    }
    if (prevPoint.getAltitude()>maxAltitude) {
      maxAltitude=curPoint.getAltitude();
    }
  }
}

// Updates the metrics (altitude, length, duration, ...) of the path
void NavigationPath::updateMetrics() {
  Int startIndex, endIndex;
  MapPosition prevPos = NavigationPath::getPathInterruptedPos();
  startIndex=reverse ? mapPositions.size()-1 : 0;
  if (this->startIndex!=-1)
    startIndex=this->startIndex;
  endIndex=reverse ? 0 : mapPositions.size()-1;
  if (this->endIndex!=-1)
    endIndex=this->endIndex;
  if (reverse) {
    if (startIndex<mapPositions.size()-1)
      prevPos=mapPositions[startIndex+1];
  } else {
    if (startIndex>0)
      prevPos=mapPositions[startIndex-1];
  }
  length=0;
  altitudeUp=0;
  altitudeDown=0;
  minAltitude=std::numeric_limits<double>::max();
  maxAltitude=std::numeric_limits<double>::min();
  for(Int i=startIndex;reverse?i>endIndex:i<endIndex;reverse?i--:i++) {
    updateMetrics(prevPos,mapPositions[i]);
    prevPos=mapPositions[i];
  }
}

// Updates the flag visualization for all zoom levels
void NavigationPath::updateFlagVisualization(NavigationPathVisualizationType type, bool remove, bool animate) {
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    NavigationPathVisualization *visualization=*i;
    updateTileVisualization(type,NULL,visualization,remove,animate);
  }
}

// Sets the start flag at the given index
void NavigationPath::setStartFlag(Int index, const char *file, int line) {

  // Path may only be changed by one thread
  lockAccess(file,line);

  // Remove the current visualization of start and end flag
  if (startIndex!=-1)
    updateFlagVisualization(NavigationPathVisualizationTypeStartFlag,true,true);

  // Shall the start flag be hidden?
  if (index==-1) {
    startIndex=-1;
  } else {

    // First check if start flag is within range of path
    if (index<0)
      index=0;
    if (index>mapPositions.size()-1)
      index=mapPositions.size()-1;

    // If the new start flag is behind the end flag, set the end flag to the start flag
    bool updateEndFlag=false;
    Int newEndIndex=-1;
    startIndex=index;
    if (reverse) {
      if ((endIndex!=-1)&&(index<endIndex)) {
        updateEndFlag=true;
        newEndIndex=index;
      }
    } else {
      if ((endIndex!=-1)&&(index>endIndex)) {
        updateEndFlag=true;
        newEndIndex=index;
      }
    }

    // Update the visualization
    updateFlagVisualization(NavigationPathVisualizationTypeStartFlag,false,true);
    std::string routePath="Navigation/Route[@name='" + getGpxFilename() + "']";
    core->getConfigStore()->setIntValue(routePath,"startFlagIndex",startIndex, __FILE__, __LINE__);
    if (updateEndFlag) {
      updateFlagVisualization(NavigationPathVisualizationTypeEndFlag,true,true);
      endIndex=newEndIndex;
      updateFlagVisualization(NavigationPathVisualizationTypeEndFlag,false,true);
      core->getConfigStore()->setIntValue(routePath,"endFlagIndex",endIndex, __FILE__, __LINE__);
    }
  }

  // Update the metrics of the path
  updateMetrics();

  // Path can be used again
  unlockAccess();

  // Inform the widget engine
  core->onPathChange(this, NavigationPathChangeTypeFlagSet);
}

// Sets the end flag at the given index
void NavigationPath::setEndFlag(Int index, const char *file, int line) {

  // Path may only be changed by one thread
  lockAccess(file, line);

  // Remove the current visualization of start and end flag
  if (endIndex!=-1)
    updateFlagVisualization(NavigationPathVisualizationTypeEndFlag,true,true);

  // Shall the end flag be hidden?
  if (index==-1) {
    endIndex=-1;
  } else {

    // First check if end flag is within range of path
    if (index<0)
      index=0;
    if (index>mapPositions.size()-1)
      index=mapPositions.size()-1;

    // If the new end flag is behind the start flag, set the start flag to the end flag
    bool updateStartFlag=false;
    Int newStartIndex=-1;
    endIndex=index;
    if (reverse) {
      if ((startIndex!=-1)&&(index>startIndex)) {
        updateStartFlag=true;
        newStartIndex=index;
      }
    } else {
      if ((endIndex!=-1)&&(index<startIndex)) {
        updateStartFlag=true;
        newStartIndex=index;
      }
    }

    // Update the visualization
    updateFlagVisualization(NavigationPathVisualizationTypeEndFlag,false,true);
    std::string routePath="Navigation/Route[@name='" + getGpxFilename() + "']";
    core->getConfigStore()->setIntValue(routePath,"endFlagIndex",endIndex, __FILE__, __LINE__);
    if (updateStartFlag) {
      updateFlagVisualization(NavigationPathVisualizationTypeStartFlag,true,true);
      startIndex=newStartIndex;
      updateFlagVisualization(NavigationPathVisualizationTypeStartFlag,false,true);
      core->getConfigStore()->setIntValue(routePath,"startFlagIndex",startIndex, __FILE__, __LINE__);
    }
  }

  // Update the metrics of the path
  updateMetrics();

  // Path can be used again
  unlockAccess();

  // Inform the widget engine
  core->onPathChange(this, NavigationPathChangeTypeFlagSet);
}

// Returns the points selected by the flags
std::vector<MapPosition> NavigationPath::getSelectedPoints() {
  //DEBUG("startIndex=%d endIndex=%d size=%d reverse=%d",startIndex,endIndex,mapPositions.size(),reverse);
  if ((startIndex==-1)&&(endIndex==-1))
    return mapPositions;
  else {
    Int startIndex;
    Int endIndex;
    if (reverse) {
      endIndex=0;
      startIndex=mapPositions.size()-1;
    } else {
      startIndex=0;
      endIndex=mapPositions.size()-1;
    }
    if (this->startIndex!=-1)
      startIndex=this->startIndex;
    if (this->endIndex!=-1)
      endIndex=this->endIndex;
    std::vector<MapPosition>::iterator startIterator;
    std::vector<MapPosition>::iterator endIterator;
    if (reverse) {
      startIterator = mapPositions.begin()+endIndex;
      endIterator = mapPositions.begin()+startIndex+1;
    } else {
      startIterator = mapPositions.begin()+startIndex;
      endIterator = mapPositions.begin()+endIndex+1;
    }
    return std::vector<MapPosition>(startIterator,endIterator);
  }
}

}
