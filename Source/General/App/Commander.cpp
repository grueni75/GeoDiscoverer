//============================================================================
// Name        : Commander.cpp
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
Commander::Commander() {
  pos=NULL;
  accessMutex=core->getThread()->createMutex();
  lastTouchedX=0;
  lastTouchedY=0;
}

// Destructor
Commander::~Commander() {
  core->getThread()->destroyMutex(accessMutex);
}

// Execute a command
std::string Commander::execute(std::string cmd, bool innerCall) {

  // Only one at a time
  if (!innerCall) core->getThread()->lockMutex(accessMutex);

  //DEBUG("executing command <%s>",cmd.c_str());
  TRACE(cmd.c_str(),NULL);
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();

  // Set the default result
  std::string result="";

  // Extract the command to execute
  Int argStart=cmd.find('(');
  if (argStart==std::string::npos) {
    ERROR("can not extract start of arguments from command <%s>",cmd.c_str());
    if (!innerCall) core->getThread()->unlockMutex(accessMutex);
    return "";
  }
  Int argEnd=cmd.find(')');
  if (argEnd==std::string::npos) {
    ERROR("can not extract end of arguments from command <%s>",cmd.c_str());
    if (!innerCall) core->getThread()->unlockMutex(accessMutex);
    return "";
  }
  std::string cmdName=cmd.substr(0,argStart);
  //DEBUG("cmdName=%s",cmdName.c_str());
  std::string cmdArgs=cmd.substr(argStart+1,argEnd-argStart-1);
  std::string unparsedCmdArgs=cmdArgs;
  //DEBUG("cmdArgs=%s",cmdArgs.c_str());
  std::vector<std::string> args;
  Int endPos;
  endPos=cmdArgs.find(',');
  if (endPos==std::string::npos) {
    endPos=cmdArgs.size();
  }
  while(endPos!=0) {
    std::string currentArg=cmdArgs.substr(0,endPos);
    //DEBUG("currentArg=%s",currentArg.c_str());
    args.push_back(currentArg);
    if (endPos!=cmdArgs.size()) {
      cmdArgs=cmdArgs.substr(endPos+1);
      //DEBUG("cmdArgs=%s",cmdArgs.c_str());
      endPos=cmdArgs.find(',');
      if (endPos==std::string::npos)
        endPos=cmdArgs.size();
    } else {
      endPos=0;
    }
  }

  // Handle the command
  bool cmdExecuted=false;
  //DEBUG("before: x=%d y=%d",pos->getX(),pos->getY());
  if (cmdName=="zoom") {
    pos=core->getGraphicEngine()->lockPos();
    pos->zoom(atof(args[0].c_str()));
    pos->updateLastUserModification();
    core->getGraphicEngine()->unlockPos();
    cmdExecuted=true;
  }
  if (cmdName=="pan") {
    pos=core->getGraphicEngine()->lockPos();
    pos->pan(atoi(args[0].c_str()),atoi(args[1].c_str()));
    pos->updateLastUserModification();
    core->getGraphicEngine()->unlockPos();
    cmdExecuted=true;
  }
  if (cmdName=="rotate") {
    pos=core->getGraphicEngine()->lockPos();
    pos->rotate(atof(args[0].c_str()));
    pos->updateLastUserModification();
    core->getGraphicEngine()->unlockPos();
    // Map is not update if rotation only, so we need to update screen graphic of navigation engine here
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->updateScreenGraphic(false);
    }
    cmdExecuted=true;
  }
  if (cmdName=="twoFingerGesture") {
    Int x,y;
    x=atoi(args[0].c_str());
    y=atoi(args[1].c_str());
    x=x-core->getScreen()->getWidth()/2;
    y=core->getScreen()->getHeight()/2-1-y;
    Int dX=lastTouchedX-x;
    Int dY=lastTouchedY-y;
    pos=core->getGraphicEngine()->lockPos();
    pos->rotate(atof(args[2].c_str()));
    pos->zoom(atof(args[3].c_str()));
    pos->pan(dX,dY);
    pos->updateLastUserModification();
    core->getGraphicEngine()->unlockPos();
    lastTouchedX=x;
    lastTouchedY=y;
    cmdExecuted=true;
  }
  if (cmdName.substr(0,5)=="touch") {
    Int x,y;
    x=atoi(args[0].c_str());
    y=atoi(args[1].c_str());

    // Convert the coordinates to screen coordinates
    x=x-core->getScreen()->getWidth()/2;
    y=core->getScreen()->getHeight()/2-1-y;

    // First check if a widget was touched
    bool widgetTouched=false;
    if ((cmdName=="touchDown")||(cmdName=="touchMove")) {
      //DEBUG("touchDown(%d,%d)",x,y);
      if (core->getWidgetEngine()->onTouchDown(t,x,y))
        widgetTouched=true;
    }
    if (cmdName=="touchUp") {
      //DEBUG("touchUp(%d,%d)",x,y);
      if (core->getWidgetEngine()->onTouchUp(t,x,y))
        widgetTouched=true;
    }

    // Then do the map scrolling
    if (!widgetTouched) {
      if ((cmdName=="touchMove")||(cmdName=="touchUp")) {
        Int dX=lastTouchedX-x;
        Int dY=lastTouchedY-y;
        //DEBUG("pan(%d,%d)",dX,dY);
        pos=core->getGraphicEngine()->lockPos();
        pos->pan(dX,dY);
        pos->updateLastUserModification();
        core->getGraphicEngine()->unlockPos();
      }
    }

    // Update some variables
    lastTouchedX=x;
    lastTouchedY=y;
    cmdExecuted=true;
  }
  //DEBUG("after: x=%d y=%d",pos->getX(),pos->getY());
  if (cmdName=="screenChanged") {
    GraphicScreenOrientation orientation=graphicScreenOrientationProtrait;
    if (args[0] == "landscape") {
      orientation=graphicScreenOrientationLandscape;
    }
    core->getScreen()->init(orientation,atoi(args[1].c_str()),atoi(args[2].c_str()));
    core->getWidgetEngine()->updateWidgetPositions();
    cmdExecuted=true;
  }
  if (cmdName=="graphicInvalidated") {
    core->updateGraphic(true);
    cmdExecuted=true;
  }
  if (cmdName=="createGraphic") {
    core->updateGraphic(false);
    cmdExecuted=true;
  }
  if (cmdName=="locationChanged") {
    if (core->getIsInitialized()) {

      // Get the fix
      MapPosition pos;
      pos.setSource(std::string(args[0]));
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
      core->getNavigationEngine()->newLocationFix(pos);
    }
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
      WARNING("Please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="getRecordTrack") {
    if (core->getIsInitialized()) {
      if (core->getConfigStore()->getIntValue("Navigation","recordTrack")) {
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
      WARNING("Please wait until map is loaded (command ignored)",NULL);
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
      core->maintenance(false);
    }
    cmdExecuted=true;
  }
  if (cmdName=="setWakeLock") {
    core->getScreen()->setWakeLock(atoi(args[0].c_str()));
    cmdExecuted=true;
  }
  if (cmdName=="getWakeLock") {
    if (core->getScreen()->getWakeLock()) {
      result="true";
    } else {
      result="false";
    }
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
  if (cmdName=="updateRoutes") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->updateRoutes();
    }
    cmdExecuted=true;
  }
  if (cmdName=="newPointOfInterest") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->newPointOfInterest("unknown","unknown",atof(args[0].c_str()),atof(args[1].c_str()));
    } else {
      WARNING("Please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="hideTarget") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->hideTarget();
    } else {
      WARNING("Please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="showTarget") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->showTarget(true);
      GraphicPosition *visPos=core->getGraphicEngine()->lockPos();
      visPos->updateLastUserModification();
      core->getGraphicEngine()->unlockPos();
    } else {
      WARNING("Please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="setTargetAtMapCenter") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->setTargetAtMapCenter();
      GraphicPosition *visPos=core->getGraphicEngine()->lockPos();
      visPos->updateLastUserModification();
      core->getGraphicEngine()->unlockPos();
    } else {
      WARNING("Please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="setTargetAtGeographicCoordinate") {
    if (core->getIsInitialized()) {
      core->getNavigationEngine()->setTargetAtGeographicCoordinate(atof(args[0].c_str()),atof(args[1].c_str()),true);
      GraphicPosition *visPos=core->getGraphicEngine()->lockPos();
      visPos->updateLastUserModification();
      core->getGraphicEngine()->unlockPos();
    } else {
      WARNING("Please wait until map is loaded (command ignored)",NULL);
    }
    cmdExecuted=true;
  }
  if (cmdName=="showContextMenu") {
    core->getWidgetEngine()->showContextMenu();
    cmdExecuted=true;
  }
  if ((cmdName=="newNavigationInfos")||(cmdName=="initComplete")) {
    core->getWidgetEngine()->showContextMenu();
    cmdExecuted=true;
  }

  // Check if command has been executed
  if (!cmdExecuted) {
    ERROR("unknown command <%s>",cmd.c_str());
  }
  if (!innerCall) core->getThread()->unlockMutex(accessMutex);
  return result;
}

}
