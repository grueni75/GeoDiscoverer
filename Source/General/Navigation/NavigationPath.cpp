//============================================================================
// Name        : NavigationPath.cpp
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


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
NavigationPath::NavigationPath() {

  // Init variables
  accessMutex=core->getThread()->createMutex();
  isInitMutex=core->getThread()->createMutex();
  gpxFilefolder=core->getNavigationEngine()->getTrackPath();
  pathMinSegmentLength=core->getConfigStore()->getIntValue("Graphic","pathMinSegmentLength");
  pathMinDirectionDistance=core->getConfigStore()->getIntValue("Graphic","pathMinDirectionDistance");
  pathWidth=core->getConfigStore()->getIntValue("Graphic","pathWidth");
  minDistanceToRouteWayPoint=core->getConfigStore()->getIntValue("Navigation","minDistanceToRouteWayPoint");
  minTurnAngle=core->getConfigStore()->getIntValue("Navigation","minTurnAngle");
  maxDistanceToTurnWayPoint=core->getConfigStore()->getIntValue("Navigation","maxDistanceToTurnWayPoint");
  turnDetectionDistance=core->getConfigStore()->getIntValue("Navigation","turnDetectionDistance");
  isInit=false;
  reverse=false;

  // Do the dynamic initialization
  init();

  // Add the animator to the graphic engine
  GraphicObject *pathAnimators=core->getGraphicEngine()->lockPathAnimators();
  animatorKey=pathAnimators->addPrimitive(&animator);
  core->getGraphicEngine()->unlockPathAnimators();
}

// Destructor
NavigationPath::~NavigationPath() {

  // Deinit everything
  deinit();

  // Remove the animator
  GraphicObject *pathAnimators=core->getGraphicEngine()->lockPathAnimators();
  pathAnimators->removePrimitive(animatorKey,false);
  core->getGraphicEngine()->unlockPathAnimators();

  // Free variables
  core->getThread()->destroyMutex(accessMutex);
  core->getThread()->destroyMutex(isInitMutex);
}

// Updates the visualization of the tile
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
  core->getMapSource()->lockAccess();
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
        GraphicLine *line=info->getGraphicLine();
        GraphicObject *tileVisualization=(*k)->getVisualization();
        tileVisualization->lockAccess();
        if (!line) {

          // Create a new line and add it to the tile
          line=new GraphicLine(0,pathWidth);
          if (!line) {
            FATAL("can not create graphic line object",NULL);
            return;
          }
          line->setAnimator(&animator);
          line->setZ(1); // ensure that line is drawn after tile texture
          line->setCutEnabled(true);
          line->setCutWidth((*k)->getWidth());
          line->setCutHeight((*k)->getHeight());
          info->setGraphicLine(line);
          info->setGraphicLineKey(tileVisualization->addPrimitive(line));

        }

        // Add the stroke to the line
        Int x1=prevPos.getX()-(*k)->getMapX(0);
        Int y1=(*k)->getHeight()-(prevPos.getY()-(*k)->getMapY(0));
        Int x2=currentPos.getX()-(*k)->getMapX(0);
        Int y2=(*k)->getHeight()-(currentPos.getY()-(*k)->getMapY(0));
        line->addStroke(x1,y1,x2,y2);

        // Add arrow if necessary
        if (currentPos.getHasBearing()) {

          // Get the existing rectangle list or create a new one
          GraphicRectangleList *rectangleList=info->getGraphicRectangeList();
          if (!rectangleList) {

            // Create a new rectangle list and add it to the tile
            rectangleList=new GraphicRectangleList(0);
            if (!rectangleList) {
              FATAL("can not create graphic rectangle list object",NULL);
              return;
            }
            rectangleList->setAnimator(&animator);
            rectangleList->setZ(2); // ensure that rectangle is drawn after line texture
            rectangleList->setTexture(core->getGraphicEngine()->getPathDirectionIcon()->getTexture());
            rectangleList->setDestroyTexture(false);
            rectangleList->setCutEnabled(true);
            rectangleList->setCutWidth((*k)->getWidth());
            rectangleList->setCutHeight((*k)->getHeight());
            Int arrowWidth=core->getGraphicEngine()->getPathDirectionIcon()->getWidth();
            Int arrowHeight=core->getGraphicEngine()->getPathDirectionIcon()->getHeight();
            Int arrowXToCenter=arrowWidth/2-core->getGraphicEngine()->getPathDirectionIcon()->getIconWidth()/2;
            Int arrowYToCenter=arrowHeight/2-core->getGraphicEngine()->getPathDirectionIcon()->getIconHeight()/2;
            double totalRadius=sqrt((double)(arrowWidth*arrowWidth/4+arrowHeight*arrowHeight/4));
            double distanceToCenter=sqrt((double)(arrowXToCenter*arrowXToCenter+arrowYToCenter*arrowYToCenter));
            double angleToCenter;
            if (arrowXToCenter==0) {
              angleToCenter=M_PI/2;
            } else {
              angleToCenter=atan((double)arrowYToCenter/(double)arrowXToCenter);
            }
            rectangleList->setParameter(totalRadius,distanceToCenter,angleToCenter);
            info->setGraphicRectangleList(rectangleList);
            info->setGraphicRectangleListKey(tileVisualization->addPrimitive(rectangleList));
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
          rectangleList->addRectangle(x,y,angle);

        }
        tileVisualization->unlockAccess();

      }
    }
  }
  core->getMapSource()->unlockAccess();
}

