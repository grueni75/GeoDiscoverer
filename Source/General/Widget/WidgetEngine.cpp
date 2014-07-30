//============================================================================
// Name        : WidgetEngine.cpp
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
WidgetEngine::WidgetEngine() {

  // Get global config
  ConfigStore *c=core->getConfigStore();
  selectedWidgetColor=core->getConfigStore()->getGraphicColorValue("Graphic/Widget/SelectedColor",__FILE__, __LINE__);
  buttonRepeatDelay=c->getIntValue("Graphic/Widget","buttonRepeatDelay",__FILE__, __LINE__);
  buttonRepeatPeriod=c->getIntValue("Graphic/Widget","buttonRepeatPeriod",__FILE__, __LINE__);
  contextMenuDelay=c->getIntValue("Graphic/Widget","contextMenuDelay",__FILE__, __LINE__);
  contextMenuAllowedPixelJitter=c->getIntValue("Graphic/Widget","contextMenuAllowedPixelJitter",__FILE__, __LINE__);
  isTouched=false;
  contextMenuIsShown=false;
  currentPage=NULL;
  changePageDuration=c->getIntValue("Graphic/Widget","changePageDuration",__FILE__, __LINE__);
  ignoreTouchesEnd=0;
  widgetsActiveTimeout=c->getIntValue("Graphic/Widget","widgetsActiveTimeout",__FILE__, __LINE__);
  nearestPath=NULL;
  nearestPathIndex=-1;

  // Init the rest
  init();
}

// Destructor
WidgetEngine::~WidgetEngine() {
  deinit();
}

// Adds a widget to a page
void WidgetEngine::addWidgetToPage(
  std::string pageName,
  WidgetType widgetType,
  std::string widgetName,
  double portraitX, double portraitY, Int portraitZ,
  double landscapeX, double landscapeY, Int landscapeZ,
  UByte activeRed, UByte activeGreen, UByte activeBlue, UByte activeAlpha,
  UByte inactiveRed, UByte inactiveGreen, UByte inactiveBlue, UByte inactiveAlpha,
  ParameterMap parameters) {

  std::string path="Graphic/Widget/Page[@name='" + pageName + "']/Primitive[@name='" + widgetName + "']";
  ConfigStore *c=core->getConfigStore();
  std::string widgetTypeString="unknown";
  switch(widgetType) {
    case WidgetTypeButton: widgetTypeString="button"; break;
    case WidgetTypeCheckbox: widgetTypeString="checkbox"; break;
    case WidgetTypeMeter: widgetTypeString="meter"; break;
    case WidgetTypeScale: widgetTypeString="scale"; break;
    case WidgetTypeStatus: widgetTypeString="status"; break;
    case WidgetTypeNavigation: widgetTypeString="navigation"; break;
    case WidgetTypePathInfo: widgetTypeString="pathInfo"; break;
    default: FATAL("unknown widget type",NULL); break;
  }
  c->setStringValue(path,"type",widgetTypeString,__FILE__, __LINE__);
  c->setDoubleValue(path + "/Portrait","x",portraitX,__FILE__, __LINE__);
  c->setDoubleValue(path + "/Portrait","y",portraitY,__FILE__, __LINE__);
  c->setIntValue(path + "/Portrait","z",portraitZ,__FILE__, __LINE__);
  c->setDoubleValue(path + "/Landscape","x",landscapeX,__FILE__, __LINE__);
  c->setDoubleValue(path + "/Landscape","y",landscapeY,__FILE__, __LINE__);
  c->setIntValue(path + "/Landscape","z",landscapeZ,__FILE__, __LINE__);
  c->setGraphicColorValue(path + "/ActiveColor",GraphicColor(activeRed,activeGreen,activeBlue,activeAlpha),__FILE__, __LINE__);
  c->setGraphicColorValue(path + "/InactiveColor",GraphicColor(inactiveRed,inactiveGreen,inactiveBlue,inactiveAlpha),__FILE__, __LINE__);
  ParameterMap::iterator i;
  for (i=parameters.begin();i!=parameters.end();i++) {
    std::string innerPath = path;
    std::string name = i->first;
    size_t pos;
    while ((pos = name.find("/"))!=std::string::npos) {
      innerPath.append("/");
      innerPath.append(name.substr(0,pos));
      name=name.substr(pos+1);
    }
    c->setStringValue(innerPath, name, i->second, __FILE__, __LINE__);
  }
}

// Inits the engine
void WidgetEngine::init() {
}

// Clears all widget pages
void WidgetEngine::destroyGraphic() {
  deinit();
}

