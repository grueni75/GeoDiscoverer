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
  gpxFilefolder=core->getNavigationEngine()->getTrackPath();
  pathMinSegmentLength=core->getConfigStore()->getIntValue("Graphic","pathMinSegmentLength","Minimum segment length of tracks, routes and other paths",8);
  pathMinDirectionDistance=core->getConfigStore()->getIntValue("Graphic","pathMinDirectionDistance","Minimum distance between two direction arrows of tracks, routes and other paths",64);
  pathWidth=core->getConfigStore()->getIntValue("Graphic","pathWidth","Width of tracks, routes and other paths",8);

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
}

// Adds a point to the path
void NavigationPath::addEndPosition(MapPosition pos) {

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
  for(std::list<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    NavigationPathVisualization *visualization=*i;

    // Just add this point if the prev point or the current point interrupted the line
    if ((visualization->getPrevLinePoint()==NavigationPath::getPathInterruptedPos())||(pos==NavigationPath::getPathInterruptedPos())) {
      visualization->setPrevLinePoint(pos);
      visualization->setPrevArrowPoint(pos);
    } else {

      // Check if the point is far enough away from the previous point for this zoom level
      double length=visualization->computePixelDistance(visualization->getPrevLinePoint(),pos);
      if (length>=pathMinSegmentLength) {

        // Check if an arrow needs to be added
        bool addArrow=false;
        double arrowDistance=visualization->computePixelDistance(visualization->getPrevArrowPoint(),pos);
        if (arrowDistance>=pathMinDirectionDistance) {
          addArrow=true;
        }

        // Compute the search boundary
        double lngEast,lngWest;
        lngEast=pos.getLng();
        if (visualization->getPrevLinePoint().getLng()>lngEast) {
          lngWest=lngEast;
          lngEast=visualization->getPrevLinePoint().getLng();
        } else {
          lngWest=visualization->getPrevLinePoint().getLng();
        }
        double latNorth,latSouth;
        latNorth=pos.getLat();
        if (visualization->getPrevLinePoint().getLat()>latNorth) {
          latSouth=latNorth;
          latNorth=visualization->getPrevLinePoint().getLat();
        } else {
          latSouth=visualization->getPrevLinePoint().getLat();
        }

        // Find all map tiles that overlap with this segment
        MapArea area;
        area.setZoomLevel(visualization->getZoomLevel());
        double latExtend=pathWidth/visualization->getLatScale();
        double lngExtend=pathWidth/visualization->getLngScale();
        area.setLatNorth(latNorth+latExtend);
        area.setLatSouth(latSouth-latExtend);
        area.setLngWest(lngWest-lngExtend);
        area.setLngEast(lngEast+lngExtend);
        std::list<MapContainer*> mapContainers=core->getMapSource()->findMapContainersByGeographicArea(area);

        // Go through all of them
        for(std::list<MapContainer*>::iterator j=mapContainers.begin();j!=mapContainers.end();j++) {

          // Compute the coordinates of the segment within the container
          MapContainer *mapContainer=*j;
          MapPosition prevPos=visualization->getPrevLinePoint();
          MapPosition prevArrowPos=visualization->getPrevArrowPoint();
          MapPosition currentPos=pos;
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
              MapTile *mapTile=*k;
              NavigationPathTileInfo *info=visualization->findTileInfo(mapTile);
              GraphicLine *line=info->getGraphicLine();
              GraphicObject *tileVisualization=mapTile->getVisualization();
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
                line->setCutWidth(mapTile->getWidth());
                line->setCutHeight(mapTile->getHeight());
                info->setGraphicLine(line);
                info->setGraphicLineKey(tileVisualization->addPrimitive(line));

              }

              // Add the stroke to the line
              Int x1=prevPos.getX()-mapTile->getMapX(0);
              Int y1=mapTile->getHeight()-(prevPos.getY()-mapTile->getMapY(0));
              Int x2=currentPos.getX()-mapTile->getMapX(0);
              Int y2=mapTile->getHeight()-(currentPos.getY()-mapTile->getMapY(0));
              line->addStroke(x1,y1,x2,y2);

              // Add arrow if necessary
              if (addArrow) {

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
                  rectangleList->setCutWidth(mapTile->getWidth());
                  rectangleList->setCutHeight(mapTile->getHeight());
                  Int arrowWidth=core->getGraphicEngine()->getPathDirectionIcon()->getWidth();
                  Int arrowHeight=core->getGraphicEngine()->getPathDirectionIcon()->getHeight();
                  rectangleList->setRadius(sqrt((double)(arrowWidth*arrowWidth/4+arrowHeight*arrowHeight/4)));
                  info->setGraphicRectangleList(rectangleList);
                  info->setGraphicRectangleListKey(tileVisualization->addPrimitive(rectangleList));
                }

                // Add the arrow
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

        // Update the previous point
        visualization->setPrevLinePoint(pos);
        if (addArrow)
          visualization->setPrevArrowPoint(pos);

      }
    }
  }
}

// Clears the graphical representation
void NavigationPath::deinit() {

  // Delete the zoom level visualization
  for(std::list<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    delete *i;
  }
  zoomLevelVisualizations.clear();

  // Force a redraw
  GraphicObject *pathAnimators=core->getGraphicEngine()->lockPathAnimators();
  pathAnimators->setIsUpdated(true);
  core->getGraphicEngine()->unlockPathAnimators();
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
  animator.setColor(normalColor);
  animator.setBlinkAnimation(blinkMode,highlightColor);

  // Create a visualization object for each zoom level
  std::vector<MapContainerTreeNode*> *zoomLevelSearchTrees=core->getMapSource()->getZoomLevelSearchTrees();
  for(Int zoomLevel=1;zoomLevel<zoomLevelSearchTrees->size();zoomLevel++) {
    NavigationPathVisualization *visualization=new NavigationPathVisualization();
    MapContainerTreeNode *searchTree=(*zoomLevelSearchTrees)[zoomLevel];
    if (!visualization) {
      FATAL("can not create navigation path visualization object",NULL);
    }
    visualization->setZoomLevel(zoomLevel);
    visualization->setLatScale(searchTree->getContents()->getLatScale());
    visualization->setLngScale(searchTree->getContents()->getLngScale());
    zoomLevelVisualizations.push_back(visualization);
  }

}

// Indicates that textures and buffers have been invalidated
void NavigationPath::graphicInvalidated() {

  // Invalidate all zoom levels
  for(std::list<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    (*i)->graphicInvalidated();
  }

}

// Recreate the graphic objects to reduce the number of graphic point buffers
void NavigationPath::optimizeGraphic() {

  // Go through all zoom levels
  for(std::list<NavigationPathVisualization*>::iterator i=zoomLevelVisualizations.begin();i!=zoomLevelVisualizations.end();i++) {
    (*i)->optimizeGraphic();
  }

}

}
