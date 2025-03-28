//============================================================================
// Name        : Commander.cpp
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
#include <Commander.h>
#include <GraphicPosition.h>
#include <GraphicEngine.h>
#include <NavigationEngine.h>
#include <WidgetEngine.h>
#include <Device.h>
#include <MapEngine.h>
#include <WidgetPathInfo.h>
#include <UnitConverter.h>
#include <NavigationPath.h>
#include <NavigationEngine.h>
#include <ElevationEngine.h>
#include <MapSourceMercatorTiles.h>
#include <ZipArchive.h>
#include <CRC16.h>

namespace GEODISCOVERER {

// Constructor
Commander::Commander() {
  accessMutex=core->getThread()->createMutex("commander access mutex");
  lastTouchedX=0;
  lastTouchedY=0;
  mapTileNr=0;
}

// Destructor
Commander::~Commander() {
  core->getThread()->destroyMutex(accessMutex);
}

// Extracts the command name and its arguments from a command
bool Commander::splitCommand(std::string cmdString, std::string& cmd, std::vector<std::string>& args) {

  // Extract the command to execute
  Int argStart=cmdString.find_first_of('(');
  if (argStart==std::string::npos) {
    ERROR("can not extract start of arguments from command <%s>",cmdString.c_str());
    return false;
  }
  Int argEnd=cmdString.find_last_of(')');
  if (argEnd==std::string::npos) {
    ERROR("can not extract end of arguments from command <%s>",cmdString.c_str());
    return false;
  }
  cmd=cmdString.substr(0,argStart);
  std::string cmdArgs=cmdString.substr(argStart+1,argEnd-argStart-1);
  std::string unparsedCmdArgs=cmdArgs;
  enum { normal, ignoreCommas, } state = normal;
  std::string currentArg="";
  for (Int i=0;i<cmdArgs.size();i++) {
    bool endFound=false;
    bool processCharacter=false;
    switch (state) {
    case normal:
      switch (cmdArgs[i]) {
      case '"':
        state=ignoreCommas;
        break;
      case ',':
        endFound=true;
        break;
      default:
        processCharacter=true;
      }
      break;
    case ignoreCommas:
      switch (cmdArgs[i]) {
      case '"':
        state=normal;
        break;
      default:
        processCharacter=true;
      }
      break;
    }
    if (processCharacter) {
      if ((cmdArgs[i]=='\\')&&(i+1<cmdArgs.length())) {
        switch(cmdArgs[i+1]) {
        case '\\':
          currentArg+='\\';
          i++;
          processCharacter=false;
          break;
        case '"':
          currentArg+='"';
          i++;
          processCharacter=false;
          break;
        }
      }
      if (processCharacter) {
        currentArg+=cmdArgs[i];
      }
    }
    if (endFound) {
      args.push_back(currentArg);
      currentArg="";
    }
  }
  args.push_back(currentArg);
  currentArg="";
  return true;
}

// Creates a command string from the given arguments
std::string Commander::joinCommand(std::string cmd, std::vector<std::string> args) {

  std::stringstream cmdStr;
  cmdStr << cmd << "(";
  for (std::vector<std::string>::iterator i=args.begin();i!=args.end();i++) {
    cmdStr << *i;
    if (i!=args.end()-1)
      cmdStr << ",";
  }
  cmdStr << ")";
  return cmdStr.str();
}

// Execute a command
std::string Commander::execute(std::string cmd) {

  //DEBUG("executing command <%s>",cmd.c_str());
  core->getThread()->lockMutex(accessMutex, __FILE__, __LINE__);
  TRACE(cmd.c_str(),NULL);
  core->getThread()->unlockMutex(accessMutex);
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();

  //DEBUG("cmd=%s",cmd.c_str());

  // Set the default result
  std::string result="";

  // Extract the command to execute
  std::string cmdName;
  std::vector<std::string> args;
  if (!splitCommand(cmd,cmdName,args)) {
    return "";
  }

  // Handle the command
  bool cmdExecuted=false;
  //DEBUG("before: x=%d y=%d",pos->getX(),pos->getY());
  if (cmdName=="zoom") {
    GraphicPosition *pos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
    pos->zoom(atof(args[0].c_str()));
    pos->updateLastUserModification();
    core->getDefaultGraphicEngine()->unlockPos();
    cmdExecuted=true;
  }
  if (cmdName=="pan") {
    GraphicPosition *pos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
    pos->pan(atoi(args[0].c_str()),atoi(args[1].c_str()));
    pos->updateLastUserModification();
    core->getDefaultGraphicEngine()->unlockPos();
    cmdExecuted=true;
  }
  if (cmdName=="rotate") {
    GraphicPosition *pos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
    pos->rotate(atof(args[0].c_str()));
    pos->updateLastUserModification();
    core->getDefaultGraphicEngine()->unlockPos();
    // Map is not update if rotation only, so we need to update screen graphic of navigation engine here
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->updateScreenGraphic(false);
    }
    cmdExecuted=true;
  }
  if (cmdName=="setPage") {
    core->getDefaultWidgetEngine()->setPage(args[0],atoi(args[1].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="getPage") {
    result=core->getConfigStore()->getStringValue("Graphic/Widget/Device[@name='" + core->getDefaultDevice()->getName() + "']","selectedPage",__FILE__, __LINE__);
    cmdExecuted=true;
  }
  if (cmdName=="twoFingerGesture") {

    // Convert the coordinates
    Int x,y;
    x=atoi(args[0].c_str());
    y=atoi(args[1].c_str());
    x=x-core->getDefaultScreen()->getWidth()/2;
    y=core->getDefaultScreen()->getHeight()/2-1-y;
    core->getThread()->lockMutex(accessMutex, __FILE__, __LINE__);
    Int dX=lastTouchedX-x;
    Int dY=lastTouchedY-y;
    core->getThread()->unlockMutex(accessMutex);

    // Touch on widgets possible?
    TimestampInMicroseconds duration;
    bool widgetTouched=false;
    if ((!core->getDefaultGraphicEngine()->isAmbientMode(duration))&&(!core->getDefaultGraphicEngine()->isWidgetlessMode(duration))) {

      // First check if a widget was two fingure gestured
      if (core->getDefaultWidgetEngine()->onTwoFingerGesture(t,dX,dY,atof(args[2].c_str()),atof(args[3].c_str()))) {
        widgetTouched=true;
      }
    }

    // Then do the map scrolling
    if (!widgetTouched) {
      GraphicPosition *pos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
      pos->rotate(atof(args[2].c_str()));
      pos->zoom(atof(args[3].c_str()));
      pos->pan(dX,dY);
      pos->updateLastUserModification();
      core->getDefaultGraphicEngine()->unlockPos();
    }

    // Update some variables
    core->getThread()->lockMutex(accessMutex, __FILE__, __LINE__);
    lastTouchedX=x;
    lastTouchedY=y;
    core->getThread()->unlockMutex(accessMutex);
    cmdExecuted=true;
  }
  if (cmdName.substr(0,5)=="touch") {
    Int x,y;
    x=atoi(args[0].c_str());
    y=atoi(args[1].c_str());

    // Convert the coordinates to screen coordinates
    x=x-core->getDefaultScreen()->getWidth()/2;
    y=core->getDefaultScreen()->getHeight()/2-1-y;

    // Touch possible on widgets?
    TimestampInMicroseconds duration;
    bool widgetTouched=false;
    if ((!core->getDefaultGraphicEngine()->isAmbientMode(duration))&&(!core->getDefaultGraphicEngine()->isWidgetlessMode(duration))) {

      // First check if a widget was touched
      if ((cmdName=="touchDown")||(cmdName=="touchMove")) {
        //DEBUG("touchDown(%d,%d)",x,y);
        if (core->getDefaultWidgetEngine()->onTouchDown(t,x,y))
          widgetTouched=true;
      }
      if (cmdName=="touchUp") {
        //DEBUG("touchUp(%d,%d)",x,y);
        if (core->getDefaultWidgetEngine()->onTouchUp(t,x,y))
          widgetTouched=true;
      }
      if (cmdName=="touchCancel") {
        //DEBUG("touchUp(%d,%d)",x,y);
        if (core->getDefaultWidgetEngine()->onTouchUp(t,x,y,true))
          widgetTouched=true;
      }
    }

    // Then do the map scrolling
    if (!widgetTouched) {
      if ((cmdName=="touchMove")||(cmdName=="touchUp")) {
        core->getThread()->lockMutex(accessMutex, __FILE__, __LINE__);
        Int dX=lastTouchedX-x;
        Int dY=lastTouchedY-y;
        core->getThread()->unlockMutex(accessMutex);
        //DEBUG("pan(%d,%d)",dX,dY);
        GraphicPosition *pos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
        pos->pan(dX,dY);
        pos->updateLastUserModification();
        core->getDefaultGraphicEngine()->unlockPos();
      }
    }

    // Update some variables
    core->getThread()->lockMutex(accessMutex, __FILE__, __LINE__);
    lastTouchedX=x;
    lastTouchedY=y;
    core->getThread()->unlockMutex(accessMutex);
    cmdExecuted=true;
  }
  //DEBUG("after: x=%d y=%d",pos->getX(),pos->getY());
  if (cmdName=="screenChanged") {
    GraphicScreenOrientation orientation=GraphicScreenOrientationProtrait;
    if (args[0] == "landscape") {
      orientation=GraphicScreenOrientationLandscape;
    }
    core->getDefaultDevice()->setOrientation(orientation);
    core->getDefaultDevice()->setWidth(atoi(args[1].c_str()));
    core->getDefaultDevice()->setHeight(atoi(args[2].c_str()));
    core->getMapEngine()->setWidth(atoi(args[1].c_str()));
    core->getMapEngine()->setHeight(atoi(args[2].c_str()));
    core->getDefaultDevice()->reconfigure();
    cmdExecuted=true;
  }
  if (cmdName=="setMapWindow") {
    core->getMapEngine()->setWindow(atoi(args[0].c_str()),atoi(args[1].c_str()),atoi(args[2].c_str()),atoi(args[3].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="graphicInvalidated") {
    core->updateGraphic(true,false);
    cmdExecuted=true;
  }
  if (cmdName=="createGraphic") {
    core->updateGraphic(false,false);
    cmdExecuted=true;
  }
  if (cmdName=="locationChanged") {
    if (core->getIsInitialized()) {

      // Get the fix
      MapPosition pos;
      pos.setTimestamp(atoll(args[1].c_str()));
      pos.setLng(atof(args[2].c_str()));
      pos.setLat(atof(args[3].c_str()));
      pos.setHasAltitude(atoi(args[4].c_str()));
      pos.setAltitude(atof(args[5].c_str()));
      pos.setIsWGS84Altitude(atoi(args[6].c_str()));
      pos.setHasBearing(atoi(args[7].c_str()));
      pos.setBearing(atof(args[8].c_str()));
      pos.setHasSpeed(atoi(args[9].c_str()));
      pos.setSpeed(atof(args[10].c_str()));
      pos.setHasAccuracy(atoi(args[11].c_str()));
      pos.setAccuracy(atof(args[12].c_str()));

      // Inform the location manager
      core->getNavigationEngine()->newLocationFix(pos,std::string(args[0]));
    }
    cmdExecuted=true;
  }
  if (cmdName=="setLocationPos") {

    // Get the fix
    MapPosition pos;
    pos.setTimestamp(atoll(args[1].c_str()));
    pos.setLng(atof(args[2].c_str()));
    pos.setLat(atof(args[3].c_str()));
    pos.setHasAltitude(atoi(args[4].c_str()));
    pos.setAltitude(atof(args[5].c_str()));
    pos.setIsWGS84Altitude(atoi(args[6].c_str()));
    pos.setHasBearing(atoi(args[7].c_str()));
    pos.setBearing(atof(args[8].c_str()));
    pos.setHasSpeed(atoi(args[9].c_str()));
    pos.setSpeed(atof(args[10].c_str()));
    pos.setHasAccuracy(atoi(args[11].c_str()));
    pos.setAccuracy(atof(args[12].c_str()));

    // Inform the location manager
    core->getNavigationEngine()->setLocationPos(pos,std::string(args[0]),false,__FILE__,__LINE__);
    cmdExecuted=true;
  }
  if (cmdName=="setTargetPos") {
    core->getNavigationEngine()->setTargetPos(atof(args[0].c_str()),atof(args[1].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="setRecordTrack") {
    if (core->getIsInitialized()) {
      bool state;
      if (atoi(args[0].c_str())) {
        state=true;
      } else {
        state=false;
      }
      if (core->getNavigationEngine()->setRecordTrack(state))
        result="true";
      else
        result="false";
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="getRecordTrack") {
    if (core->getIsInitialized()) {
      if (core->getConfigStore()->getIntValue("Navigation","recordTrack", __FILE__, __LINE__)) {
        result="true";
      } else {
        result="false";
      }
    }
    cmdExecuted=true;
  }
  if (cmdName=="createNewTrack") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->createNewTrack();
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="exportActiveRoute") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->exportActiveRoute();
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="compassBearingChanged") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->newCompassBearing(atof(args[0].c_str()));
    }
    cmdExecuted=true;
  }
  if (cmdName=="maintenance") {
    if (core->getIsInitialized()) {
      //DEBUG("triggering maintenance",NULL);
      core->triggerMaintenance();
    }
    cmdExecuted=true;
  }
  if (cmdName=="setWakeLock") {
    core->getDefaultScreen()->setWakeLock(atoi(args[0].c_str()), __FILE__, __LINE__);
    cmdExecuted=true;
  }
  if (cmdName=="getWakeLock") {
    if (core->getDefaultScreen()->getWakeLock()) {
      result="true";
    } else {
      result="false";
    }
    cmdExecuted=true;
  }
  if (cmdName=="getMapLegendNames") {
    std::list<std::string> names=core->getMapSource()->getLegendNames();
    result="";
    for (std::list<std::string>::iterator i=names.begin();i!=names.end();i++) {
      if (i!=names.begin())
        result=result+",";
      result=result+*i;
    }
    cmdExecuted=true;
  }
  if (cmdName=="getMapLegendPath") {
    if (args.size()==1)
      result=core->getMapSource()->getLegendPath(args[0].c_str());
    else
      result="";
    cmdExecuted=true;
  }
  if (cmdName=="getMapFolder") {
    result=core->getMapSource()->getFolder();
    cmdExecuted=true;
  }
  if (cmdName=="setReturnToLocation") {
    core->getMapEngine()->setReturnToLocation(atoi(args[0].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="setZoomLevelLock") {
    core->getMapEngine()->setZoomLevelLock(atoi(args[0].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="toggleZoomLevelLock") {
    core->getMapEngine()->toggleZoomLevelLock();
    cmdExecuted=true;
  }
  if (cmdName=="forceMapRedownload") {
    core->getMapEngine()->setForceMapRedownload(atoi(args[0].c_str()),__FILE__,__LINE__);
    core->getMapEngine()->setForceMapUpdate(__FILE__,__LINE__);
    cmdExecuted=true;
  }
  if (cmdName=="forceMapUpdate") {
    core->getMapEngine()->setForceMapUpdate(__FILE__,__LINE__);
    cmdExecuted=true;
  }
  if (cmdName=="updateRoutes") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->updateRoutes();
    }
    cmdExecuted=true;
  }
  if (cmdName=="hideTarget") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->hideTarget();
      INFO("target has been hidden",NULL);
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="showTarget") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->showTarget(true);
      GraphicPosition *visPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
      visPos->updateLastUserModification();
      core->getDefaultGraphicEngine()->unlockPos();
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="setTargetAtMapCenter") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->setTargetAtMapCenter();
      GraphicPosition *visPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
      visPos->updateLastUserModification();
      core->getDefaultGraphicEngine()->unlockPos();
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="setTargetAtGeographicCoordinate") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->setTargetAtGeographicCoordinate(atof(args[0].c_str()),atof(args[1].c_str()),true);
      GraphicPosition *visPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
      visPos->updateLastUserModification();
      core->getDefaultGraphicEngine()->unlockPos();
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="showContextMenu") {
    core->getDefaultWidgetEngine()->showContextMenu();
    cmdExecuted=true;
  }
  if (cmdName=="openFingerMenu") {
    core->getDefaultWidgetEngine()->openFingerMenu();
    cmdExecuted=true;
  }
  if (cmdName=="closeFingerMenu") {
    core->getDefaultWidgetEngine()->closeFingerMenu();
    cmdExecuted=true;
  }
  if (cmdName=="toggleFingerMenu") {
    core->getDefaultWidgetEngine()->toggleFingerMenu();
    cmdExecuted=true;
  }
  if (cmdName=="setTargetAtAddress") {
    core->getDefaultWidgetEngine()->setTargetAtAddress();
    cmdExecuted=true;
  }
  if ((cmdName=="newNavigationInfos")||(cmdName=="lateInitComplete")) {
    core->getDefaultWidgetEngine()->showContextMenu();
    cmdExecuted=true;
  }
  if (cmdName=="setPathInfoLock") {
    bool state;
    if (atoi(args[0].c_str())) {
      state=true;
      INFO("path info widget always shows current path",NULL);
    } else {
      INFO("path info widget shows nearest path",NULL);
      state=false;
    }
    WidgetPathInfo::setCurrentPathLocked(state, __FILE__, __LINE__);
    cmdExecuted=true;
  }
  if (cmdName=="setPathStartFlag") {
    Int nearestPathIndex;
    NavigationPath *nearestPath = core->getDefaultWidgetEngine()->getNearestPath(&nearestPathIndex,NULL);
    if (nearestPath==NULL) {
      WARNING("no path near to the current map center found",NULL);
    } else {
      core->getNavigationEngine()->setStartFlag(nearestPath,nearestPathIndex,__FILE__, __LINE__);
    }
    cmdExecuted=true;
  }
  if (cmdName=="resetPathStartFlag") {
    Int nearestPathIndex;
    NavigationPath *nearestPath = core->getDefaultWidgetEngine()->getNearestPath(&nearestPathIndex,NULL);
    if (nearestPath==NULL) {
      WARNING("no path near to the current map center found",NULL);
    } else {
      core->getNavigationEngine()->setStartFlag(nearestPath,-2,__FILE__, __LINE__);
    }
    cmdExecuted=true;
  }
  if (cmdName=="setPathEndFlag") {
    Int nearestPathIndex;
    NavigationPath *nearestPath = core->getDefaultWidgetEngine()->getNearestPath(&nearestPathIndex,NULL);
    if (nearestPath==NULL) {
      WARNING("no path near to the current map center found",NULL);
    } else {
      core->getNavigationEngine()->setEndFlag(nearestPath,nearestPathIndex,__FILE__, __LINE__);
    }
    cmdExecuted=true;
  }
  if (cmdName=="resetPathEndFlag") {
    Int nearestPathIndex;
    NavigationPath *nearestPath = core->getDefaultWidgetEngine()->getNearestPath(&nearestPathIndex,NULL);
    if (nearestPath==NULL) {
      WARNING("no path near to the current map center found",NULL);
    } else {
      core->getNavigationEngine()->setEndFlag(nearestPath,-2,__FILE__, __LINE__);
    }
    cmdExecuted=true;
  }
  if (cmdName=="setActiveRoute") {
    NavigationPath *nearestPath = core->getDefaultWidgetEngine()->getNearestPath(NULL,NULL);
    if (nearestPath==NULL) {
      WARNING("no path near to the current map center found: Disabling active route",NULL);
    }
    if (nearestPath==core->getNavigationEngine()->getActiveRoute()) {
      nearestPath=NULL;
    }
    core->getNavigationEngine()->setActiveRoute(nearestPath);
    cmdExecuted=true;
  }
  if (cmdName=="trashPath") {
    NavigationPath *nearestPath = core->getDefaultWidgetEngine()->getNearestPath(NULL,NULL);
    if (nearestPath==NULL) {
      WARNING("no path near to the current map center found",NULL);
    } else {
      core->getNavigationEngine()->trashPath(nearestPath);
    }
    cmdExecuted=true;
  }
  if (cmdName=="hidePath") {
    NavigationPath *nearestPath = core->getDefaultWidgetEngine()->getNearestPath(NULL,NULL);
    if (nearestPath==NULL) {
      WARNING("no path near to the current map center found",NULL);
    } else {
      core->getNavigationEngine()->hidePath(nearestPath);
    }
    cmdExecuted=true;
  }
  if (cmdName=="reversePath") {
    NavigationPath *nearestPath = core->getDefaultWidgetEngine()->getNearestPath(NULL,NULL);
    if (nearestPath==NULL) {
      WARNING("no path near to the current map center found",NULL);
    } else {
      core->getNavigationEngine()->reversePath(nearestPath);
    }
    cmdExecuted=true;
  }
  if (cmdName=="log") {
    if (core->getDebug()) {
      if (args[0]=="DEBUG") core->getDebug()->print(verbosityDebug,args[1].c_str(),0,true,args[2].c_str());
      if (args[0]=="INFO") core->getDebug()->print(verbosityInfo,args[1].c_str(),0,true,args[2].c_str());
      if (args[0]=="WARNING") core->getDebug()->print(verbosityWarning,args[1].c_str(),0,true,args[2].c_str());
      if (args[0]=="ERROR") core->getDebug()->print(verbosityError,args[1].c_str(),0,true,args[2].c_str());
      if (args[0]=="FATAL") core->getDebug()->print(verbosityFatal,args[1].c_str(),0,true,args[2].c_str());
      if (args[0]=="UNKNOWN") core->getDebug()->print(verbosityError,args[1].c_str(),0,true,args[2].c_str());
    }
    cmdExecuted=true;
  }
  if (cmdName=="replayTrace") {
    if (core->getDebug()) {
      MapPosition *pos=core->getNavigationEngine()->lockLocationPos(__FILE__,__LINE__);
      pos->setTimestamp(0);
      core->getNavigationEngine()->unlockLocationPos();
      core->getDebug()->replayTrace(args[0]);
    }
    cmdExecuted=true;
  }
  if (cmdName=="addDashboardDevice") {
    core->addDashboardDevice(args[0],atoi(args[1].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="decideContinueOrNewTrack") {
    dispatch(cmd);
    result="false";
    cmdExecuted=true;
  }
  if (cmdName=="changeMapLayer") {
    if (core->getIsInitialized()) {
      std::string appCmd = "changeMapLayer()";
      dispatch(appCmd);
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="selectMapLayer") {
    if (core->getIsInitialized()) {
      core->getMapSource()->selectMapLayer(args[0]);
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="addDownloadJob") {
    if (core->getIsInitialized()) {
      std::string zoomLevels = "";
      for (Int i=2;i<args.size();i++) {
        zoomLevels+=args[i];
        if (i!=args.size()-1)
          zoomLevels+=",";
      }
      core->getMapSource()->addDownloadJob(atoi(args[0].c_str()),args[1],zoomLevels);
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="getMapLayers") {
    if (core->getIsInitialized()) {
      std::list<std::string> names=core->getMapSource()->getMapLayerNames();
      for (std::list<std::string>::iterator i=names.begin();i!=names.end();i++) {
        if (i==names.begin())
          result += *i;
        else
          result += "," + *i;
      }
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="getSelectedMapLayer") {
    if (core->getIsInitialized()) {
      result=core->getMapSource()->getSelectedMapLayer();
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="getMapDownloadActive") {
    result="false";
    if (core->getIsInitialized()) {
      MapDownloader *mapDownloader=core->getMapSource()->getMapDownloader();
      if (mapDownloader&&mapDownloader->countActiveDownloads()>0)
        result="true";
      if (core->getMapSource()->hasDownloadJobs())
        result="true";
    } else {
      //WARNING("Please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="computeDistanceToAddressPoint") {
    NavigationPoint point;
    bool found=true;
    point.setName(args[0]);
    result="";
    if (point.readFromConfig("Navigation/AddressPoint")) {
      MapPosition target;
      target.setLat(point.getLat());
      target.setLng(point.getLng());
      MapPosition *pos=core->getMapEngine()->lockMapPos(__FILE__,__LINE__);
      double distance=pos->computeDistance(target);
      core->getMapEngine()->unlockMapPos();
      std::string value,unit;
      core->getUnitConverter()->formatMeters(distance,value,unit);      
      std::stringstream resultStream;
      resultStream<<distance<<";"<<value<<" "<<unit;
      result=resultStream.str();
    }
    cmdExecuted=true;
  }
  if (cmdName=="formatMeters") {
    std::string value,unit;
    core->getUnitConverter()->formatMeters(atof(args[0].c_str()),value,unit);
    result=value+" "+unit;
    cmdExecuted=true;
  }
  if (cmdName=="addAddressPoint") {
    NavigationPoint point;
    point.setName(args[0]);
    point.setAddress(args[1]);
    point.setLng(atof(args[2].c_str()));
    point.setLat(atof(args[3].c_str()));
    point.setGroup(args[4]);
    core->getNavigationEngine()->addAddressPoint(point);
    cmdExecuted=true;
  }
  if (cmdName=="renameAddressPoint") {
    result=core->getNavigationEngine()->renameAddressPoint(args[0],args[1]);
    cmdExecuted=true;
  }
  if (cmdName=="removeAddressPoint") {
    core->getNavigationEngine()->removeAddressPoint(args[0]);
    cmdExecuted=true;
  }
  if (cmdName=="addressPointGroupChanged") {
    core->getNavigationEngine()->addressPointGroupChanged();
    cmdExecuted=true;
  }
  if (cmdName=="setTargetAtAddressPoint") {
    NavigationPoint point;
    bool found=true;
    if (args[0]=="") {
      GraphicPosition visPos=*(core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__));
      core->getDefaultGraphicEngine()->unlockPos();
      if (!core->getNavigationEngine()->getAddressPoint(visPos,point)) {
        WARNING("no address point near to the current map center found",NULL);
        found=false;
      }
    } else {
      point.setName(args[0]);
      point.readFromConfig("Navigation/AddressPoint");
    }
    if (found) {
      core->getNavigationEngine()->setTargetAtGeographicCoordinate(point.getLng(),point.getLat(),true);
      GraphicPosition *visPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
      visPos->updateLastUserModification();
      core->getDefaultGraphicEngine()->unlockPos();
    }
    cmdExecuted=true;
  }
  if (cmdName=="addAddressPointCandidate") {
    core->getNavigationEngine()->addAddressPointCandidate(args[0],atof(args[1].c_str()),atof(args[2].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="removeAddressPointCandidates") {
    core->getNavigationEngine()->removeAddressPointCandidates();
    cmdExecuted=true;
  }
  if (cmdName=="downloadActiveRoute") {
    const NavigationPath *activeRoute = core->getNavigationEngine()->getActiveRoute();
    if (!activeRoute) {
      ERROR("please activate a route first",NULL);
    } else {
      dispatch("askForMapDownloadDetails(" + activeRoute->getGpxFilename() + ")");
    }
    cmdExecuted=true;
  }
  if (cmdName=="stopDownload") {
    core->getMapSource()->clearDownloadJobs();
    cmdExecuted=true;
  }
  if (cmdName=="remoteMapInit") {
    core->getMapSource()->remoteMapInit();
    cmdExecuted=true;
  }
  if (cmdName=="triggerNavigationInfoUpdate") {
    core->getNavigationEngine()->triggerNavigationInfoUpdate();
    cmdExecuted=true;
  }
  if (cmdName=="findRemoteMapTileByGeographicCoordinate") {
    core->getMapSource()->queueRemoteServerCommand(cmd);
    cmdExecuted=true;
  }
  if (cmdName=="fillGeographicAreaWithRemoteTiles") {
    core->getMapSource()->queueRemoteServerCommand(cmd);
    cmdExecuted=true;
  }
  if (cmdName=="fillGeographicAreaWithRemoteTiles") {
    core->getMapSource()->queueRemoteServerCommand(cmd);
    cmdExecuted=true;
  }
  if (cmdName=="remoteMapArchiveServed") {
    core->getMapSource()->queueRemoteServerCommand(cmd);
    cmdExecuted=true;
  }
  if (cmdName=="remoteOverlayArchiveServed") {
    core->getMapSource()->queueRemoteServerCommand(cmd);
    cmdExecuted=true;
  }
  if (cmdName=="addMapArchive") {
    result=core->getMapSource()->addMapArchive(args[0],args[1]);
    cmdExecuted=true;
  }
  if (cmdName=="addOverlayArchive") {
    result=core->getMapSource()->addOverlayArchive(args[0],args[1]);
    cmdExecuted=true;
  }
  if (cmdName=="setRemoteServerActive") {
    core->setRemoteServerActive(atoi(args[0].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="setTouchMode") {
    core->getDefaultWidgetEngine()->setTouchMode(atoi(args[0].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="setPlainNavigationInfo") {
    NavigationInfo *navigationInfo=core->getNavigationEngine()->lockNavigationInfo(__FILE__,__LINE__);
    navigationInfo->setType((NavigationInfoType)atoi(args[0].c_str()));
    navigationInfo->setAltitude(atof(args[1].c_str()));
    navigationInfo->setLocationBearing(atof(args[2].c_str()));
    navigationInfo->setLocationSpeed(atof(args[3].c_str()));
    navigationInfo->setTrackLength(atof(args[4].c_str()));
    navigationInfo->setTargetBearing(atof(args[5].c_str()));
    navigationInfo->setTargetDistance(atof(args[6].c_str()));
    navigationInfo->setTargetDuration(atof(args[7].c_str()));
    navigationInfo->setOffRoute(atoi(args[8].c_str()));
    navigationInfo->setRouteDistance(atof(args[9].c_str()));
    navigationInfo->setTurnAngle(atof(args[10].c_str()));
    navigationInfo->setTurnDistance(atof(args[11].c_str()));
    navigationInfo->setNearestNavigationPointBearing(atof(args[12].c_str()));
    navigationInfo->setNearestNavigationPointDistance(atof(args[13].c_str()));
    /*DEBUG("navigationInfos: %d %f %f %f %f %f %f %f %d %f %f %f",
        navigationInfo->getType(),
        navigationInfo->getAltitude(),
        navigationInfo->getLocationBearing(),
        navigationInfo->getLocationSpeed(),
        navigationInfo->getTrackLength(),
        navigationInfo->getTargetBearing(),
        navigationInfo->getTargetDistance(),
        navigationInfo->getTargetDuration(),
        navigationInfo->getOffRoute(),
        navigationInfo->getRouteDistance(),
        navigationInfo->getTurnAngle(),
        navigationInfo->getTurnDistance()
        );*/
    core->getNavigationEngine()->unlockNavigationInfo();
    cmdExecuted=true;
  }
  if (cmdName=="setBattery") {
    core->setBatteryLevel(atoi(args[0].c_str()));
    core->setBatteryCharging(atoi(args[1].c_str()));
    core->onDataChange();
    cmdExecuted=true;
  }
  if (cmdName=="setRemoteBattery") {
    core->setRemoteBatteryLevel(atoi(args[0].c_str()));
    core->setRemoteBatteryCharging(atoi(args[1].c_str()));
    core->onDataChange();
    cmdExecuted=true;
  }
  if (cmdName=="showMenu") {
    dispatch(cmd);
    cmdExecuted=true;
  }
  if (cmdName=="dataChanged") {
    core->onDataChange();
    cmdExecuted=true;
  }
  if (cmdName=="setGoogleBookmarksCookie") {
    core->getConfigStore()->setStringValue("GoogleBookmarksSync","cookies",args[0],__FILE__,__LINE__);
    core->getNavigationEngine()->triggerGoogleBookmarksSynchronization();
    cmdExecuted=true;
  }
  if (cmdName=="updateGoogleBookmarks") {
    core->getNavigationEngine()->triggerGoogleBookmarksSynchronization();
    cmdExecuted=true;
  }
  if (cmdName=="trashAddressPoint") {
    GraphicPosition visPos=*(core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__));
    core->getDefaultGraphicEngine()->unlockPos();
    NavigationPoint addressPoint;
    if (core->getNavigationEngine()->getAddressPoint(visPos,addressPoint)) {
      core->getNavigationEngine()->removeAddressPoint(addressPoint.getName());
    } else {
      WARNING("no address point near to the current map center found",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="askForRouteRemovalKind") {
    if (core->getIsInitialized()) {
      std::string appCmd = "askForRouteRemovalKind()";
      dispatch(appCmd);
    } else {
      WARNING("please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="setAmbientModeStartTime") {
    core->getDefaultGraphicEngine()->setAmbientModeStartTime(atol(args[0].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="setAmbientMode") {
    core->getDefaultGraphicEngine()->setAmbientMode(atoi(args[0].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="setWidgetlessMode") {
    //DEBUG("%s %s",cmdName.c_str(),args[0].c_str());
    core->getDefaultGraphicEngine()->setWidgetlessMode(atoi(args[0].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="getMapPos") {
    MapPosition *pos=core->getMapEngine()->lockMapPos(__FILE__,__LINE__);
    std::stringstream posStr;
    posStr << pos->getLat() << "," << pos->getLng();
    core->getMapEngine()->unlockMapPos();
    result=posStr.str();
    cmdExecuted=true;
  }
  if (cmdName=="getMapServerZoomLevel") {
    std::stringstream resultStream;
    resultStream<<core->getMapSource()->getServerZoomLevel(core->getMapEngine()->getZoomLevel());
    result=resultStream.str();
    cmdExecuted=true;
  }
  if (cmdName=="setNearestPOI") {
    core->getConfigStore()->setStringValue("Navigation/NearestPointOfInterest","name",args[0],__FILE__,__LINE__);
    core->getConfigStore()->setDoubleValue("Navigation/NearestPointOfInterest","lat",atof(args[1].c_str()),__FILE__,__LINE__);
    core->getConfigStore()->setDoubleValue("Navigation/NearestPointOfInterest","lng",atof(args[2].c_str()),__FILE__,__LINE__);
    core->onDataChange();
    cmdExecuted=true;
  }
  if (cmdName=="computeCRC") {
    crc.reset();
    crc.add((uint8_t*)args[0].c_str(),args[0].length());
    std::stringstream resultStream;
    resultStream << crc.getCRC();
    result=resultStream.str();
    cmdExecuted=true;
  }
  if (cmdName=="setNearestPOI") {
    core->getConfigStore()->setStringValue("Navigation/NearestPointOfInterest","name",args[0],__FILE__,__LINE__);
    core->getConfigStore()->setDoubleValue("Navigation/NearestPointOfInterest","lat",atof(args[1].c_str()),__FILE__,__LINE__);
    core->getConfigStore()->setDoubleValue("Navigation/NearestPointOfInterest","lng",atof(args[2].c_str()),__FILE__,__LINE__);
    core->onDataChange();
    cmdExecuted=true;
  }

  // Check if command has been executed
  if (!cmdExecuted) {
    ERROR("unknown command <%s>",cmd.c_str());
  }
  return result;
}

}