// Adds a point to the path
void NavigationPath::addEndPosition(MapPosition pos) {

  // Ensure that only one thread is executing this code
  core->getThread()->lockMutex(accessMutex);

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

  // Update the length
  if ((hasSecondLastPoint)&&(hasLastPoint)) {
    if ((secondLastPoint!=NavigationPath::getPathInterruptedPos())&&(lastPoint!=NavigationPath::getPathInterruptedPos())) {
      length+=secondLastPoint.computeDistance(lastPoint);
    }
  }

  // Path was modified
  hasChanged=true;
  isStored=false;

  // Update the visualization for each zoom level
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    NavigationPathVisualization *visualization=*i;

    // Get the calibrator for this point
    core->getMapSource()->lockAccess();
    bool deleteCalibrator=false;
    MapCalibrator *calibrator=NULL;
    if (pos!=NavigationPath::getPathInterruptedPos())
      calibrator=visualization->findCalibrator(pos,deleteCalibrator);

    // Just add this point if the prev point or the current point interrupted the line
    if ((visualization->getPrevLinePoint()==NavigationPath::getPathInterruptedPos())||(pos==NavigationPath::getPathInterruptedPos())) {
      if ((pos==NavigationPath::getPathInterruptedPos())||(calibrator!=NULL)) {
        visualization->addPoint(pos);
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

          // Update the previous point
          visualization->addPoint(pos);

        } else {
          core->getMapSource()->unlockAccess();
        }
      }
    }

    // Delete the calibrator if required
    if ((deleteCalibrator)&&(calibrator))
      delete calibrator;

  }

  // Unlock the mutex
  core->getThread()->unlockMutex(accessMutex);

}

// Clears the graphical representation
void NavigationPath::deinit() {

  // Delete the zoom level visualization
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    delete *i;
  }
  zoomLevelVisualizations.clear();

  // Force a redraw
  GraphicObject *pathAnimators=core->getGraphicEngine()->lockPathAnimators();
  pathAnimators->setIsUpdated(true);
  core->getGraphicEngine()->unlockPathAnimators();

  // Is not initialized
  setIsInit(false);
}

// Clears all points and sets a new gpx filname
void NavigationPath::init() {

  // Reset variables
  hasLastPoint=false;
  hasSecondLastPoint=false;
  gpxFilename="track-" + core->getClock()->getFormattedDate() + ".gpx";
  setName("Track-" + core->getClock()->getFormattedDate());
  setDescription("Track recorded with GeoDiscoverer.");
  hasChanged=true;
  isStored=true;
  hasBeenLoaded=false;
  isNew=true;
  mapPositions.clear();
  length=0;
  blinkMode=false;

  // Configure the animator
  core->getGraphicEngine()->lockPathAnimators();
  animator.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),normalColor,normalColor,false,0);
  core->getGraphicEngine()->unlockPathAnimators();

  // Create a visualization object for each zoom level
  core->getMapSource()->lockAccess();
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
  core->getThread()->lockMutex(accessMutex);
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    (*i)->destroyGraphic();
  }
  core->getThread()->unlockMutex(accessMutex);

}

