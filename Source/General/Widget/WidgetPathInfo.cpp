//============================================================================
// Name        : WidgetPathInfo.cpp
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

// Static variables
std::string WidgetPathInfo::currentPathName = "";
bool WidgetPathInfo::currentPathLocked = false;

// Constructor
WidgetPathInfo::WidgetPathInfo() : WidgetPrimitive(), altitudeProfileAxisPointBuffer(8*6) {
  widgetType=WidgetTypePathInfo;
  pathNameFontString=NULL;
  pathLengthFontString=NULL;
  pathAltitudeUpFontString=NULL;
  pathAltitudeDownFontString=NULL;
  pathDurationFontString=NULL;
  currentPath=NULL;
  altitudeProfileFillPointBuffer=NULL;
  altitudeProfileLinePointBuffer=NULL;
  noAltitudeProfileFontString=NULL;
  averageTravelSpeed=core->getConfigStore()->getDoubleValue("Navigation","averageTravelSpeed");
  altitudeProfileXTickFontStrings=NULL;
  altitudeProfileYTickFontStrings=NULL;
  redrawRequired=false;
  startIndex=0;
  endIndex=0;
  maxEndIndex=0;
  hideLocationIcon=true;
  firstTouchDown=true;
  prevX=0;
  currentPathName=core->getConfigStore()->getStringValue("Navigation","pathInfoName");
  currentPathLocked=core->getConfigStore()->getIntValue("Navigation","pathInfoLocked");
}

