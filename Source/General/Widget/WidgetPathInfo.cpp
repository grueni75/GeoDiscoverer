//============================================================================
// Name        : WidgetPathInfo.cpp
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

#include <Core.h>

namespace GEODISCOVERER {

// Static variables
std::string WidgetPathInfo::currentPathName = "";
bool WidgetPathInfo::currentPathLocked = false;

// Path info widget thread
void *widgetPathInfoThread(void *args) {
  ((WidgetPathInfo*)args)->updateVisualization();
  return NULL;
}

// Constructor
WidgetPathInfo::WidgetPathInfo(WidgetPage *widgetPage) :
  WidgetPrimitive(widgetPage),
  locationIcon(screen),
  navigationPointIcon(screen)
{

  widgetType=WidgetTypePathInfo;
  altitudeProfileXTickCount=0;
  altitudeProfileYTickCount=0;
  altitudeProfileHeightWithNavigationPoints=0;
  altitudeProfileHeightWithoutNavigationPoints=0;
  pathNameFontString=NULL;
  pathLengthFontString=NULL;
  pathAltitudeUpFontString=NULL;
  pathAltitudeDownFontString=NULL;
  pathDurationFontString=NULL;
  currentPath=NULL;
  altitudeProfileFillPointBuffer=NULL;
  altitudeProfileLinePointBuffer=NULL;
  altitudeProfileAxisPointBuffer=NULL;
  noAltitudeProfileFontString=NULL;
  altitudeProfileXTickFontStrings=NULL;
  altitudeProfileYTickFontStrings=NULL;
  altitudeProfileNavigationPoints=NULL;
  redrawRequired=false;
  startIndex=0;
  endIndex=0;
  maxEndIndex=0;
  hideLocationIcon=true;
  firstTouchDown=true;
  prevX=0;
  currentPathName=core->getConfigStore()->getStringValue("Navigation","pathInfoName",__FILE__, __LINE__);
  currentPathLocked=core->getConfigStore()->getIntValue("Navigation","pathInfoLocked",__FILE__, __LINE__);
  updateVisualizationSignal=core->getThread()->createSignal();
  visualizationMutex=core->getThread()->createMutex("widget path info visualization mutex");
  widgetPathInfoThreadInfo=core->getThread()->createThread("widget path info thread",widgetPathInfoThread,this);
  minDistanceToBeOffRoute=core->getConfigStore()->getDoubleValue("Navigation","minDistanceToBeOffRoute",__FILE__, __LINE__);
  widgetPathInfoThreadWorkingMutex=core->getThread()->createMutex("widget path info thread working mutex");
  quitWidgetPathInfoThread=false;
  visualizationPathName=NULL;
  visualizationPathLength=NULL;
  visualizationPathAltitudeUp=NULL;
  visualizationPathAltitudeDown=NULL;
  visualizationPathDuration=NULL;
  visualizationAltitudeProfileFillPoints=NULL;
  visualizationAltitudeProfileLinePoints=NULL;
  visualizationAltitudeProfileLocationIconPoint=NULL;
  visualizationAltitudeProfileHideLocationIcon=false;
  visualizationAltitudeProfileAxisPoints=NULL;
  visualizationAltitudeProfileXTickLabels=NULL;
  visualizationAltitudeProfileXTickPoints=NULL;
  visualizationAltitudeProfileYTickLabels=NULL;
  visualizationAltitudeProfileYTickPoints=NULL;
  visualizationAltitudeProfileNavigationPoints=NULL;
  visualizationNoAltitudeProfile=false;
  visualizationValid=false;
}

// Destructor
WidgetPathInfo::~WidgetPathInfo() {
  quitWidgetPathInfoThread=true;
  core->getThread()->issueSignal(updateVisualizationSignal);
  core->getThread()->waitForThread(widgetPathInfoThreadInfo);
  core->getThread()->destroyThread(widgetPathInfoThreadInfo);
  core->getThread()->destroySignal(updateVisualizationSignal);
  core->getThread()->destroyMutex(visualizationMutex);
  core->getThread()->destroyMutex(widgetPathInfoThreadWorkingMutex);
  widgetPage->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (pathNameFontString) widgetPage->getFontEngine()->destroyString(pathNameFontString);
  if (pathLengthFontString) widgetPage->getFontEngine()->destroyString(pathLengthFontString);
  if (pathAltitudeUpFontString) widgetPage->getFontEngine()->destroyString(pathAltitudeUpFontString);
  if (pathAltitudeDownFontString) widgetPage->getFontEngine()->destroyString(pathAltitudeDownFontString);
  if (pathDurationFontString) widgetPage->getFontEngine()->destroyString(pathDurationFontString);
  widgetPage->getFontEngine()->unlockFont();
  if (altitudeProfileFillPointBuffer) delete altitudeProfileFillPointBuffer;
  if (altitudeProfileLinePointBuffer) delete altitudeProfileLinePointBuffer;
  if (altitudeProfileAxisPointBuffer) delete altitudeProfileAxisPointBuffer;
  if (altitudeProfileNavigationPoints) delete altitudeProfileNavigationPoints;
  widgetPage->getFontEngine()->lockFont("sansTiny",__FILE__, __LINE__);
  if (altitudeProfileXTickFontStrings) {
    for (Int i=0;i<altitudeProfileXTickCount;i++)
      if (altitudeProfileXTickFontStrings[i]) widgetPage->getFontEngine()->destroyString(altitudeProfileXTickFontStrings[i]);
    free(altitudeProfileXTickFontStrings);
  }
  if (altitudeProfileYTickFontStrings) {
    for (Int i=0;i<altitudeProfileYTickCount;i++)
      if (altitudeProfileYTickFontStrings[i]) widgetPage->getFontEngine()->destroyString(altitudeProfileYTickFontStrings[i]);
    free(altitudeProfileYTickFontStrings);
  }
  widgetPage->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetPathInfo::work(TimestampInMicroseconds t) {

  FontEngine *fontEngine=widgetPage->getFontEngine();
  Int textX, textY;
  std::list<std::string> status;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Redraw the widget if required
  core->getThread()->lockMutex(visualizationMutex,__FILE__, __LINE__);
  if (redrawRequired&&visualizationValid) {

    // Update the labels
    /*visualizationPathName="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    visualizationPathLength="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    visualizationPathAltitudeUp="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    visualizationPathAltitudeDown="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    visualizationPathDuration="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";*/
    fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
    fontEngine->updateString(&pathNameFontString,*visualizationPathName,pathNameWidth);
    Int maxWidth=pathNameFontString->getIconWidth();
    pathNameFontString->setX(x+pathNameOffsetX+(pathNameWidth-maxWidth)/2);
    pathNameFontString->setY(y+pathNameOffsetY);
    fontEngine->updateString(&pathLengthFontString,*visualizationPathLength,pathValuesWidth);
    fontEngine->updateString(&pathAltitudeUpFontString,*visualizationPathAltitudeUp,pathValuesWidth);
    fontEngine->updateString(&pathAltitudeDownFontString,*visualizationPathAltitudeDown,pathValuesWidth);
    fontEngine->updateString(&pathDurationFontString,*visualizationPathDuration,pathValuesWidth);
    fontEngine->unlockFont();
    maxWidth=pathLengthFontString->getIconWidth();
    if (pathAltitudeUpFontString->getIconWidth()>maxWidth) maxWidth=pathAltitudeUpFontString->getIconWidth();
    if (pathAltitudeDownFontString->getIconWidth()>maxWidth) maxWidth=pathAltitudeDownFontString->getIconWidth();
    if (pathDurationFontString->getIconWidth()>maxWidth) maxWidth=pathDurationFontString->getIconWidth();
    //maxWidth=pathValuesWidth;
    pathLengthFontString->setX(x+pathLengthOffsetX+(pathValuesWidth-maxWidth)/2);
    pathLengthFontString->setY(y+pathLengthOffsetY);
    pathAltitudeUpFontString->setX(x+pathAltitudeUpOffsetX+(pathValuesWidth-maxWidth)/2);
    pathAltitudeUpFontString->setY(y+pathAltitudeUpOffsetY);
    pathAltitudeDownFontString->setX(x+pathAltitudeDownOffsetX+(pathValuesWidth-maxWidth)/2);
    pathAltitudeDownFontString->setY(y+pathAltitudeDownOffsetY);
    pathDurationFontString->setX(x+pathDurationOffsetX+(pathValuesWidth-maxWidth)/2);
    pathDurationFontString->setY(y+pathDurationOffsetY);

    // Show only text if no altitude profile is present
    if (visualizationNoAltitudeProfile) {
      if (altitudeProfileFillPointBuffer) delete altitudeProfileFillPointBuffer;
      altitudeProfileFillPointBuffer=NULL;
      if (altitudeProfileLinePointBuffer) delete altitudeProfileLinePointBuffer;
      altitudeProfileLinePointBuffer=NULL;
      fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
      fontEngine->updateString(&noAltitudeProfileFontString,"No altitude profile");
      fontEngine->unlockFont();
      noAltitudeProfileFontString->setX(x+noAltitudeProfileOffsetX-noAltitudeProfileFontString->getIconWidth()/2);
      noAltitudeProfileFontString->setY(y+noAltitudeProfileOffsetY-noAltitudeProfileFontString->getIconHeight()/2);
    } else {

      // Create the altitude profile
      if (altitudeProfileFillPointBuffer) delete altitudeProfileFillPointBuffer;
      altitudeProfileFillPointBuffer=new GraphicPointBuffer(screen,visualizationAltitudeProfileFillPoints->size());
      if (!altitudeProfileFillPointBuffer) {
        FATAL("can not create point buffer for altitude profile",NULL);
        return changed;
      }
      altitudeProfileFillPointBuffer->addPoints(visualizationAltitudeProfileFillPoints);
      if (altitudeProfileLinePointBuffer) delete altitudeProfileLinePointBuffer;
      altitudeProfileLinePointBuffer=new GraphicPointBuffer(screen,visualizationAltitudeProfileLinePoints->size());
      if (!altitudeProfileLinePointBuffer) {
        FATAL("can not create point buffer for altitude profile",NULL);
        return changed;
      }
      altitudeProfileLinePointBuffer->addPoints(visualizationAltitudeProfileLinePoints);
      locationIcon.setX(this->x+visualizationAltitudeProfileLocationIconPoint->getX());
      locationIcon.setY(this->y+visualizationAltitudeProfileLocationIconPoint->getY());
      hideLocationIcon=visualizationAltitudeProfileHideLocationIcon;

      // Create the axis
      fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
      if (altitudeProfileAxisPointBuffer) {
        altitudeProfileAxisPointBuffer->reset();
        altitudeProfileAxisPointBuffer->addPoints(visualizationAltitudeProfileAxisPoints);
      }
      for(Int i=0;i<visualizationAltitudeProfileXTickLabels->size();i++) {
        fontEngine->updateString(&altitudeProfileXTickFontStrings[i],visualizationAltitudeProfileXTickLabels->operator[](i));
        altitudeProfileXTickFontStrings[i]->setX(this->x+visualizationAltitudeProfileXTickPoints->operator[](i).getX()-altitudeProfileXTickFontStrings[i]->getIconWidth()/2);
        altitudeProfileXTickFontStrings[i]->setY(this->y+visualizationAltitudeProfileXTickPoints->operator[](i).getY()-altitudeProfileXTickFontStrings[i]->getIconHeight());
      }
      for(Int i=0;i<visualizationAltitudeProfileYTickLabels->size();i++) {
        fontEngine->updateString(&altitudeProfileYTickFontStrings[i],visualizationAltitudeProfileYTickLabels->operator[](i));
        Int y=altitudeProfileYTickFontStrings[i]->getBaselineOffsetY();
        if (i!=0)
          y+=altitudeProfileYTickFontStrings[i]->getIconHeight()/2;
        altitudeProfileYTickFontStrings[i]->setX(this->x+visualizationAltitudeProfileYTickPoints->operator[](i).getX()-altitudeProfileYTickFontStrings[i]->getIconWidth());
        altitudeProfileYTickFontStrings[i]->setY(this->y+visualizationAltitudeProfileYTickPoints->operator[](i).getY()-y);
      }
      fontEngine->unlockFont();

      // Create the navigation points
      if (altitudeProfileNavigationPoints) delete altitudeProfileNavigationPoints;
      altitudeProfileNavigationPoints=NULL;
      if (visualizationAltitudeProfileNavigationPoints) {
        altitudeProfileNavigationPoints=new GraphicRectangleList(screen,visualizationAltitudeProfileNavigationPoints->size());
        if (!altitudeProfileLinePointBuffer) {
          FATAL("can not create point buffer for altitude profile",NULL);
          return changed;
        }
        altitudeProfileNavigationPoints->setTexture(navigationPointIcon.getTexture());
        altitudeProfileNavigationPoints->setDestroyTexture(false);
        Int navigationPointWidth=navigationPointIcon.getWidth();
        Int navigationPointHeight=navigationPointIcon.getHeight();
        Int navigationPointIconWidth=navigationPointIcon.getIconWidth();
        Int navigationPointIconHeight=navigationPointIcon.getIconHeight();
        Int navigationPointIconOffsetX=(navigationPointWidth-navigationPointIconWidth)/2;
        Int navigationPointIconOffsetY=(navigationPointHeight-navigationPointIconHeight)/2;
        double navigationPointIconRadius=sqrt((double)(navigationPointWidth*navigationPointWidth/4+navigationPointHeight*navigationPointHeight/4));
        altitudeProfileNavigationPoints->setParameter(navigationPointIconRadius,0,0);
        std::list<double> usedX;
        for (std::list<NavigationPoint>::iterator i=visualizationAltitudeProfileNavigationPoints->begin();i!=visualizationAltitudeProfileNavigationPoints->end();i++) {
          bool found=false;
          for (std::list<double>::iterator j=usedX.begin();j!=usedX.end();j++) {
            if (*j==i->getX()) {
              found=true;
              break;
            }
          }
          if (!found) {
            altitudeProfileNavigationPoints->addRectangle(i->getX()+navigationPointIconOffsetX,i->getY()+navigationPointIconOffsetY,0);
            usedX.push_back(i->getX());
          }
        }
      }
    }

  }
  redrawRequired=false;
  core->getThread()->unlockMutex(visualizationMutex);

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetPathInfo::draw(TimestampInMicroseconds t) {

  //PROFILE_START;

  // Let the primitive draw the background
  WidgetPrimitive::draw(t);
  //PROFILE_ADD("primitive draw");

  // Shall the widget be shown
  if (color.getAlpha()!=0) {

    // Draw the path infos
    if (pathNameFontString) {
      pathNameFontString->setColor(color);
      pathNameFontString->draw(t);
      pathLengthFontString->setColor(color);
      pathLengthFontString->draw(t);
      pathAltitudeUpFontString->setColor(color);
      pathAltitudeUpFontString->draw(t);
      pathAltitudeDownFontString->setColor(color);
      pathAltitudeDownFontString->draw(t);
      pathDurationFontString->setColor(color);
      pathDurationFontString->draw(t);
    }
    //PROFILE_ADD("path info draw");

    // Draw the altitude profile
    if (altitudeProfileFillPointBuffer) {
      screen->startObject();
      screen->translate(getX()+altitudeProfileOffsetX,getY()+altitudeProfileOffsetY,getZ());
      screen->setColor(altitudeProfileFillColor.getRed(),altitudeProfileFillColor.getGreen(),altitudeProfileFillColor.getBlue(),color.getAlpha());
      altitudeProfileFillPointBuffer->drawAsTriangles();
      screen->setColor(altitudeProfileLineColor.getRed(),altitudeProfileLineColor.getGreen(),altitudeProfileLineColor.getBlue(),color.getAlpha());
      altitudeProfileLinePointBuffer->drawAsTriangles();
      screen->setColor(altitudeProfileAxisColor.getRed(),altitudeProfileAxisColor.getGreen(),altitudeProfileAxisColor.getBlue(),altitudeProfileAxisColor.getAlpha()*color.getAlpha()/255);
      if (altitudeProfileAxisPointBuffer)
        altitudeProfileAxisPointBuffer->drawAsTriangles();
      screen->endObject();
      for(Int i=0;i<altitudeProfileXTickCount;i++) {
        altitudeProfileXTickFontStrings[i]->setColor(color);
        altitudeProfileXTickFontStrings[i]->draw(t);
      }
      for(Int i=0;i<altitudeProfileYTickCount;i++) {
        altitudeProfileYTickFontStrings[i]->setColor(color);
        altitudeProfileYTickFontStrings[i]->draw(t);
      }
      if (!hideLocationIcon) {
        locationIcon.setColor(color);
        locationIcon.draw(t);
      }
      if (altitudeProfileNavigationPoints) {
        screen->startObject();
        screen->translate(getX()+altitudeProfileOffsetX,getY()+altitudeProfileOffsetY+altitudeProfileHeightWithNavigationPoints,getZ());
        altitudeProfileNavigationPoints->setColor(color);
        altitudeProfileNavigationPoints->draw();
        screen->endObject();
      }
    } else {
      if (noAltitudeProfileFontString) {
        noAltitudeProfileFontString->setColor(color);
        noAltitudeProfileFontString->draw(t);
      }
    }
    //PROFILE_ADD("altitude profile draw");
  }
}

// Ensures that the complete path becomes visible
void WidgetPathInfo::resetPathVisibility(bool widgetVisible) {
  NavigationPath *path=currentPath;
  if (path) {
    path->lockAccess(__FILE__, __LINE__);
    if (path->getReverse()) {
      startIndex=0;
      endIndex=path->getSelectedSize()-2;
    } else {
      startIndex=1;
      endIndex=path->getSelectedSize()-1;
    }
    maxEndIndex=path->getSelectedSize()-1;
    indexLen=endIndex-startIndex;
    currentPathName=path->getGpxFilename();
    path->unlockAccess();
    core->getConfigStore()->setStringValue("Navigation","pathInfoName",currentPathName,__FILE__, __LINE__);
  }
  core->getThread()->issueSignal(updateVisualizationSignal);
}

// Called when the map has changed
void WidgetPathInfo::onMapChange(bool widgetVisible, MapPosition pos) {

  // Do not change if path is locked
  if (currentPathLocked)
    return;

  //PROFILE_START;

  // Visualize the nearest path if it has changed  PROFILE_END;

  NavigationPath* nearestPath = widgetPage->getWidgetEngine()->getNearestPath();
  if ((nearestPath)&&(nearestPath!=currentPath)) {

    // Remember the selected path
    currentPath=nearestPath;
    //PROFILE_ADD("nearest path change check");
    resetPathVisibility(widgetVisible);
    //PROFILE_ADD("reset path visibility");
    if (widgetVisible) {
      widgetPage->getWidgetEngine()->setWidgetsActive(true);
    }
    //PROFILE_ADD("set widgets active");

  }
  //PROFILE_ADD("clean up");
}

// Called when the location has changed
void WidgetPathInfo::onLocationChange(bool widgetVisible, MapPosition pos) {
  core->getThread()->lockMutex(widgetPathInfoThreadWorkingMutex,__FILE__,__LINE__);
  locationPos=pos;
  core->getThread()->unlockMutex(widgetPathInfoThreadWorkingMutex);
  core->getThread()->issueSignal(updateVisualizationSignal);
}

// Called when a path has changed
void WidgetPathInfo::onPathChange(bool widgetVisible, NavigationPath *path, NavigationPathChangeType changeType) {

  // If no path is selected, check if the given path was previously selected
  if ((currentPath==NULL)&&(path->getGpxFilename()==currentPathName)) {
    currentPath=path;
    resetPathVisibility(widgetVisible);
  }

  // Redraw is also required if this widget is showing the changed path
  if (currentPath==path) {

    // Was only a new point added?
    Int newEndIndex;
    bool reversed;
    switch(changeType) {
      case NavigationPathChangeTypeEndPositionAdded:

        // If the user has zoomed in, keep the zoom
        path->lockAccess(__FILE__, __LINE__);
        newEndIndex=path->getSelectedSize()-1;
        reversed=path->getReverse();
        maxEndIndex=path->getSelectedSize()-1;
        path->unlockAccess();
        if (newEndIndex>1) {
          if (reversed) {
            if ((startIndex==0)&&(endIndex==newEndIndex-2)) {
              resetPathVisibility(widgetVisible);
            }
          } else {
            if ((startIndex==1)&&(endIndex==newEndIndex-1)) {
              resetPathVisibility(widgetVisible);
            }
          }
        } else {
          resetPathVisibility(widgetVisible);
        }
        break;
      case NavigationPathChangeTypeFlagSet:
        resetPathVisibility(widgetVisible);
        break;
      case NavigationPathChangeTypeWillBeRemoved:
        core->getThread()->lockMutex(widgetPathInfoThreadWorkingMutex,__FILE__,__LINE__);
        currentPath=NULL;
        core->getThread()->unlockMutex(widgetPathInfoThreadWorkingMutex);
        break;
      case NavigationPathChangeTypeWidgetEngineInit:
        // Already handled by the prefix code
        break;
      default:
        FATAL("navigation path change type not supported",NULL);
        break;
    }
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
  currentPath->lockAccess(__FILE__, __LINE__);
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
  firstTouchDown=true;
  currentPath->unlockAccess();
  core->getThread()->issueSignal(updateVisualizationSignal);
}

// Called when the widget is touched
void WidgetPathInfo::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchDown(t,x,y);
  if ((!firstTouchDown)&&(isSelected)) {
    if (!currentPath)
      return;
    Int dX=x-prevX;
    currentPath->lockAccess(__FILE__, __LINE__);
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
    core->getThread()->issueSignal(updateVisualizationSignal);
  }
  prevX=x;
  firstTouchDown=false;
}

// Called when the widget is not touched anymore
void WidgetPathInfo::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {
  WidgetPrimitive::onTouchUp(t,x,y,cancel);
  firstTouchDown=true;
}

// Recomputes the visualization of the path info
void WidgetPathInfo::updateVisualization() {

  std::list<std::string> status;

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // This thread can be cancelled at any time
  core->getThread()->setThreadCancable();

  // Do an endless loop
  while (true) {

    // Wait for an update trigger
    core->getThread()->waitForSignal(updateVisualizationSignal);
    if (quitWidgetPathInfoThread)
      return;

    // Thread is now working
    core->getThread()->lockMutex(widgetPathInfoThreadWorkingMutex,__FILE__,__LINE__);

    //PROFILE_START;

    // Variables that define the visualization
    std::string *vPathName = new std::string();
    std::string *vPathLength = new std::string();
    std::string *vPathAltitudeUp = new std::string();
    std::string *vPathAltitudeDown = new std::string();
    std::string *vPathDuration = new std::string();
    std::list<GraphicPoint> *altitudeProfileFillPoints = new std::list<GraphicPoint>;
    std::list<GraphicPoint> *altitudeProfileLinePoints = new std::list<GraphicPoint>;
    GraphicPoint *altitudeProfileLocationIconPoint = new GraphicPoint;
    bool altitudeProfileHideLocationIcon=true;
    std::list<GraphicPoint> *altitudeProfileAxisPoints = new std::list<GraphicPoint>;
    std::vector<std::string> *altitudeProfileXTickLabels = new std::vector<std::string>;
    std::vector<GraphicPoint> *altitudeProfileXTickPoints = new std::vector<GraphicPoint>;
    std::vector<std::string> *altitudeProfileYTickLabels = new std::vector<std::string>;
    std::vector<GraphicPoint> *altitudeProfileYTickPoints = new std::vector<GraphicPoint>;
    std::list<NavigationPoint> *altitudeProfileNavigationPoints = NULL;
    bool noAltitudeProfile=true;

    // Only update if we have a path
    NavigationPath *currentPath=this->currentPath;
    if (currentPath) {

      // Get infos from path
      currentPath->lockAccess(__FILE__, __LINE__);
      std::string pathName=currentPath->getGpxFilename();
      double pathLength=currentPath->getLength();
      double pathDuration=currentPath->getDuration();
      double pathAltitudeUp=currentPath->getAltitudeUp();
      double pathAltitudeDown=currentPath->getAltitudeDown();
      // it seems that the start end indexes changed by the background loader causes some memory problem here
      std::vector<MapPosition> pathPoints=currentPath->getSelectedPoints();
      //DEBUG("pathPoints.size()=%d",pathPoints.size());
      bool pathReversed=currentPath->getReverse();
      double pathMinAltitude = currentPath->getMinAltitude();
      double pathMaxAltitude = currentPath->getMaxAltitude();
      Int pathEndIndex = endIndex;
      Int pathStartIndex = startIndex;
      Int pathMaxEndIndex = maxEndIndex;
      if (pathMaxEndIndex>=pathPoints.size()) {
        pathMaxEndIndex=pathPoints.size()-1;
      }
      if (pathEndIndex>pathMaxEndIndex) {
        pathEndIndex=pathMaxEndIndex;
      }
      if (pathStartIndex>=pathEndIndex) {
        pathEndIndex=pathEndIndex-1;
      }
      if (pathStartIndex<0) {
        pathStartIndex=0;
      }
      currentPath->unlockAccess();
      //PROFILE_ADD("get path info");

      // Get navigation point infos
      std::list<NavigationPoint> *navigationPoints=core->getNavigationEngine()->lockAddressPoints(__FILE__,__LINE__);
      if (navigationPoints->size()>0) {
        altitudeProfileNavigationPoints = new std::list<NavigationPoint>;
        for (std::list<NavigationPoint>::iterator i=navigationPoints->begin();i!=navigationPoints->end();i++) {
          NavigationPoint p = *i;
          p.setDistance(std::numeric_limits<double>::max());
          altitudeProfileNavigationPoints->push_back(p);
        }
      }
      core->getNavigationEngine()->unlockAddressPoints();
      Int altitudeProfileHeight=this->altitudeProfileHeightWithoutNavigationPoints;
      if (altitudeProfileNavigationPoints) {
        altitudeProfileHeight=altitudeProfileHeightWithNavigationPoints;
      }

      // Update the labels
      *vPathName=pathName;
      std::string value="";
      std::string unit="";
      core->getUnitConverter()->formatMeters(pathLength,value,unit,1);
      *vPathLength=value + " " + unit;
      //vPathLength="XXXXXXXXXXXXXXX";
      core->getUnitConverter()->formatMeters(pathAltitudeUp,value,unit,1);
      *vPathAltitudeUp=value + " " + unit;
      core->getUnitConverter()->formatMeters(pathAltitudeDown,value,unit,1);
      *vPathAltitudeDown=value + " " + unit;
      core->getUnitConverter()->formatTime(pathDuration,value,unit,1);
      *vPathDuration=value + " " + unit;

      // Compute the altitude profile
      double minDistance=std::numeric_limits<double>::max();
      double minBearingDiff=std::numeric_limits<double>::max();
      if ((pathMinAltitude<=pathMaxAltitude)&&(pathPoints.size()>=2)) {
        MapPosition prevPos=NavigationPath::getPathInterruptedPos();
        double visiblePathLength=0;
        double visiblePathMinAltitude=std::numeric_limits<double>::max();
        double visiblePathMaxAltitude=std::numeric_limits<double>::min();
        for(Int i=pathReversed?pathEndIndex+1:pathStartIndex-1;pathReversed?i>=pathStartIndex:i<=pathEndIndex;pathReversed?i--:i++) {
          MapPosition curPos = pathPoints[i];
          if (curPos!=NavigationPath::getPathInterruptedPos()) {
            if (curPos.getHasAltitude()) {
              if (curPos.getAltitude()<visiblePathMinAltitude)
                visiblePathMinAltitude=curPos.getAltitude();
              if (curPos.getAltitude()>visiblePathMaxAltitude)
                visiblePathMaxAltitude=curPos.getAltitude();
            }
            if (prevPos!=NavigationPath::getPathInterruptedPos()) {
              if (pathReversed?i<=pathEndIndex:i>=pathStartIndex) {
                visiblePathLength += curPos.computeDistance(prevPos);
              }
            }
          }
          prevPos = curPos;
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
          double prevX=0,curX=0;
          double prevY=0,curY=0;
          double prevXU=0, prevXD=0, curXU=0, curXD=0;
          Int prevXU2i=0, prevXD2i=0;
          double prevYU=0, prevYD=0, curYU=0, curYD=0;
          Int prevYU2i=0, prevYD2i=0;
          noAltitudeProfile=false;
          double curLen = 0, startLen = 0;
          prevPos = pathReversed ? pathPoints[pathMaxEndIndex] : pathPoints[0];
          bool firstVisiblePointSeen=false;
          for(Int i=pathReversed?pathMaxEndIndex-1:1;pathReversed?i>=0:i<=pathMaxEndIndex;pathReversed?i--:i++) {
            MapPosition curPos = pathPoints[i];
            if ((prevPos!=NavigationPath::getPathInterruptedPos())&&(curPos!=NavigationPath::getPathInterruptedPos())) {
              if (!firstVisiblePointSeen) {
                startLen=curLen;
              }
              curLen += curPos.computeDistance(prevPos);

              // Only draw the requested horizontal part of the profile
              if ((i>=pathStartIndex)&&(i<=pathEndIndex)) {
                firstVisiblePointSeen=true;
                curX=(curLen-startLen)*pixelPerLen;
                //DEBUG("curX=%d curLen=%f",curX,curLen);
                if (curX>prevX) {

                  // Compute the current Y positions
                  if (curPos.getHasAltitude()) {
                    curY=((double)curPos.getAltitude()-visiblePathMinAltitude)*pixelPerHeight;
                  }
                  if (prevPos.getHasAltitude()) {
                    prevY=((double)prevPos.getAltitude()-visiblePathMinAltitude)*pixelPerHeight;
                  }
                  //DEBUG("curX=%d curY=%d curLen=%f  pixelPerLen=%f pixelPerHeight=%f",curX,curY,curLen,pixelPerLen,pixelPerHeight);

                  // Add the new point to the profile
                  Int curXi=round(curX);
                  Int curYi=round(curY);
                  Int prevXi=round(prevX);
                  Int prevYi=round(prevY);
                  altitudeProfileFillPoints->push_back(GraphicPoint(prevXi,0));
                  altitudeProfileFillPoints->push_back(GraphicPoint(prevXi,prevYi));
                  altitudeProfileFillPoints->push_back(GraphicPoint(curXi,0));
                  altitudeProfileFillPoints->push_back(GraphicPoint(curXi,0));
                  altitudeProfileFillPoints->push_back(GraphicPoint(prevXi,prevYi));
                  altitudeProfileFillPoints->push_back(GraphicPoint(curXi,curYi));
                  double alpha=atan(((double)(curY-prevY))/((double)(curX-prevX)));
                  double dX=sin(alpha)*(((double)altitudeProfileLineWidth)/2);
                  double dY=cos(alpha)*(((double)altitudeProfileLineWidth)/2);
                  curXU=curX-dX;
                  curYU=curY+dY;
                  curXD=curX+dX;
                  curYD=curY-dY;
                  prevXU=prevX-dX;
                  prevYU=prevY+dY;
                  prevXD=prevX+dX;
                  prevYD=prevY-dY;
                  Int curXUi=round(curXU);
                  Int curYUi=round(curYU);
                  Int curXDi=round(curXD);
                  Int curYDi=round(curYD);
                  Int prevXUi=round(prevXU);
                  Int prevYUi=round(prevYU);
                  Int prevXDi=round(prevXD);
                  Int prevYDi=round(prevYD);
                  altitudeProfileLinePoints->push_back(GraphicPoint(prevXUi,prevYUi));
                  altitudeProfileLinePoints->push_back(GraphicPoint(curXUi,curYUi));
                  altitudeProfileLinePoints->push_back(GraphicPoint(prevXDi,prevYDi));
                  altitudeProfileLinePoints->push_back(GraphicPoint(prevXDi,prevYDi));
                  altitudeProfileLinePoints->push_back(GraphicPoint(curXDi,curYDi));
                  altitudeProfileLinePoints->push_back(GraphicPoint(curXUi,curYUi));
                  if (pathReversed?i<=pathEndIndex-1:i>=pathStartIndex+1) {
                    altitudeProfileLinePoints->push_back(GraphicPoint(prevXD2i,prevYD2i));
                    altitudeProfileLinePoints->push_back(GraphicPoint(prevXDi,prevYDi));
                    altitudeProfileLinePoints->push_back(GraphicPoint(prevXi,prevYi));
                    altitudeProfileLinePoints->push_back(GraphicPoint(prevXU2i,prevYU2i));
                    altitudeProfileLinePoints->push_back(GraphicPoint(prevXUi,prevYUi));
                    altitudeProfileLinePoints->push_back(GraphicPoint(prevXi,prevYi));
                  }
                  prevY=curY;
                  prevX=curX;
                  prevXU2i=curXUi;
                  prevYU2i=curYUi;
                  prevXD2i=curXDi;
                  prevYD2i=curYDi;
                }
              }

              // If this position is nearer to the current location, update the position of the location indicator
              if (locationPos.isValid()) {
                bool updateLocationIcon=false;
                double d=curPos.computeDistance(locationPos);
                if ((locationPos.getHasBearing())&&(d<minDistanceToBeOffRoute)) {
                  double bearingDiff=fabs(prevPos.computeBearing(curPos)-locationPos.getBearing());
                  if (bearingDiff<minBearingDiff) {
                    minBearingDiff=bearingDiff;
                    updateLocationIcon=true;
                  }
                } else {
                  if (d<minDistance) {
                    updateLocationIcon=true;
                  }
                }
                if (updateLocationIcon) {
                  altitudeProfileLocationIconPoint->setX(altitudeProfileOffsetX+curX-locationIcon.getIconWidth()/2);
                  altitudeProfileLocationIconPoint->setY(altitudeProfileOffsetY+curY-locationIcon.getIconHeight()/2);
                  minDistance=d;
                  if ((i<pathStartIndex)||(i>pathEndIndex)) {
                    altitudeProfileHideLocationIcon=true;
                  } else {
                    altitudeProfileHideLocationIcon=false;
                  }
                }
              } else {
                altitudeProfileHideLocationIcon=true;
              }

              // Remember for each navigation point the nearest position in the altitude profile
              if (altitudeProfileNavigationPoints) {
                for (std::list<NavigationPoint>::iterator i=altitudeProfileNavigationPoints->begin();i!=altitudeProfileNavigationPoints->end();i++) {
                  MapPosition navigationPointPos;
                  navigationPointPos.setLat(i->getLat());
                  navigationPointPos.setLng(i->getLng());
                  double currentDistance=curPos.computeDistance(navigationPointPos);
                  if (currentDistance<i->getDistance()) {
                    i->setDistance(currentDistance);
                    i->setX(round(curX));
                    i->setY(0);
                  }
                }
              }
            }
            prevPos = curPos;
          }

          // Prepare the axis
          Int count=altitudeProfileXTickCount;
          if (!altitudeProfileXTickFontStrings) {
            if (!(altitudeProfileXTickFontStrings=(FontString**)malloc(altitudeProfileXTickCount*sizeof(FontString*)))) {
              FATAL("can not create font string array for x tick labels of altitude profile",NULL);
              return;
            }
            memset(altitudeProfileXTickFontStrings,0,altitudeProfileXTickCount*sizeof(FontString*));
          }
          std::string lockedUnit;
          core->getUnitConverter()->formatMeters(pathLength,value,lockedUnit);
          Int precision=-1;
          do {
            precision++;
            core->getUnitConverter()->formatMeters(startLen+visiblePathLength,value,unit,precision+1,lockedUnit);
          }
          while (value.length()<=altitudeProfileXTickLabelWidth);
          core->getUnitConverter()->formatMeters(pathLength,value,lockedUnit,0);
          Int negHalveLineWidth = -altitudeProfileAxisLineWidth/2;
          Int posHalveLineWidth = altitudeProfileAxisLineWidth/2+altitudeProfileAxisLineWidth%2;
          for(Int i=0;i<count;i++) {
            Int x=i*altitudeProfileWidth/(count-1)+negHalveLineWidth;
            altitudeProfileAxisPoints->push_back(GraphicPoint(x,negHalveLineWidth));
            altitudeProfileAxisPoints->push_back(GraphicPoint(x,altitudeProfileHeight+posHalveLineWidth));
            altitudeProfileAxisPoints->push_back(GraphicPoint(x+altitudeProfileAxisLineWidth,negHalveLineWidth));
            altitudeProfileAxisPoints->push_back(GraphicPoint(x+altitudeProfileAxisLineWidth,negHalveLineWidth));
            altitudeProfileAxisPoints->push_back(GraphicPoint(x+altitudeProfileAxisLineWidth,altitudeProfileHeight+posHalveLineWidth));
            altitudeProfileAxisPoints->push_back(GraphicPoint(x,altitudeProfileHeight+posHalveLineWidth));
            core->getUnitConverter()->formatMeters(startLen+((double)i)*(visiblePathLength/(double)(count-1)),value,unit,precision,lockedUnit);
            altitudeProfileXTickLabels->push_back(value);
            Int x2=altitudeProfileOffsetX+x+altitudeProfileAxisLineWidth/2;
            altitudeProfileXTickPoints->push_back(GraphicPoint(x2,altitudeProfileOffsetY-altitudeProfileXTickLabelOffsetY));
          }
          count=altitudeProfileYTickCount;
          if (!altitudeProfileYTickFontStrings) {
            if (!(altitudeProfileYTickFontStrings=(FontString**)malloc(altitudeProfileYTickCount*sizeof(FontString*)))) {
              FATAL("can not create font string array for y tick labels of altitude profile",NULL);
              return;
            }
            memset(altitudeProfileYTickFontStrings,0,altitudeProfileYTickCount*sizeof(FontString*));
          }
          std::string value1, lockedUnit1, value2, lockedUnit2;
          double visiblePathRefAltitude;
          core->getUnitConverter()->formatMeters(visiblePathMaxAltitude,value1,lockedUnit1);
          core->getUnitConverter()->formatMeters(visiblePathMinAltitude,value2,lockedUnit2);
          if (value2.size()>value1.size()) {
            lockedUnit=lockedUnit2;
            visiblePathRefAltitude=visiblePathMinAltitude;
          } else {
            lockedUnit=lockedUnit1;
            visiblePathRefAltitude=visiblePathMaxAltitude;
          }
          precision=-1;
          do {
            precision++;
            core->getUnitConverter()->formatMeters(visiblePathRefAltitude,value,unit,precision+1,lockedUnit);
          }
          while (value.length()<=altitudeProfileYTickLabelWidth);
          for(Int i=0;i<count;i++) {
            Int y=i*altitudeProfileHeight/(count-1)+negHalveLineWidth;
            altitudeProfileAxisPoints->push_back(GraphicPoint(negHalveLineWidth,y));
            altitudeProfileAxisPoints->push_back(GraphicPoint(altitudeProfileWidth+posHalveLineWidth,y));
            altitudeProfileAxisPoints->push_back(GraphicPoint(altitudeProfileWidth+posHalveLineWidth,y+altitudeProfileAxisLineWidth));
            altitudeProfileAxisPoints->push_back(GraphicPoint(altitudeProfileWidth+posHalveLineWidth,y+altitudeProfileAxisLineWidth));
            altitudeProfileAxisPoints->push_back(GraphicPoint(negHalveLineWidth,y+altitudeProfileAxisLineWidth));
            altitudeProfileAxisPoints->push_back(GraphicPoint(negHalveLineWidth,y));
            core->getUnitConverter()->formatMeters(visiblePathMinAltitude+((double)i)*(altitudeDiff/(double)(count-1)),value,unit,precision,lockedUnit);
            altitudeProfileYTickLabels->push_back(value);
            Int y2=altitudeProfileOffsetY+y+altitudeProfileAxisLineWidth/2;
            altitudeProfileYTickPoints->push_back(GraphicPoint(altitudeProfileOffsetX-altitudeProfileYTickLabelOffsetX,y2));
          }
        }
      }
    }
    //PROFILE_ADD("compute path info");

    // Update the variables for the next drawing round
    std::string *oldVisualizationPathName;
    std::string *oldVisualizationPathLength;
    std::string *oldVisualizationPathAltitudeUp;
    std::string *oldVisualizationPathAltitudeDown;
    std::string *oldVisualizationPathDuration;
    std::list<GraphicPoint> *oldVisualizationAltitudeProfileFillPoints;
    std::list<GraphicPoint> *oldVisualizationAltitudeProfileLinePoints;
    GraphicPoint *oldVisualizationAltitudeProfileLocationIconPoint;
    std::list<GraphicPoint> *oldVisualizationAltitudeProfileAxisPoints;
    std::vector<std::string> *oldVisualizationAltitudeProfileXTickLabels;
    std::vector<GraphicPoint> *oldVisualizationAltitudeProfileXTickPoints;
    std::vector<std::string> *oldVisualizationAltitudeProfileYTickLabels;
    std::vector<GraphicPoint> *oldVisualizationAltitudeProfileYTickPoints;
    std::list<NavigationPoint> *oldVisualizationNavigationPoints;
    core->getThread()->lockMutex(visualizationMutex,__FILE__, __LINE__);
    oldVisualizationPathName=this->visualizationPathName;
    this->visualizationPathName=vPathName;
    oldVisualizationPathLength=this->visualizationPathLength;
    this->visualizationPathLength=vPathLength;
    oldVisualizationPathAltitudeUp=this->visualizationPathAltitudeUp;
    this->visualizationPathAltitudeUp=vPathAltitudeUp;
    oldVisualizationPathAltitudeDown=this->visualizationPathAltitudeDown;
    this->visualizationPathAltitudeDown=vPathAltitudeDown;
    oldVisualizationPathDuration=this->visualizationPathDuration;
    this->visualizationPathDuration=vPathDuration;
    oldVisualizationAltitudeProfileFillPoints=this->visualizationAltitudeProfileFillPoints;
    this->visualizationAltitudeProfileFillPoints=altitudeProfileFillPoints;
    oldVisualizationAltitudeProfileLinePoints=this->visualizationAltitudeProfileLinePoints;
    this->visualizationAltitudeProfileLinePoints=altitudeProfileLinePoints;
    oldVisualizationAltitudeProfileLocationIconPoint=this->visualizationAltitudeProfileLocationIconPoint;
    this->visualizationAltitudeProfileLocationIconPoint=altitudeProfileLocationIconPoint;
    this->visualizationAltitudeProfileHideLocationIcon=altitudeProfileHideLocationIcon;
    oldVisualizationAltitudeProfileAxisPoints=this->visualizationAltitudeProfileAxisPoints;
    this->visualizationAltitudeProfileAxisPoints=altitudeProfileAxisPoints;
    oldVisualizationAltitudeProfileXTickLabels=this->visualizationAltitudeProfileXTickLabels;
    this->visualizationAltitudeProfileXTickLabels=altitudeProfileXTickLabels;
    oldVisualizationAltitudeProfileXTickPoints=this->visualizationAltitudeProfileXTickPoints;
    this->visualizationAltitudeProfileXTickPoints=altitudeProfileXTickPoints;
    oldVisualizationAltitudeProfileYTickLabels=this->visualizationAltitudeProfileYTickLabels;
    this->visualizationAltitudeProfileYTickLabels=altitudeProfileYTickLabels;
    oldVisualizationAltitudeProfileYTickPoints=this->visualizationAltitudeProfileYTickPoints;
    this->visualizationAltitudeProfileYTickPoints=altitudeProfileYTickPoints;
    this->visualizationNoAltitudeProfile=noAltitudeProfile;
    oldVisualizationNavigationPoints=this->visualizationAltitudeProfileNavigationPoints;
    this->visualizationAltitudeProfileNavigationPoints=altitudeProfileNavigationPoints;
    redrawRequired=true;
    visualizationValid=true;
    core->getThread()->unlockMutex(visualizationMutex);
    //PROFILE_ADD("put path info");

    // Delete objects from previous run
    delete oldVisualizationPathName;
    delete oldVisualizationPathLength;
    delete oldVisualizationPathAltitudeUp;
    delete oldVisualizationPathAltitudeDown;
    delete oldVisualizationPathDuration;
    delete oldVisualizationAltitudeProfileFillPoints;
    delete oldVisualizationAltitudeProfileLinePoints;
    delete oldVisualizationAltitudeProfileLocationIconPoint;
    delete oldVisualizationAltitudeProfileAxisPoints;
    delete oldVisualizationAltitudeProfileXTickLabels;
    delete oldVisualizationAltitudeProfileXTickPoints;
    delete oldVisualizationAltitudeProfileYTickLabels;
    delete oldVisualizationAltitudeProfileYTickPoints;
    delete oldVisualizationNavigationPoints;
    //PROFILE_ADD("delete old objects");

    // Thread is not working anymore
    core->getThread()->unlockMutex(widgetPathInfoThreadWorkingMutex);

  }
}

// Set the x tick count for the altitude profile
void WidgetPathInfo::setAltitudeProfileXTickCount(Int altitudeProfileXTickCount) {
  this->altitudeProfileXTickCount = altitudeProfileXTickCount;
  if (altitudeProfileAxisPointBuffer) delete altitudeProfileAxisPointBuffer;
  altitudeProfileAxisPointBuffer=new GraphicPointBuffer(screen,(altitudeProfileXTickCount+altitudeProfileYTickCount)*6);
}

// Set the y tick count for the altitude profile
void WidgetPathInfo::setAltitudeProfileYTickCount(Int altitudeProfileYTickCount) {
  this->altitudeProfileYTickCount = altitudeProfileYTickCount;
  if (altitudeProfileAxisPointBuffer) delete altitudeProfileAxisPointBuffer;
  altitudeProfileAxisPointBuffer=new GraphicPointBuffer(screen,(altitudeProfileXTickCount+altitudeProfileYTickCount)*6);
}

// Called when the widget has changed his position
void WidgetPathInfo::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  redrawRequired=true;
}

// Called when some data has changed
void WidgetPathInfo::onDataChange() {
  core->getThread()->issueSignal(updateVisualizationSignal);
}

} /* namespace GEODISCOVERER */