// Indicates that textures and buffers shall be created
void NavigationPath::createGraphic() {

  // Invalidate all zoom levels
  core->getThread()->lockMutex(accessMutex);
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    (*i)->createGraphic();
  }
  core->getThread()->unlockMutex(accessMutex);

}

// Recreate the graphic objects to reduce the number of graphic point buffers
void NavigationPath::optimizeGraphic() {

  // Go through all zoom levels
  core->getThread()->lockMutex(accessMutex);
  for(std::vector<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    (*i)->optimizeGraphic();
  }
  core->getThread()->unlockMutex(accessMutex);

}

// Updates the visualization of the given containers
void NavigationPath::addVisualization(std::list<MapContainer*> *mapContainers) {

  std::list<MapContainer*> mapContainersOfSameZoomLevel;
  Int zoomLevel=-1;

  // Go through all containers
  // The mapContainers list is sorted according to the zoom level
  core->getThread()->lockMutex(accessMutex);
  std::list<MapContainer*>::iterator j=mapContainers->begin();
  bool more_entries=true;
  while(more_entries) {

    // New zoom level?
    if ((j==mapContainers->end())||(zoomLevel!=(*j)->getZoomLevel())) {
      if (zoomLevel!=-1) {

        // Process the so far collected containers
        MapPosition prevPoint=NavigationPath::getPathInterruptedPos();
        MapPosition prevArrowPoint;
        NavigationPathVisualization *visualization = zoomLevelVisualizations[zoomLevel-1];
        std::list<MapPosition> *points = visualization->getPoints();
        for(std::list<MapPosition>::iterator i=points->begin();i!=points->end();i++) {

          // Handle path interrupted positions
          if ((*i==NavigationPath::getPathInterruptedPos())||(prevPoint==NavigationPath::getPathInterruptedPos())) {
            prevArrowPoint=*i;
            prevPoint=*i;
          } else {

            // Add visualization
            updateTileVisualization(&mapContainersOfSameZoomLevel,visualization,prevPoint,prevArrowPoint,*i);

            // Remember the last point
            prevPoint=*i;
            if ((*i).getHasBearing())
              prevArrowPoint=*i;
          }
        }
      }

      // Create a new empty list
      mapContainersOfSameZoomLevel.clear();
    }

    // Add the current container to the list
    if (j==mapContainers->end()) {
      more_entries=false;
    } else {
      mapContainersOfSameZoomLevel.push_back(*j);
      zoomLevel=(*j)->getZoomLevel();
      j++;
    }
  }
  core->getThread()->unlockMutex(accessMutex);
}

// Removes the visualization of the given container
void NavigationPath::removeVisualization(MapContainer* mapContainer) {

  std::list<MapContainer*> mapContainersOfSameZoomLevel;
  Int zoomLevel=-1;

  // Remove the tile from the visualization
  core->getThread()->lockMutex(accessMutex);
  NavigationPathVisualization *visualization=zoomLevelVisualizations[mapContainer->getZoomLevel()-1];
  std::vector<MapTile*> *mapTiles=mapContainer->getMapTiles();
  for(std::vector<MapTile*>::iterator i=mapTiles->begin();i!=mapTiles->end();i++) {
    visualization->removeTileInfo(*i);
  }
  core->getThread()->unlockMutex(accessMutex);

}