// Destructor
WidgetPathInfo::~WidgetPathInfo() {
  core->getFontEngine()->lockFont("sansNormal");
  if (pathNameFontString) core->getFontEngine()->destroyString(pathNameFontString);
  if (pathLengthFontString) core->getFontEngine()->destroyString(pathLengthFontString);
  if (pathAltitudeUpFontString) core->getFontEngine()->destroyString(pathAltitudeUpFontString);
  if (pathAltitudeDownFontString) core->getFontEngine()->destroyString(pathAltitudeDownFontString);
  if (pathDurationFontString) core->getFontEngine()->destroyString(pathDurationFontString);
  core->getFontEngine()->unlockFont();
  if (altitudeProfileFillPointBuffer) delete altitudeProfileFillPointBuffer;
  if (altitudeProfileLinePointBuffer) delete altitudeProfileLinePointBuffer;
  core->getFontEngine()->lockFont("sansTiny");
  if (altitudeProfileXTickFontStrings) {
    for (Int i=0;i<altitudeProfileXTickCount;i++)
      if (altitudeProfileXTickFontStrings[i]) core->getFontEngine()->destroyString(altitudeProfileXTickFontStrings[i]);
    free(altitudeProfileXTickFontStrings);
  }
  if (altitudeProfileYTickFontStrings) {
    for (Int i=0;i<altitudeProfileYTickCount;i++)
      if (altitudeProfileYTickFontStrings[i]) core->getFontEngine()->destroyString(altitudeProfileYTickFontStrings[i]);
    free(altitudeProfileYTickFontStrings);
  }
  core->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetPathInfo::work(TimestampInMicroseconds t) {

  FontEngine *fontEngine=core->getFontEngine();
  Int textX, textY;
  std::list<std::string> status;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Redraw the widget if required
  if ((redrawRequired)&&(currentPath)) {

    // Get infos from path
    currentPath->lockAccess();
    std::string pathName=currentPath->getName();
    double pathLength=currentPath->getLength();
    double pathAltitudeUp=currentPath->getAltitudeUp();
    double pathAltitudeDown=currentPath->getAltitudeDown();
    std::vector<MapPosition> pathPoints=currentPath->getPoints();
    //DEBUG("pathPoints.size()=%d",pathPoints.size());
    bool pathReversed=currentPath->getReverse();
    double pathMinAltitude = currentPath->getMinAltitude();
    double pathMaxAltitude = currentPath->getMaxAltitude();
    currentPath->unlockAccess();

    // Update the labels
    FontEngine *fontEngine=core->getFontEngine();
    fontEngine->lockFont("sansNormal");
    fontEngine->updateString(&pathNameFontString,pathName,pathNameWidth);
    pathNameFontString->setX(x+pathNameOffsetX);
    pathNameFontString->setY(y+pathNameOffsetY);
    std::string value="";
    std::string unit="";
    core->getUnitConverter()->formatMeters(pathLength,value,unit,1);
    fontEngine->updateString(&pathLengthFontString,value + " " + unit,pathValuesWidth);
    core->getUnitConverter()->formatMeters(pathAltitudeUp,value,unit,1);
    fontEngine->updateString(&pathAltitudeUpFontString,value + " " + unit,pathValuesWidth);
    core->getUnitConverter()->formatMeters(pathAltitudeDown,value,unit,1);
    fontEngine->updateString(&pathAltitudeDownFontString,value + " " + unit,pathValuesWidth);
    core->getUnitConverter()->formatTime(pathLength/averageTravelSpeed,value,unit,1);
    fontEngine->updateString(&pathDurationFontString,value + " " + unit,pathValuesWidth);
    core->getFontEngine()->unlockFont();
    Int maxWidth=pathLengthFontString->getIconWidth();
    if (pathAltitudeUpFontString->getIconWidth()>maxWidth) maxWidth=pathAltitudeUpFontString->getIconWidth();
    if (pathAltitudeDownFontString->getIconWidth()>maxWidth) maxWidth=pathAltitudeDownFontString->getIconWidth();
    if (pathDurationFontString->getIconWidth()>maxWidth) maxWidth=pathDurationFontString->getIconWidth();
    //maxWidth=pathValuesWidth;
    pathLengthFontString->setX(x+pathValuesOffsetX+(pathValuesWidth-maxWidth)/2);
    pathLengthFontString->setY(y+pathLengthOffsetY);
    pathAltitudeUpFontString->setX(x+pathValuesOffsetX+(pathValuesWidth-maxWidth)/2);
    pathAltitudeUpFontString->setY(y+pathAltitudeUpOffsetY);
    pathAltitudeDownFontString->setX(x+pathValuesOffsetX+(pathValuesWidth-maxWidth)/2);
    pathAltitudeDownFontString->setY(y+pathAltitudeDownOffsetY);
    pathDurationFontString->setX(x+pathValuesOffsetX+(pathValuesWidth-maxWidth)/2);
    pathDurationFontString->setY(y+pathDurationOffsetY);

    // Compute the altitude profile
    double minDistance=std::numeric_limits<double>::max();
    bool noAltitudeProfile=true;
    if ((pathMinAltitude<=pathMaxAltitude)&&(pathPoints.size()>=2)) {
      std::list<GraphicPoint> altitudeProfileFillPoints;
      std::list<GraphicPoint> altitudeProfileLinePoints;
      MapPosition prevPos;
      double visiblePathLength=0;
      double visiblePathMinAltitude=std::numeric_limits<double>::max();
      double visiblePathMaxAltitude=std::numeric_limits<double>::min();
      double visiblePathMaxDistance=0;
      bool firstRound=true;
      for(Int i=pathReversed?endIndex+1:startIndex-1;pathReversed?i>=startIndex:i<=endIndex;pathReversed?i--:i++) {
        MapPosition curPos = pathPoints[i];
        if (!firstRound)
          visiblePathMaxDistance += curPos.computeDistance(prevPos);
        if (pathReversed?i<=endIndex:i>=startIndex) {
          visiblePathLength += curPos.computeDistance(prevPos);
        }
        if (curPos.getHasAltitude()) {
          if (curPos.getAltitude()<visiblePathMinAltitude)
            visiblePathMinAltitude=curPos.getAltitude();
          if (curPos.getAltitude()>visiblePathMaxAltitude)
            visiblePathMaxAltitude=curPos.getAltitude();
        }
        prevPos = curPos;
        firstRound = false;
      }
      double pixelPerLen = ((double)altitudeProfileWidth)/visiblePathLength;
      double pixelPerHeight;
      double altitudeDiff;
      if ((visiblePathMaxAltitude-visiblePathMinAltitude)<altitudeProfileMinAltitudeDiff) {
        altitudeDiff = altitudeProfileMinAltitudeDiff;
      } else {
        altitudeDiff = visiblePathMaxAltitude-visiblePathMinAltitude;
      }
      pixelPerHeight = ((double)altitudeProfileHeight)/altitudeDiff;
      //DEBUG("pixelPerHeight=%e visiblePathMaxDistance=%f visiblePathMaxAltitude=%f visiblePathMinAltitude=%f altitudeProfileMinAltitudeDiff=%f altitudeDiff=%f",pixelPerHeight,visiblePathMaxDistance,visiblePathMaxAltitude,visiblePathMinAltitude,altitudeProfileMinAltitudeDiff,altitudeDiff);
      if ((pixelPerLen!=std::numeric_limits<double>::infinity())&&(pixelPerHeight!=std::numeric_limits<double>::infinity())) {

        // Compute the profile points from the path positions
        Int prevX=0,curX=0;
        Int prevY=0,curY=0;
        Int prevXU=0, prevXU2=0, prevXD=0, prevXD2=0, curXU=0, curXD=0;
        Int prevYU=0, prevYU2=0, prevYD=0, prevYD2=0, curYU=0, curYD=0;
        noAltitudeProfile=false;
        double curLen = 0, startLen = 0;
        prevPos = pathReversed ? pathPoints[maxEndIndex] : pathPoints[0];
        bool firstVisiblePointSeen=false;
        for(Int i=pathReversed?maxEndIndex-1:1;pathReversed?i>=0:i<=maxEndIndex;pathReversed?i--:i++) {
          MapPosition curPos = pathPoints[i];
          if ((prevPos!=NavigationPath::getPathInterruptedPos())&&(curPos!=NavigationPath::getPathInterruptedPos())) {
            if (!firstVisiblePointSeen) {
              startLen=curLen;
            }
            curLen += curPos.computeDistance(prevPos);

            // Only draw the requested horizontal part of the profile
            if ((i>=startIndex)&&(i<=endIndex)) {
              firstVisiblePointSeen=true;
              curX=round((curLen-startLen)*pixelPerLen);
              //DEBUG("curX=%d curLen=%f",curX,curLen);
              if (curX>prevX) {

                // Compute the current Y positions
                if (curPos.getHasAltitude()) {
                  curY=round(((double)curPos.getAltitude()-visiblePathMinAltitude)*pixelPerHeight);
                }
                if (prevPos.getHasAltitude()) {
                  prevY=round(((double)prevPos.getAltitude()-visiblePathMinAltitude)*pixelPerHeight);
                }
                //DEBUG("curX=%d curY=%d curLen=%f  pixelPerLen=%f pixelPerHeight=%f",curX,curY,curLen,pixelPerLen,pixelPerHeight);

                // Add the new point to the profile
                altitudeProfileFillPoints.push_back(GraphicPoint(prevX,0));
                altitudeProfileFillPoints.push_back(GraphicPoint(prevX,prevY));
                altitudeProfileFillPoints.push_back(GraphicPoint(curX,0));
                altitudeProfileFillPoints.push_back(GraphicPoint(curX,0));
                altitudeProfileFillPoints.push_back(GraphicPoint(prevX,prevY));
                altitudeProfileFillPoints.push_back(GraphicPoint(curX,curY));
                double alpha=atan(((double)(curY-prevY))/((double)(curX-prevX)));
                Int dX=round(sin(alpha)*(((double)altitudeProfileLineWidth)/2));
                Int dY=round(cos(alpha)*(((double)altitudeProfileLineWidth)/2));
                curXU=curX-dX;
                curYU=curY+dY;
                curXD=curX+dX;
                curYD=curY-dY;
                prevXU=prevX-dX;
                prevYU=prevY+dY;
                prevXD=prevX+dX;
                prevYD=prevY-dY;
                altitudeProfileLinePoints.push_back(GraphicPoint(prevXU,prevYU));
                altitudeProfileLinePoints.push_back(GraphicPoint(curXU,curYU));
                altitudeProfileLinePoints.push_back(GraphicPoint(prevXD,prevYD));
                altitudeProfileLinePoints.push_back(GraphicPoint(prevXD,prevYD));
                altitudeProfileLinePoints.push_back(GraphicPoint(curXD,curYD));
                altitudeProfileLinePoints.push_back(GraphicPoint(curXU,curYU));
                if (pathReversed?i<=endIndex-1:i>=startIndex+1) {
                  altitudeProfileLinePoints.push_back(GraphicPoint(prevXD2,prevYD2));
                  altitudeProfileLinePoints.push_back(GraphicPoint(prevXD,prevYD));
                  altitudeProfileLinePoints.push_back(GraphicPoint(prevX,prevY));
                  altitudeProfileLinePoints.push_back(GraphicPoint(prevXU2,prevYU2));
                  altitudeProfileLinePoints.push_back(GraphicPoint(prevXU,prevYU));
                  altitudeProfileLinePoints.push_back(GraphicPoint(prevX,prevY));
                }
                prevY=curY;
                prevX=curX;
                prevXU2=curXU;
                prevYU2=curYU;
                prevXD2=curXD;
                prevYD2=curYD;
              }
            }

            // If this position is nearer to the current location, update the position of the location indicator
            if (locationPos.isValid()) {
              double d=curPos.computeDistance(locationPos);
              if (d<minDistance) {
                locationIcon.setX(this->x+altitudeProfileOffsetX+curX-locationIcon.getIconWidth()/2);
                locationIcon.setY(this->y+altitudeProfileOffsetY+curY-locationIcon.getIconHeight()/2);
                minDistance=d;
                if ((i<startIndex)||(i>endIndex)) {
                  hideLocationIcon=true;
                } else {
                  hideLocationIcon=false;
                }
              }
            } else {
              hideLocationIcon=true;
            }
          }
          prevPos = curPos;
        }
        if (altitudeProfileFillPointBuffer) delete altitudeProfileFillPointBuffer;
        altitudeProfileFillPointBuffer=new GraphicPointBuffer(altitudeProfileFillPoints.size());
        if (!altitudeProfileFillPointBuffer) {
          FATAL("can not create point buffer for altitude profile",NULL);
          return changed;
        }
        altitudeProfileFillPointBuffer->addPoints(altitudeProfileFillPoints);
        if (altitudeProfileLinePointBuffer) delete altitudeProfileLinePointBuffer;
        altitudeProfileLinePointBuffer=new GraphicPointBuffer(altitudeProfileLinePoints.size());
        if (!altitudeProfileLinePointBuffer) {
          FATAL("can not create point buffer for altitude profile",NULL);
          return changed;
        }
        altitudeProfileLinePointBuffer->addPoints(altitudeProfileLinePoints);

        // Prepare the axis
        fontEngine->lockFont("sansTiny");
        altitudeProfileAxisPointBuffer.reset();
        Int count=altitudeProfileXTickCount;
        if (!altitudeProfileXTickFontStrings) {
          if (!(altitudeProfileXTickFontStrings=(FontString**)malloc(altitudeProfileXTickCount*sizeof(FontString*)))) {
            FATAL("can not create font string array for x tick labels of altitude profile",NULL);
            return changed;
          }
          memset(altitudeProfileXTickFontStrings,0,altitudeProfileXTickCount*sizeof(FontString*));
        }
        std::string lockedUnit;
        core->getUnitConverter()->formatMeters(pathLength,value,lockedUnit);
        Int precision=-1;
        do {
          precision++;
          core->getUnitConverter()->formatMeters(visiblePathMaxDistance,value,unit,precision,lockedUnit);
          DEBUG("visiblePathMaxDistance=%f value=%s",visiblePathMaxDistance,value.c_str());
        }
        while (value.size()<altitudeProfileXTickLabelWidth);
        DEBUG("precision=%d",precision);
        core->getUnitConverter()->formatMeters(pathLength,value,lockedUnit,0);
        Int negHalveLineWidth = -altitudeProfileAxisLineWidth/2;
        Int posHalveLineWidth = altitudeProfileAxisLineWidth/2+altitudeProfileAxisLineWidth%2;
        for(Int i=0;i<count;i++) {
          Int x=i*altitudeProfileWidth/(count-1)+negHalveLineWidth;
          altitudeProfileAxisPointBuffer.addPoint(x,negHalveLineWidth);
          altitudeProfileAxisPointBuffer.addPoint(x,altitudeProfileHeight+posHalveLineWidth);
          altitudeProfileAxisPointBuffer.addPoint(x+altitudeProfileAxisLineWidth,negHalveLineWidth);
          altitudeProfileAxisPointBuffer.addPoint(x+altitudeProfileAxisLineWidth,negHalveLineWidth);
          altitudeProfileAxisPointBuffer.addPoint(x+altitudeProfileAxisLineWidth,altitudeProfileHeight+posHalveLineWidth);
          altitudeProfileAxisPointBuffer.addPoint(x,altitudeProfileHeight+posHalveLineWidth);
          core->getUnitConverter()->formatMeters(startLen+((double)i)*(visiblePathLength/(double)(count-1)),value,unit,precision,lockedUnit);
          fontEngine->updateString(&altitudeProfileXTickFontStrings[i],value);
          double x2=this->x+altitudeProfileOffsetX+x+((double)altitudeProfileAxisLineWidth)/2;
          if (i==0)
            x2+=0;
          else
            x2-=((double)altitudeProfileXTickFontStrings[i]->getIconWidth())/2;
          altitudeProfileXTickFontStrings[i]->setX(x2);
          altitudeProfileXTickFontStrings[i]->setY(this->y+altitudeProfileOffsetY-altitudeProfileXTickLabelOffsetY-altitudeProfileXTickFontStrings[i]->getIconHeight());
        }
        count=altitudeProfileYTickCount;
        if (!altitudeProfileYTickFontStrings) {
          if (!(altitudeProfileYTickFontStrings=(FontString**)malloc(altitudeProfileYTickCount*sizeof(FontString*)))) {
            FATAL("can not create font string array for y tick labels of altitude profile",NULL);
            return changed;
          }
          memset(altitudeProfileYTickFontStrings,0,altitudeProfileYTickCount*sizeof(FontString*));
        }
        core->getUnitConverter()->formatMeters(pathMaxAltitude,value,lockedUnit);
        precision=-1;
        do {
          precision++;
          core->getUnitConverter()->formatMeters(visiblePathMaxAltitude,value,unit,precision,lockedUnit);
        }
        while (value.size()<altitudeProfileYTickLabelWidth);
        for(Int i=0;i<count;i++) {
          Int y=i*altitudeProfileHeight/(count-1)+negHalveLineWidth;
          altitudeProfileAxisPointBuffer.addPoint(negHalveLineWidth,y);
          altitudeProfileAxisPointBuffer.addPoint(altitudeProfileWidth+posHalveLineWidth,y);
          altitudeProfileAxisPointBuffer.addPoint(altitudeProfileWidth+posHalveLineWidth,y+altitudeProfileAxisLineWidth);
          altitudeProfileAxisPointBuffer.addPoint(altitudeProfileWidth+posHalveLineWidth,y+altitudeProfileAxisLineWidth);
          altitudeProfileAxisPointBuffer.addPoint(negHalveLineWidth,y+altitudeProfileAxisLineWidth);
          altitudeProfileAxisPointBuffer.addPoint(negHalveLineWidth,y);
          core->getUnitConverter()->formatMeters(visiblePathMinAltitude+((double)i)*(altitudeDiff/(double)(count-1)),value,unit,precision,lockedUnit);
          fontEngine->updateString(&altitudeProfileYTickFontStrings[i],value);
          double y2=this->y+altitudeProfileOffsetY+y+((double)altitudeProfileAxisLineWidth)/2-altitudeProfileYTickFontStrings[i]->getBaselineOffsetY();
          if (i==0)
            y2+=0;
          else
            y2-=((double)altitudeProfileYTickFontStrings[i]->getIconHeight())/2;
          altitudeProfileYTickFontStrings[i]->setY(y2);
          altitudeProfileYTickFontStrings[i]->setX(this->x+altitudeProfileOffsetX-altitudeProfileYTickLabelOffsetX-altitudeProfileYTickFontStrings[i]->getIconWidth());
        }
        fontEngine->unlockFont();
      }
    }
    if (noAltitudeProfile) {
      if (altitudeProfileFillPointBuffer) delete altitudeProfileFillPointBuffer;
      altitudeProfileFillPointBuffer=NULL;
      if (altitudeProfileLinePointBuffer) delete altitudeProfileLinePointBuffer;
      altitudeProfileLinePointBuffer=NULL;
      fontEngine->lockFont("sansNormal");
      fontEngine->updateString(&noAltitudeProfileFontString,"No altitude profile");
      fontEngine->unlockFont();
      noAltitudeProfileFontString->setX(x+noAltitudeProfileOffsetX-noAltitudeProfileFontString->getIconWidth()/2);
      noAltitudeProfileFontString->setY(y+noAltitudeProfileOffsetY-noAltitudeProfileFontString->getIconHeight()/2);
    }

  }
  redrawRequired=false;

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetPathInfo::draw(Screen *screen, TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(screen,t);

  // Shall the widget be shown
  if (color.getAlpha()!=0) {

    // Draw the path infos
    if (pathNameFontString) {
      pathNameFontString->setColor(color);
      pathNameFontString->draw(screen,t);
      pathLengthFontString->setColor(color);
      pathLengthFontString->draw(screen,t);
      pathAltitudeUpFontString->setColor(color);
      pathAltitudeUpFontString->draw(screen,t);
      pathAltitudeDownFontString->setColor(color);
      pathAltitudeDownFontString->draw(screen,t);
      pathDurationFontString->setColor(color);
      pathDurationFontString->draw(screen,t);
    }

    // Draw the altitude profile
    if (altitudeProfileFillPointBuffer) {
      screen->startObject();
      screen->translate(getX()+altitudeProfileOffsetX,getY()+altitudeProfileOffsetY,getZ());
      screen->setColor(altitudeProfileFillColor.getRed(),altitudeProfileFillColor.getGreen(),altitudeProfileFillColor.getBlue(),color.getAlpha());
      altitudeProfileFillPointBuffer->drawAsTriangles(screen);
      screen->setColor(altitudeProfileLineColor.getRed(),altitudeProfileLineColor.getGreen(),altitudeProfileLineColor.getBlue(),color.getAlpha());
      altitudeProfileLinePointBuffer->drawAsTriangles(screen);
      screen->setColor(altitudeProfileAxisColor.getRed(),altitudeProfileAxisColor.getGreen(),altitudeProfileAxisColor.getBlue(),altitudeProfileAxisColor.getAlpha()*color.getAlpha()/255);
      altitudeProfileAxisPointBuffer.drawAsTriangles(screen);
      screen->endObject();
      for(Int i=0;i<altitudeProfileXTickCount;i++) {
        altitudeProfileXTickFontStrings[i]->setColor(color);
        altitudeProfileXTickFontStrings[i]->draw(screen,t);
      }
      for(Int i=0;i<altitudeProfileYTickCount;i++) {
        altitudeProfileYTickFontStrings[i]->setColor(color);
        altitudeProfileYTickFontStrings[i]->draw(screen,t);
      }
      if (!hideLocationIcon) {
        locationIcon.setColor(color);
        locationIcon.draw(screen,t);
      }
    } else {
      if (noAltitudeProfileFontString) {
        noAltitudeProfileFontString->setColor(color);
        noAltitudeProfileFontString->draw(screen,t);
      }
    }
  }
}

// Called when the map has changed
void WidgetPathInfo::onMapChange(MapPosition pos) {

  // Do not change if path is locked
  if (currentPathLocked)
    return;

  // Find the nearest path in the currently visible map tile
  std::list<NavigationPathSegment*> nearbyPathSegments;
  if (pos.getMapTile()) {
    nearbyPathSegments=pos.getMapTile()->getCrossingNavigationPathSegments();
  }
  NavigationPath *nearestPath=NULL;
  double minDistance=std::numeric_limits<double>::max();
  for(std::list<NavigationPathSegment*>::iterator i=nearbyPathSegments.begin();i!=nearbyPathSegments.end();i++) {
    NavigationPathSegment *s=*i;
    s->getPath()->lockAccess();
    for(Int j=s->getStartIndex();j<=s->getEndIndex();j++) {
      MapPosition pathPos=s->getPath()->getPoint(j);
      double d=pos.computeDistance(pathPos);
      if (d<minDistance) {
        minDistance=d;
        nearestPath=s->getPath();
      }
    }
    s->getPath()->unlockAccess();
  }

  // Visualize the path
  if ((nearestPath)&&(nearestPath!=currentPath)) {

    // Remember the selected path
    currentPath=nearestPath;
    resetPathVisibility();
  }

}

// Ensures that the complete path becomes visible
void WidgetPathInfo::resetPathVisibility() {
  if (currentPath) {
    currentPath->lockAccess();
    if (currentPath->getReverse()) {
      startIndex=0;
      endIndex=currentPath->getSize()-2;
    } else {
      startIndex=1;
      endIndex=currentPath->getSize()-1;
    }
    maxEndIndex=currentPath->getSize()-1;
    indexLen=endIndex-startIndex;
    currentPathName=currentPath->getName();
    currentPath->unlockAccess();
    if (endIndex<startIndex) {
      currentPath=NULL;
    } else {
      core->getWidgetEngine()->setWidgetsActive(true,false);
    }
    core->getConfigStore()->setStringValue("Navigation","pathInfoName",currentPathName);
  }
  redrawRequired=true;
}

// Called when the location has changed
void WidgetPathInfo::onLocationChange(MapPosition pos) {
  locationPos=pos;
  core->getWidgetEngine()->setWidgetsActive(true,false);
  redrawRequired=true;
}

// Called when a path has changed
void WidgetPathInfo::onPathChange(NavigationPath *path) {

  // If no path is selected, check if the given path was previsouly selected
  if ((currentPath==NULL)&&(path->getName()==currentPathName)) {
    currentPath=path;
  }

  // Check if the widget needs to change its shown path
  MapPosition *pos=core->getMapEngine()->lockMapPos();
  if (pos->getMapTile()!=NULL)
    onMapChange(*pos);
  core->getMapEngine()->unlockMapPos();

  // Redraw is also required if this widget is showing the changed paht
  if (currentPath==path) {
    resetPathVisibility();
  }
}

// Called when a two fingure gesture is done on the widget
void WidgetPathInfo::onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff) {
  if (!currentPath)
    return;
  Int newStartIndex, newEndIndex;
  scaleDiff=2.0-scaleDiff;
  indexLen=indexLen*scaleDiff;
  if (indexLen>maxEndIndex+1)
    indexLen=maxEndIndex+1;
  Int indexLenDiff = indexLen - (endIndex-startIndex+1);
  //DEBUG("scaleDiff=%f indexLenDiff=%d",scaleDiff,indexLenDiff);
  currentPath->lockAccess();
  if (currentPath->getReverse()) {
    newEndIndex=endIndex+indexLenDiff/2;
    if (newEndIndex>maxEndIndex-1)
      newEndIndex=maxEndIndex-1;
    if (newEndIndex<0)
      newEndIndex=0;
    newStartIndex=startIndex-indexLenDiff/2;
    if (newStartIndex<0)
      newStartIndex=0;
    if (newStartIndex>newEndIndex)
      newStartIndex=newEndIndex;
  } else {
    newStartIndex=startIndex-indexLenDiff/2;
    if (newStartIndex<1)
      newStartIndex=1;
    if (newStartIndex>maxEndIndex)
      newStartIndex=maxEndIndex;
    newEndIndex=endIndex+indexLenDiff/2;
    if (newEndIndex>maxEndIndex)
      newEndIndex=maxEndIndex;
    if (newEndIndex<newStartIndex)
      newEndIndex=newStartIndex;
  }
  endIndex=newEndIndex;
  startIndex=newStartIndex;
  //DEBUG("startIndex=%d endIndex=%d",startIndex,endIndex);
  currentPath->unlockAccess();
  redrawRequired=true;
}

// Called when the widget is touched
void WidgetPathInfo::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchDown(t,x,y);
  if ((!firstTouchDown)&&(isSelected)) {
    if (!currentPath)
      return;
    Int dX=x-prevX;
    currentPath->lockAccess();
    if (currentPath->getReverse()) {
      dX=-dX;
      if ((startIndex-dX>=0)&&(endIndex-dX<=maxEndIndex-1)) {
        endIndex=endIndex-dX;
        startIndex=startIndex-dX;
      }
    } else {
      if ((startIndex-dX>=1)&&(endIndex-dX<=maxEndIndex)) {
        startIndex=startIndex-dX;
        endIndex=endIndex-dX;
      }
    }
    currentPath->unlockAccess();
    redrawRequired=true;
  }
  prevX=x;
  firstTouchDown=false;
}

// Called when the widget is not touched anymore
void WidgetPathInfo::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchUp(t,x,y);
  firstTouchDown=true;
}

} /* namespace GEODISCOVERER */