// (Re)creates all widget pages from the current config
void WidgetEngine::createGraphic() {

  ConfigStore *c=core->getConfigStore();

  // Only one thread please
  visiblePages.lockAccess(__FILE__, __LINE__);

  // Get all widget pages
  // If no exist, create the default ones
  std::list<std::string> pageNames=c->getAttributeValues("Graphic/Widget/Page","name",__FILE__, __LINE__);
  if (pageNames.size()==0) {
    ParameterMap parameters;
    parameters.clear();
    parameters["iconFilename"]="pageLeft";
    parameters["command"]="setPage(Path Tools,+1)";
    parameters["repeat"]="0";
    addWidgetToPage("Default",WidgetTypeButton,"Page Left",               3.5, 50.0,0, 3.0, 50.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="pageRight";
    parameters["command"]="setPage(Path Tools,-1)";
    parameters["repeat"]="0";
    addWidgetToPage("Default",WidgetTypeButton,"Page Right",             96.5, 50.0,0,97.0, 50.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="zoomIn";
    parameters["command"]="zoom(1.125)";
    parameters["repeat"]="1";
    addWidgetToPage("Default",WidgetTypeButton,"Zoom In",                87.5, 23.0,0,89.5, 87.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="zoomOut";
    parameters["command"]="zoom(0.875)";
    parameters["repeat"]="1";
    addWidgetToPage("Default",WidgetTypeButton,"Zoom Out",               62.5, 23.0,0,89.5, 62.5,0,255,255,255,255,255,255,255,100,parameters);
    /*
    parameters.clear();
    parameters["iconFilename"]="rotateLeft";
    parameters["command"]="rotate(+3)";
    parameters["repeat"]="1";
    addWidgetToPage("Default",WidgetTypeButton,"Rotate Left",            37.5, 23.0,0,89.5, 37.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="rotateRight";
    parameters["command"]="rotate(-3)";
    parameters["repeat"]="1";
    addWidgetToPage("Default",WidgetTypeButton,"Rotate Right",           12.5, 23.0,0,89.5, 12.5,0,255,255,255,255,255,255,255,100,parameters);
    */
    parameters.clear();
    parameters["uncheckedIconFilename"]="trackRecordingOff";
    parameters["uncheckedCommand"]="setRecordTrack(0)";
    parameters["checkedIconFilename"]="trackRecordingOn";
    parameters["checkedCommand"]="setRecordTrack(1)";
    parameters["stateConfigPath"]="Navigation";
    parameters["stateConfigName"]="recordTrack";
    parameters["updateInterval"]="250000";
    addWidgetToPage("Default",WidgetTypeCheckbox,"Track Recording",      84.0, 80.5,0,10.5, 37.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="createNewTrack";
    parameters["command"]="createNewTrack()";
    parameters["repeat"]="0";
    addWidgetToPage("Default",WidgetTypeButton,"Create New Track",       57.0, 80.5,0,10.5, 12.5,0,255,255,255,255,255,255,255,100,parameters);
    /*
    parameters.clear();
    parameters["uncheckedIconFilename"]="wakeLockOff";
    parameters["uncheckedCommand"]="setWakeLock(0)";
    parameters["checkedIconFilename"]="wakeLockOn";
    parameters["checkedCommand"]="setWakeLock(1)";
    parameters["stateConfigPath"]="General";
    parameters["stateConfigName"]="wakeLock";
    addWidgetToPage("Default",WidgetTypeCheckbox,"Wake Lock",            37.5, 93.0,0,10.5, 37.5,0,255,255,255,255,255,255,255,100,parameters);
    */
    parameters.clear();
    parameters["uncheckedIconFilename"]="returnToLocationOff";
    parameters["uncheckedCommand"]="setReturnToLocation(0)";
    parameters["checkedIconFilename"]="returnToLocationOn";
    parameters["checkedCommand"]="setReturnToLocation(1)";
    parameters["stateConfigPath"]="Map";
    parameters["stateConfigName"]="returnToLocation";
    parameters["updateInterval"]="250000";
    addWidgetToPage("Default",WidgetTypeCheckbox,"Return To Location",   37.5, 23.0,0,89.5, 37.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["uncheckedIconFilename"]="zoomLevelLockOff";
    parameters["uncheckedCommand"]="setZoomLevelLock(0)";
    parameters["checkedIconFilename"]="zoomLevelLockOn";
    parameters["checkedCommand"]="setZoomLevelLock(1)";
    parameters["stateConfigPath"]="Map";
    parameters["stateConfigName"]="zoomLevelLock";
    parameters["updateInterval"]="250000";
    addWidgetToPage("Default",WidgetTypeCheckbox,"Zoom Level Lock",      12.5, 23.0,0,89.5, 12.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="meterBackground";
    parameters["meterType"]="altitude";
    parameters["updateInterval"]="1000000";
    parameters["labelY"]="78.0";
    parameters["valueY"]="40.0";
    parameters["unitY"]="10.0";
    addWidgetToPage("Default",WidgetTypeMeter,"Current altitude",        17.0, 9.0,0,26.5, 14.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters["meterType"]="speed";
    addWidgetToPage("Default",WidgetTypeMeter,"Current speed",           50.0, 9.0,0,50.0, 14.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters["meterType"]="trackLength";
    addWidgetToPage("Default",WidgetTypeMeter,"Track length",            83.0, 9.0,0,73.5, 14.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="scale";
    parameters["updateInterval"]="1000000";
    parameters["tickLabelOffsetX"]="0";
    parameters["mapLabelOffsetY"]="9.0";
    addWidgetToPage("Default",WidgetTypeScale,"Map scale",               70.0, 92.0,0,50.0, 88.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="statusBackground";
    parameters["updateInterval"]="100000";
    parameters["labelWidth"]="95.0";
    addWidgetToPage("Default",WidgetTypeStatus,"Status",                 70.0, 92.0,1,50.0, 88.0,1,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="navigationBackground";
    parameters["updateInterval"]="1000000";
    parameters["durationLabelOffsetY"]="38";
    parameters["durationValueOffsetY"]="25";
    parameters["targetDistanceLabelOffsetY"]="68";
    parameters["targetDistanceValueOffsetY"]="55";
    parameters["turnDistanceValueOffsetY"]="25";
    parameters["directionIconFilename"]="navigationDirection";
    parameters["targetIconFilename"]="navigationTarget";
    parameters["separatorIconFilename"]="navigationSeparator";
    parameters["directionChangeDuration"]="500000";
    parameters["targetRadius"]="86.0";
    parameters["orientationLabelRadius"]="86.5";
    parameters["turnLineWidth"] = "15.0";
    parameters["turnLineArrowOverhang"] = "7.5";
    parameters["turnLineArrowHeight"] = "13.0";
    parameters["turnLineStartHeight"] = "20.0";
    parameters["turnLineMiddleHeight"] = "8.5";
    parameters["turnLineStartX"] = "50.0";
    parameters["turnLineStartY"] = "37.0";
    parameters["TurnColor/red"] = "0";
    parameters["TurnColor/green"] = "0";
    parameters["TurnColor/blue"] = "0";
    parameters["TurnColor/alpha"] = "255";
    addWidgetToPage("Default",WidgetTypeNavigation,"Navigation",         22.0, 87.0,0,14.5, 78.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="pageLeft";
    parameters["command"]="setPage(Default,+1)";
    parameters["repeat"]="0";
    addWidgetToPage("Path Tools",WidgetTypeButton,"Page Left",            3.5, 50.0,0, 3.0, 50.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="pageRight";
    parameters["command"]="setPage(Default,-1)";
    parameters["repeat"]="0";
    addWidgetToPage("Path Tools",WidgetTypeButton,"Page Right",          96.5, 50.0,0,97.0, 50.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="pathInfoBackground";
    parameters["pathNameOffsetX"]="3.0";
    parameters["pathNameOffsetY"]="82.0";
    parameters["pathNameWidth"]="60.0";
    parameters["pathValuesOffsetX"]="74.5";
    parameters["pathValuesWidth"]="22.5";
    parameters["pathLengthOffsetY"]="82.0";
    parameters["pathAltitudeUpOffsetY"]="57.25";
    parameters["pathAltitudeDownOffsetY"]="32.5";
    parameters["pathDurationOffsetY"]="8.25";
    parameters["altitudeProfileWidth"]="50.5";
    parameters["altitudeProfileHeight"]="54.0";
    parameters["altitudeProfileOffsetX"]="9.5";
    parameters["altitudeProfileOffsetY"]="14.0";
    parameters["altitudeProfileLineWidth"]="2.0";
    parameters["altitudeProfileAxisLineWidth"]="2.0";
    parameters["altitudeProfileMinAltitudeDiff"]="1.0";
    parameters["altitudeProfileXTickCount"]="5";
    parameters["altitudeProfileYTickCount"]="3";
    parameters["altitudeProfileXTickLabelOffsetY"]="1.5";
    parameters["altitudeProfileYTickLabelOffsetX"]="1.0";
    parameters["altitudeProfileXTickLabelWidth"]="5";
    parameters["altitudeProfileYTickLabelWidth"]="4";
    parameters["AltitudeProfileFillColor/red"] = "255";
    parameters["AltitudeProfileFillColor/green"] = "190";
    parameters["AltitudeProfileFillColor/blue"] = "127";
    parameters["AltitudeProfileFillColor/alpha"] = "255";
    parameters["AltitudeProfileLineColor/red"] = "255";
    parameters["AltitudeProfileLineColor/green"] = "127";
    parameters["AltitudeProfileLineColor/blue"] = "0";
    parameters["AltitudeProfileLineColor/alpha"] = "255";
    parameters["AltitudeProfileAxisColor/red"] = "0";
    parameters["AltitudeProfileAxisColor/green"] = "0";
    parameters["AltitudeProfileAxisColor/blue"] = "0";
    parameters["AltitudeProfileAxisColor/alpha"] = "64";
    parameters["noAltitudeProfileOffsetX"]="32.0";
    parameters["noAltitudeProfileOffsetY"]="42.0";
    parameters["locationIconFilename"]="pathInfoLocation";
    addWidgetToPage("Path Tools",WidgetTypePathInfo,"Path Info",         50.0, 12.5,1,50.0, 21.0,1,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["uncheckedIconFilename"]="targetOff";
    parameters["uncheckedCommand"]="hideTarget()";
    parameters["checkedIconFilename"]="targetOn";
    parameters["checkedCommand"]="showTarget()";
    parameters["stateConfigPath"]="Navigation/Target";
    parameters["stateConfigName"]="visible";
    parameters["updateInterval"]="250000";
    addWidgetToPage("Path Tools",WidgetTypeCheckbox,"Target Visibility", 87.5, 93.0,0,10.5, 87.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="setTargetAtAddress";
    parameters["command"]="setTargetAtAddress()";
    addWidgetToPage("Path Tools",WidgetTypeButton,"Target At Address",   62.5, 93.0,0,10.5, 62.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="setTargetAtMapCenter";
    parameters["command"]="setTargetAtMapCenter()";
    parameters["repeat"]="0";
    addWidgetToPage("Path Tools",WidgetTypeButton,"Target At Center",    37.5, 93.0,0,10.5, 37.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["uncheckedIconFilename"]="pathInfoLockOff";
    parameters["uncheckedCommand"]="setPathInfoLock(0)";
    parameters["checkedIconFilename"]="pathInfoLockOn";
    parameters["checkedCommand"]="setPathInfoLock(1)";
    parameters["stateConfigPath"]="Navigation";
    parameters["stateConfigName"]="pathInfoLocked";
    parameters["updateInterval"]="250000";
    addWidgetToPage("Path Tools",WidgetTypeCheckbox,"Path Info Lock",    12.5, 93.0,0,10.5, 12.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="statusBackground";
    parameters["updateInterval"]="100000";
    parameters["labelWidth"]="95.0";
    addWidgetToPage("Path Tools",WidgetTypeStatus,"Status",              50.0, 30.0,1,50.0, 88.0,1,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="setPathEndFlag";
    parameters["command"]="setPathEndFlag()";
    parameters["repeat"]="0";
    addWidgetToPage("Path Tools",WidgetTypeButton,"Set Path End Flag",  87.5, 80.0,0,89.5, 87.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="setPathStartFlag";
    parameters["command"]="setPathStartFlag()";
    parameters["repeat"]="0";
    addWidgetToPage("Path Tools",WidgetTypeButton,"Set Path Start Flag",62.5, 80.0,0,89.5, 62.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="setActiveRoute";
    parameters["command"]="setActiveRoute()";
    parameters["repeat"]="0";
    addWidgetToPage("Path Tools",WidgetTypeButton,"Set Active Route",   37.5, 80.0,0,89.5, 37.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["uncheckedIconFilename"]="zoomLevelLockOff";
    parameters["uncheckedCommand"]="setZoomLevelLock(0)";
    parameters["checkedIconFilename"]="zoomLevelLockOn";
    parameters["checkedCommand"]="setZoomLevelLock(1)";
    parameters["stateConfigPath"]="Map";
    parameters["stateConfigName"]="zoomLevelLock";
    parameters["updateInterval"]="250000";
    addWidgetToPage("Path Tools",WidgetTypeCheckbox,"Zoom Level Lock",   12.5, 80.0,0,89.5, 12.5,0,255,255,255,255,255,255,255,100,parameters);
    pageNames=c->getAttributeValues("Graphic/Widget/Page","name",__FILE__, __LINE__);
  }

  // Create the widgets from the config
  pageNames=c->getAttributeValues("Graphic/Widget/Page","name",__FILE__, __LINE__);
  std::list<std::string>::iterator i;
  for(i=pageNames.begin();i!=pageNames.end();i++) {
    //DEBUG("found a widget page with name %s",(*i).c_str());

    // Create the page
    WidgetPage *page=new WidgetPage(*i);
    if (!page) {
      FATAL("can not create widget page object",NULL);
      visiblePages.unlockAccess();
      return;
    }
    WidgetPagePair pair=WidgetPagePair(*i,page);
    pageMap.insert(pair);

    // Go through all widgets of this page
    std::string path="Graphic/Widget/Page[@name='" + *i + "']/Primitive";
    std::list<std::string> widgetNames=c->getAttributeValues(path,"name",__FILE__, __LINE__);
    std::list<std::string>::iterator j;
    for(j=widgetNames.begin();j!=widgetNames.end();j++) {

      // Create the type-specific widget
      std::string widgetPath=path + "[@name='" + *j + "']";
      std::string widgetType=c->getStringValue(widgetPath,"type",__FILE__, __LINE__);
      WidgetPrimitive *primitive;
      WidgetButton *button;
      WidgetCheckbox *checkbox;
      WidgetMeter *meter;
      WidgetScale *scale;
      WidgetStatus *status;
      WidgetNavigation *navigation;
      WidgetPathInfo *pathInfo;
      if (widgetType=="button") {
        button=new WidgetButton();
        primitive=button;
      }
      if (widgetType=="checkbox") {
        checkbox=new WidgetCheckbox();
        primitive=checkbox;
      }
      if (widgetType=="meter") {
        meter=new WidgetMeter();
        primitive=meter;
      }
      if (widgetType=="scale") {
        scale=new WidgetScale();
        primitive=scale;
      }
      if (widgetType=="status") {
        status=new WidgetStatus();
        primitive=status;
      }
      if (widgetType=="navigation") {
        navigation=new WidgetNavigation();
        primitive=navigation;
      }
      if (widgetType=="pathInfo") {
        pathInfo=new WidgetPathInfo();
        primitive=pathInfo;
      }

      // Set type-independent properties
      std::list<std::string> name;
      name.push_back(*j);
      primitive->setName(name);
      primitive->setActiveColor(c->getGraphicColorValue(widgetPath + "/ActiveColor",__FILE__, __LINE__));
      primitive->setInactiveColor(c->getGraphicColorValue(widgetPath + "/InactiveColor",__FILE__, __LINE__));
      primitive->setColor(primitive->getInactiveColor());

      // Load the image of the widget
      if ((widgetType=="button")||(widgetType=="meter")||(widgetType=="scale")||(widgetType=="status")||(widgetType=="navigation")||(widgetType=="pathInfo")) {
        primitive->setTextureFromIcon(c->getStringValue(widgetPath,"iconFilename",__FILE__, __LINE__));
      }
      if (widgetType=="checkbox") {
        primitive->setTextureFromIcon(c->getStringValue(widgetPath,"checkedIconFilename",__FILE__, __LINE__));
        checkbox->setCheckedTexture(primitive->getTexture());
        primitive->setTextureFromIcon(c->getStringValue(widgetPath,"uncheckedIconFilename",__FILE__, __LINE__));
        checkbox->setUncheckedTexture(primitive->getTexture());
        checkbox->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval",__FILE__, __LINE__));
      }
      if (widgetType=="navigation") {
        navigation->getDirectionIcon()->setTextureFromIcon(c->getStringValue(widgetPath,"directionIconFilename",__FILE__, __LINE__));
        navigation->getDirectionIcon()->setX(-navigation->getDirectionIcon()->getIconWidth()/2);
        navigation->getDirectionIcon()->setY(-navigation->getDirectionIcon()->getIconHeight()/2);
        navigation->getTargetIcon()->setTextureFromIcon(c->getStringValue(widgetPath,"targetIconFilename",__FILE__, __LINE__));
        navigation->getTargetIcon()->setX(-navigation->getTargetIcon()->getIconWidth()/2);
        navigation->getTargetIcon()->setY(-navigation->getTargetIcon()->getIconHeight()/2);
        navigation->getSeparatorIcon()->setTextureFromIcon(c->getStringValue(widgetPath,"separatorIconFilename",__FILE__, __LINE__));
        navigation->getSeparatorIcon()->setX(0);
        navigation->getSeparatorIcon()->setY(0);
      }

      // Set type-dependent properties
      if (widgetType=="button") {
        button->setCommand(c->getStringValue(widgetPath,"command",__FILE__, __LINE__));
        button->setRepeat(c->getIntValue(widgetPath,"repeat",__FILE__, __LINE__));
      }
      if (widgetType=="checkbox") {
        checkbox->setConfigPath(widgetPath);
        checkbox->setUncheckedCommand(c->getStringValue(widgetPath,"uncheckedCommand",__FILE__, __LINE__));
        checkbox->setCheckedCommand(c->getStringValue(widgetPath,"checkedCommand",__FILE__, __LINE__));
        checkbox->setStateConfigPath(c->getStringValue(widgetPath,"stateConfigPath",__FILE__, __LINE__));
        checkbox->setStateConfigName(c->getStringValue(widgetPath,"stateConfigName",__FILE__, __LINE__));
      }
      if (widgetType=="meter") {
        std::string meterType=c->getStringValue(widgetPath,"meterType",__FILE__, __LINE__);
        if (meterType=="altitude") {
          meter->setMeterType(WidgetMeterTypeAltitude);
        } else if (meterType=="speed") {
          meter->setMeterType(WidgetMeterTypeSpeed);
        } else if (meterType=="trackLength") {
          meter->setMeterType(WidgetMeterTypeTrackLength);
        } else {
          FATAL("unknown meter type",NULL);
          visiblePages.unlockAccess();
          return;
        }
        meter->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval",__FILE__, __LINE__));
        meter->setLabelY(c->getDoubleValue(widgetPath,"labelY",__FILE__, __LINE__)*meter->getIconHeight()/100.0);
        meter->setValueY(c->getDoubleValue(widgetPath,"valueY",__FILE__, __LINE__)*meter->getIconHeight()/100.0);
        meter->setUnitY(c->getDoubleValue(widgetPath,"unitY",__FILE__, __LINE__)*meter->getIconHeight()/100.0);
      }
      if (widgetType=="scale") {
        scale->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval",__FILE__, __LINE__));
        scale->setTickLabelOffsetX(c->getDoubleValue(widgetPath,"tickLabelOffsetX",__FILE__, __LINE__)*meter->getIconWidth()/100.0);
        scale->setMapLabelOffsetY(c->getDoubleValue(widgetPath,"mapLabelOffsetY",__FILE__, __LINE__)*meter->getIconHeight()/100.0);
      }
      if (widgetType=="status") {
        status->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval",__FILE__, __LINE__));
        status->setLabelWidth(c->getDoubleValue(widgetPath,"labelWidth",__FILE__, __LINE__)*status->getIconWidth()/100.0);
        GraphicColor c=status->getColor();
        c.setAlpha(0);
        status->setColor(c);
      }
      if (widgetType=="navigation") {
        navigation->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval",__FILE__, __LINE__));
        navigation->setDurationLabelOffsetY(c->getDoubleValue(widgetPath,"durationLabelOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setDurationValueOffsetY(c->getDoubleValue(widgetPath,"durationValueOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTargetDistanceLabelOffsetY(c->getDoubleValue(widgetPath,"targetDistanceLabelOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTargetDistanceValueOffsetY(c->getDoubleValue(widgetPath,"targetDistanceValueOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnDistanceValueOffsetY(c->getDoubleValue(widgetPath,"turnDistanceValueOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setDirectionChangeDuration(c->getDoubleValue(widgetPath,"directionChangeDuration",__FILE__, __LINE__));
        navigation->setTargetRadius(c->getDoubleValue(widgetPath,"targetRadius",__FILE__, __LINE__)*navigation->getIconHeight()/200.0);
        navigation->setOrientationLabelRadius(c->getDoubleValue(widgetPath,"orientationLabelRadius",__FILE__, __LINE__)*navigation->getIconHeight()/200.0);
        navigation->setTurnLineWidth(c->getDoubleValue(widgetPath,"turnLineWidth",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineArrowOverhang(c->getDoubleValue(widgetPath,"turnLineArrowOverhang",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineArrowHeight(c->getDoubleValue(widgetPath,"turnLineArrowHeight",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineStartHeight(c->getDoubleValue(widgetPath,"turnLineStartHeight",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineMiddleHeight(c->getDoubleValue(widgetPath,"turnLineMiddleHeight",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineStartX(c->getDoubleValue(widgetPath,"turnLineStartX",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineStartY(c->getDoubleValue(widgetPath,"turnLineStartY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnColor(c->getGraphicColorValue(widgetPath+"/TurnColor",__FILE__, __LINE__));
      }
      if (widgetType=="pathInfo") {
        pathInfo->setPathNameOffsetX(c->getDoubleValue(widgetPath,"pathNameOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathNameOffsetY(c->getDoubleValue(widgetPath,"pathNameOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathNameWidth(c->getDoubleValue(widgetPath,"pathNameWidth",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathValuesOffsetX(c->getDoubleValue(widgetPath,"pathValuesOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathValuesWidth(c->getDoubleValue(widgetPath,"pathValuesWidth",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathNameOffsetY(c->getDoubleValue(widgetPath,"pathNameOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathLengthOffsetY(c->getDoubleValue(widgetPath,"pathLengthOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathAltitudeUpOffsetY(c->getDoubleValue(widgetPath,"pathAltitudeUpOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathAltitudeDownOffsetY(c->getDoubleValue(widgetPath,"pathAltitudeDownOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathDurationOffsetY(c->getDoubleValue(widgetPath,"pathDurationOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setAltitudeProfileWidth(c->getDoubleValue(widgetPath,"altitudeProfileWidth",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setAltitudeProfileHeight(c->getDoubleValue(widgetPath,"altitudeProfileHeight",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setAltitudeProfileOffsetX(c->getDoubleValue(widgetPath,"altitudeProfileOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setAltitudeProfileOffsetY(c->getDoubleValue(widgetPath,"altitudeProfileOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setAltitudeProfileLineWidth(c->getDoubleValue(widgetPath,"altitudeProfileLineWidth",__FILE__, __LINE__)*((double)core->getScreen()->getDPI())/160.0);
        pathInfo->setAltitudeProfileAxisLineWidth(c->getDoubleValue(widgetPath,"altitudeProfileAxisLineWidth",__FILE__, __LINE__)*((double)core->getScreen()->getDPI())/160.0);
        pathInfo->setNoAltitudeProfileOffsetX(c->getDoubleValue(widgetPath,"noAltitudeProfileOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setNoAltitudeProfileOffsetY(c->getDoubleValue(widgetPath,"noAltitudeProfileOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setAltitudeProfileFillColor(c->getGraphicColorValue(widgetPath+"/AltitudeProfileFillColor",__FILE__, __LINE__));
        pathInfo->setAltitudeProfileLineColor(c->getGraphicColorValue(widgetPath+"/AltitudeProfileLineColor",__FILE__, __LINE__));
        pathInfo->setAltitudeProfileAxisColor(c->getGraphicColorValue(widgetPath+"/AltitudeProfileAxisColor",__FILE__, __LINE__));
        pathInfo->setAltitudeProfileMinAltitudeDiff(c->getDoubleValue(widgetPath,"altitudeProfileMinAltitudeDiff",__FILE__, __LINE__));
        pathInfo->setAltitudeProfileXTickCount(c->getIntValue(widgetPath,"altitudeProfileXTickCount",__FILE__, __LINE__));
        pathInfo->setAltitudeProfileYTickCount(c->getIntValue(widgetPath,"altitudeProfileYTickCount",__FILE__, __LINE__));
        pathInfo->setAltitudeProfileXTickLabelOffsetY(c->getDoubleValue(widgetPath,"altitudeProfileXTickLabelOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setAltitudeProfileYTickLabelOffsetX(c->getDoubleValue(widgetPath,"altitudeProfileYTickLabelOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setAltitudeProfileXTickLabelWidth(c->getIntValue(widgetPath,"altitudeProfileXTickLabelWidth",__FILE__, __LINE__));
        pathInfo->setAltitudeProfileYTickLabelWidth(c->getIntValue(widgetPath,"altitudeProfileYTickLabelWidth",__FILE__, __LINE__));
        pathInfo->getLocationIcon()->setTextureFromIcon(c->getStringValue(widgetPath,"locationIconFilename",__FILE__, __LINE__));
      }

      // Add the widget to the page
      page->addWidget(primitive);

    }

  }

  // Set the default page on the graphic engine
  WidgetPageMap::iterator j;
  j=pageMap.find(c->getStringValue("Graphic/Widget","selectedPage",__FILE__, __LINE__));
  if (j==pageMap.end()) {
    FATAL("default page does not exist",NULL);
    visiblePages.unlockAccess();
    return;
  } else {
    currentPage=j->second;
    visiblePages.addPrimitive(currentPage->getGraphicObject());
    core->getGraphicEngine()->setWidgetGraphicObject(&visiblePages);
  }

  // Allow access by the next thread
  visiblePages.unlockAccess();

  // Set the positions of the widgets
  updateWidgetPositions();
}

// Updates the positions of the widgets in dependence of the current screen dimension
void WidgetEngine::updateWidgetPositions() {

  // Only one thread please
  visiblePages.lockAccess(__FILE__, __LINE__);

  // Find out the orientation for which we need to update the positioning
  std::string orientation="Unknown";
  switch(core->getScreen()->getOrientation()) {
    case graphicScreenOrientationLandscape: orientation="Landscape"; break;
    case graphicScreenOrientationProtrait: orientation="Portrait"; break;
  }
  DEBUG("orientation=%s",orientation.c_str());
  Int width=core->getScreen()->getWidth();
  Int height=core->getScreen()->getHeight();

  // Set global variables that depend on the screen configuration
  changePageOvershoot=(Int)(core->getConfigStore()->getDoubleValue("Graphic/Widget","changePageOvershoot",__FILE__, __LINE__)*core->getScreen()->getWidth()/100.0);

  // Go through all pages
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  ConfigStore *c=core->getConfigStore();
  WidgetPageMap::iterator i;
  for(i=pageMap.begin();i!=pageMap.end();i++) {
    WidgetPage *page=i->second;

    // Go through all widgets
    std::list<WidgetPrimitive*> primitives;
    GraphicPrimitiveMap *widgetMap=page->getGraphicObject()->getPrimitiveMap();
    GraphicPrimitiveMap::iterator j;
    for(j=widgetMap->begin();j!=widgetMap->end();j++) {
      WidgetPrimitive *primitive=(WidgetPrimitive*)j->second;

      // Update the position
      std::string path="Graphic/Widget/Page[@name='" + page->getName() + "']/Primitive[@name='" + primitive->getName().front() + "']/" + orientation;
      primitive->updatePosition(width*c->getDoubleValue(path,"x",__FILE__, __LINE__)/100.0-width/2-primitive->getIconWidth()/2,height*c->getDoubleValue(path,"y",__FILE__, __LINE__)/100.0-height/2-primitive->getIconHeight()/2,c->getIntValue(path,"z",__FILE__, __LINE__));
      primitives.push_back(primitive);

    }

    // Re-add the widget to the graphic object to get them sorted
    page->deinit(false);
    for(std::list<WidgetPrimitive*>::iterator i=primitives.begin();i!=primitives.end();i++) {
      page->addWidget(*i);
    }

  }

  // Allow access by the next thread
  visiblePages.unlockAccess();
}

// Clears all widget pages
void WidgetEngine::deinit() {

  // Only one thread
  visiblePages.lockAccess(__FILE__, __LINE__);

  // No page is active
  currentPage=NULL;

  // Clear the graphic object
  visiblePages.deinit(false);

  // Delete all pages
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    delete i->second;
  }
  pageMap.clear();

  // Allow access by the next thread
  visiblePages.unlockAccess();
}

// Called when the screen is touched
bool WidgetEngine::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {

  // Only one thread please
  visiblePages.lockAccess(__FILE__, __LINE__);

  // Shall we ignore touches?
  if (t<=ignoreTouchesEnd) {
    visiblePages.unlockAccess();
    return false;
  }

  // First check if a widget on the page was touched
  if (currentPage->onTouchDown(t,x,y)) {
    isTouched=false;
    visiblePages.unlockAccess();
    return true;
  } else {

    // Check if we should show the context menu
    if (isTouched) {
      if ((abs(lastTouchDownX-x)<=contextMenuAllowedPixelJitter)&&(abs(lastTouchDownY-y)<=contextMenuAllowedPixelJitter)) {
        if (t-firstStableTouchDownTime >= contextMenuDelay) {
          if (!contextMenuIsShown) {
            //showContextMenu();
            contextMenuIsShown=true;
          }
        }
      } else {
        firstStableTouchDownTime=t;
      }
    } else {
      isTouched=true;
      firstStableTouchDownTime=t;
    }
    lastTouchDownX=x;
    lastTouchDownY=y;
  }

  // Allow access by the next thread
  visiblePages.unlockAccess();

  return false;
}

// Called when the screen is untouched
bool WidgetEngine::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {

  visiblePages.lockAccess(__FILE__, __LINE__);
  if (t<=ignoreTouchesEnd) {
    visiblePages.unlockAccess();
    return false;
  }
  deselectPage();
  bool result = currentPage->onTouchUp(t,x,y);
  visiblePages.unlockAccess();
  return result;
}

// Called when the screen is touched
bool WidgetEngine::onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff) {

  // Only one thread please
  visiblePages.lockAccess(__FILE__, __LINE__);

  // Shall we ignore touches?
  if (t<=ignoreTouchesEnd) {
    visiblePages.unlockAccess();
    return false;
  }

  // First check if a widget on the page was touched
  if (currentPage->onTwoFingerGesture(t,dX,dY,angleDiff,scaleDiff)) {
    visiblePages.unlockAccess();
    return true;
  }

  // Allow access by the next thread
  visiblePages.unlockAccess();

  return false;
}

// Deselects the currently selected page
void WidgetEngine::deselectPage() {
  isTouched=false;
  contextMenuIsShown=false;
}

// Sets a new page
void WidgetEngine::setPage(std::string name, Int direction) {

  visiblePages.lockAccess(__FILE__, __LINE__);

  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  Int width = core->getScreen()->getWidth();

  // Check if the requested page exists
  if (pageMap.find(name)==pageMap.end()) {
    WARNING("page <%s> does not exist",name.c_str());
    visiblePages.unlockAccess();
    return;
  }
  WidgetPage *nextPage = pageMap[name];

  // Deselect widget in current page
  deselectPage();
  currentPage->deselectWidget(t);

  // Compute the durations for the two animation steps
  double changePageDurationStep1=changePageDuration/2;
  double changePageDurationStep2=changePageDuration/2;

  // Let the current page move outside the window
  std::list<GraphicTranslateAnimationParameter> translateAnimationSequence;
  GraphicObject *prevPageGraphicObject=currentPage->getGraphicObject();
  GraphicTranslateAnimationParameter translateParameter;
  translateParameter.setStartTime(t);
  translateParameter.setStartX(0);
  translateParameter.setStartY(0);
  translateParameter.setEndX(direction*(width+changePageOvershoot));
  translateParameter.setEndY(0);
  translateParameter.setDuration(changePageDurationStep1);
  translateParameter.setInfinite(false);
  translateParameter.setAnimationType(GraphicTranslateAnimationTypeAccelerated);
  translateAnimationSequence.push_back(translateParameter);
  prevPageGraphicObject->setTranslateAnimationSequence(translateAnimationSequence);
  prevPageGraphicObject->setLifeEnd(t+changePageDurationStep1);

  // Let the new page move inside the window
  translateAnimationSequence.clear();
  GraphicObject *nextPageGraphicObject=nextPage->getGraphicObject();
  translateParameter.setStartTime(t);
  translateParameter.setStartX(-1*direction*(width));
  translateParameter.setStartY(0);
  translateParameter.setEndX(+1*direction*(changePageOvershoot));
  translateParameter.setEndY(0);
  translateParameter.setDuration(changePageDurationStep1);
  translateParameter.setInfinite(false);
  translateParameter.setAnimationType(GraphicTranslateAnimationTypeAccelerated);
  translateAnimationSequence.push_back(translateParameter);
  translateParameter.setStartTime(t+changePageDurationStep1);
  translateParameter.setStartX(+1*direction*(changePageOvershoot));
  translateParameter.setStartY(0);
  translateParameter.setEndX(0);
  translateParameter.setEndY(0);
  translateParameter.setDuration(changePageDurationStep2);
  translateParameter.setInfinite(false);
  translateParameter.setAnimationType(GraphicTranslateAnimationTypeAccelerated);
  translateAnimationSequence.push_back(translateParameter);
  nextPageGraphicObject->setTranslateAnimationSequence(translateAnimationSequence);
  nextPageGraphicObject->setLifeEnd(0);
  visiblePages.addPrimitive(nextPageGraphicObject);

  // Ignore any touches during transition
  ignoreTouchesEnd=t+changePageDurationStep1+changePageDurationStep2;

  // Set the new page
  currentPage=nextPage;
  nextPage->setWidgetsActive(t,true);
  core->getConfigStore()->setStringValue("Graphic/Widget","selectedPage",name,__FILE__, __LINE__);
  visiblePages.unlockAccess();;

}

// Informs the engine that the map has changed
void WidgetEngine::onMapChange(MapPosition mapPos, std::list<MapTile*> *centerMapTiles) {

  // Find the nearest path in the currently visible map tile
  nearestPath=NULL;
  double minDistance=std::numeric_limits<double>::max();
  for(std::list<MapTile*>::iterator j=centerMapTiles->begin();j!=centerMapTiles->end();j++) {
    std::list<NavigationPathSegment*> nearbyPathSegments;
    nearbyPathSegments=(*j)->getCrossingNavigationPathSegments();
    for(std::list<NavigationPathSegment*>::iterator i=nearbyPathSegments.begin();i!=nearbyPathSegments.end();i++) {
      NavigationPathSegment *s=*i;
      s->getPath()->lockAccess(__FILE__, __LINE__);
      for(Int j=s->getStartIndex();j<=s->getEndIndex();j++) {
        MapPosition pathPos=s->getPath()->getPoint(j);
        double d=mapPos.computeDistance(pathPos);
        if (d<minDistance) {
          minDistance=d;
          nearestPath=s->getPath();
          nearestPathIndex=j;
        }
      }
      s->getPath()->unlockAccess();
    }
  }

  // Inform the widget
  visiblePages.lockAccess(__FILE__, __LINE__);
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    i->second->onMapChange(currentPage==i->second ? true : false, mapPos);
  }
  visiblePages.unlockAccess();
}

// Informs the engine that the location has changed
void WidgetEngine::onLocationChange(MapPosition mapPos) {
  visiblePages.lockAccess(__FILE__, __LINE__);
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    i->second->onLocationChange(currentPage==i->second ? true : false, mapPos);
  }
  visiblePages.unlockAccess();
}

// Informs the engine that a path has changed
void WidgetEngine::onPathChange(NavigationPath *path, NavigationPathChangeType changeType) {
  visiblePages.lockAccess(__FILE__, __LINE__);
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    i->second->onPathChange(currentPage==i->second ? true : false, path, changeType);
  }
  visiblePages.unlockAccess();
}

// Sets the widgets of the current page active
void WidgetEngine::setWidgetsActive(bool widgetsActive) {
  visiblePages.lockAccess(__FILE__, __LINE__);
  if (currentPage) {
    currentPage->setWidgetsActive(core->getClock()->getMicrosecondsSinceStart(),widgetsActive);
  }
  visiblePages.unlockAccess();
}

// Let the engine work
bool WidgetEngine::work(TimestampInMicroseconds t) {
  if (currentPage) {
    return currentPage->work(t);
  } else {
    return false;
  }
}

}