// Computes navigation details for the given location
void NavigationPath::computeNavigationInfos(MapPosition locationPos, MapPosition &wayPoint, MapPosition &turnPoint, double &distanceToRouteEnd) {

  // Ensure that only one thread is executing this code
  core->getThread()->lockMutex(accessMutex);

  // Do not calculate if path is not initialized
  if (!isInit) {
    core->getThread()->unlockMutex(accessMutex);
    return;
  }

  // Find the nearest point on the route
  double minDistance=std::numeric_limits<double>::max();
  std::list<MapPosition>::iterator nearestIterator;
  for (std::list<MapPosition>::iterator i=mapPositions.begin();i!=mapPositions.end();i++) {
    double distance = (*i).computeDistance(locationPos);
    if (distance<minDistance)  {
      minDistance=distance;
      nearestIterator=i;
    }
  }

  // From the nearest point, find the point at the predefined distance
  bool wayPointSet = false;
  distanceToRouteEnd=minDistance;
  MapPosition pos;
  MapPosition lastValidPos=NavigationPath::getPathInterruptedPos();
  MapPosition prevPos=NavigationPath::getPathInterruptedPos();
  std::list<MapPosition>::iterator iterator=std::list<MapPosition>::iterator(nearestIterator);
  bool turnPointSet = false;
  bool prevPointWasTurnPoint = true;
  double turnAngle;
  while (true) {
    pos = *iterator;
    if (pos!=NavigationPath::getPathInterruptedPos()) {
      lastValidPos=pos;

      // Determine the way point for target computation
      double distanceFromLocation = pos.computeDistance(locationPos);
      if ((!wayPointSet)&&(distanceFromLocation>minDistanceToRouteWayPoint)) {
        wayPoint=pos;
        wayPointSet=true;
      }

      // Compute the distance
      if (prevPos!=NavigationPath::getPathInterruptedPos()) {
        distanceToRouteEnd+=prevPos.computeDistance(pos);
      }
      //core->getNavigationEngine()->setTargetAtGeographicCoordinate(pos.getLng(),pos.getLat(),false);

      // Update the look back and look forward points for turn detection
      std::list<MapPosition>::iterator turnIterator=iterator;
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
        if (reverse) {
          turnIterator++;
          if (turnIterator==mapPositions.end())
            break;
        } else {
          if (turnIterator==mapPositions.begin())
            break;
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
        if (reverse) {
          if (turnIterator==mapPositions.begin())
            break;
          turnIterator--;
        } else {
          turnIterator++;
          if (turnIterator==mapPositions.end())
            break;
        }
      }

      // Turn detection
      double turnLookBackAngle=pos.computeBearing(turnLookBackPos);
      double turnLookForwardAngle=pos.computeBearing(turnLookForwardPos);
      double angle=turnLookForwardAngle-turnLookBackAngle;
      if (angle<0)
        angle+=360;
      angle=180-angle;
      /*if ((!turnPointSet)||(prevPointWasTurnPoint)) {
        DEBUG("lookBackAngle=%f loockForwardAngle=%f angle=%f",turnLookBackAngle,turnLookForwardAngle,angle);
        core->getThread()->unlockMutex(accessMutex);
        core->getNavigationEngine()->setTargetAtGeographicCoordinate(pos.getLng(),pos.getLat(),false);
        sleep(1);
        core->getThread()->lockMutex(accessMutex);
      }*/
      if (fabs(angle)>minTurnAngle) {
        if (prevPointWasTurnPoint) {
          //DEBUG("turn point candidate found: lat=%f, lng=%f, angle=%f",pos.getLat(),pos.getLng(),angle);
          if (!turnPointSet) {
            turnPoint=pos;
            turnAngle=angle;
            //DEBUG("candidate set",NULL);
          } else {
            if (turnAngle<0) {
              if ((angle<0)&&(angle<turnAngle)) {
                turnPoint=pos;
                turnAngle=angle;
                //DEBUG("candidate set",NULL);
              } else {
                prevPointWasTurnPoint=false;
              }
            } else {
              if ((angle>0)&&(angle>turnAngle)) {
                turnPoint=pos;
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
    prevPos=pos;
    if (reverse) {
      if (iterator==mapPositions.begin())
        break;
      iterator--;
    } else {
      iterator++;
      if (iterator==mapPositions.end())
        break;
    }
  }
  if (!wayPointSet) {
    if (lastValidPos!=NavigationPath::getPathInterruptedPos())
      wayPoint=pos;
    else
      wayPoint.invalidate();
  }
  if ((!turnPointSet)||(locationPos.computeDistance(turnPoint)>maxDistanceToTurnWayPoint)) {
  //if ((!turnPointSet)) {
    turnPoint.invalidate();
    turnAngle=360;
  } else {
    /*if (turnAngle>0) {
      DEBUG("turn to the left in %f meters",distanceToTurnPoint);
    } else {
      DEBUG("turn to the right in %f meters",distanceToTurnPoint);
    }*/
  }

  // That's it!
  core->getThread()->unlockMutex(accessMutex);
}

}
