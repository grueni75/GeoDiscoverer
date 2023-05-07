//============================================================================
// Name        : WidgetEngine.cpp
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
#include <WidgetEngine.h>
#include <Device.h>
#include <WidgetButton.h>
#include <WidgetCheckbox.h>
#include <WidgetCursorInfo.h>
#include <WidgetEBike.h>
#include <WidgetForumslader.h>
#include <WidgetFingerMenu.h>
#include <WidgetMeter.h>
#include <WidgetNavigation.h>
#include <WidgetPathInfo.h>
#include <WidgetScale.h>
#include <WidgetStatus.h>
#include <WidgetAddressPoint.h>
#include <GraphicEngine.h>
#include <NavigationEngine.h>
#include <MapEngine.h>
#include <NavigationPath.h>
#include <NavigationPathSegment.h>

namespace GEODISCOVERER {

// Constructor
WidgetEngine::WidgetEngine(Device *device) : 
  visiblePages(device->getScreen())
{

  // Get global config
  ConfigStore *c=core->getConfigStore();
  this->device=device;
  selectedWidgetColor=core->getConfigStore()->getGraphicColorValue("Graphic/Widget/SelectedColor",__FILE__, __LINE__);
  buttonRepeatDelay=c->getIntValue("Graphic/Widget","buttonRepeatDelay",__FILE__, __LINE__);
  buttonLongPressDelay=c->getIntValue("Graphic/Widget","buttonLongPressDelay",__FILE__, __LINE__);
  buttonRepeatPeriod=c->getIntValue("Graphic/Widget","buttonRepeatPeriod",__FILE__, __LINE__);
  maxPathDistance=c->getDoubleValue("Graphic/Widget","maxPathDistance",__FILE__, __LINE__)*device->getDPI();
  enableFingerMenu=c->getIntValue("Graphic/Widget/FingerMenu","enable",__FILE__, __LINE__);
  if (device->getName()!="Default")
    enableFingerMenu=false;
  isTouched=false;
  currentPage=NULL;
  changePageDuration=c->getIntValue("Graphic/Widget","changePageDuration",__FILE__, __LINE__);
  ignoreTouchesEnd=0;
  widgetsActiveTimeout=c->getIntValue("Graphic/Widget","widgetsActiveTimeout",__FILE__, __LINE__);
  nearestPath=NULL;
  nearestPathIndex=-1;
  accessMutex=core->getThread()->createMutex("widget engine access mutex");
  readAccessCount=0;
  fingerMenu=NULL;
  if (enableFingerMenu) {
    if (!(fingerMenu=new WidgetFingerMenu(this))) {
      FATAL("can not create finger menu object",NULL);
    }
  }

  // Init the rest
  init();
}

// Destructor
WidgetEngine::~WidgetEngine() {
  deinit();
  core->getThread()->destroyMutex(accessMutex);
}

// Adds a widget to a page
void WidgetEngine::addWidgetToPage(WidgetConfig config) {

  // Iterate through all positions
  std::string path="Graphic/Widget/Device[@name='" + device->getName() + "']/Page[@name='" + config.getPageName() + "']/Primitive[@name='" + config.getName() + "']";
  ConfigStore *c=core->getConfigStore();
  std::string widgetTypeString="unknown";
  switch(config.getType()) {
    case WidgetTypeButton: widgetTypeString="button"; break;
    case WidgetTypeCheckbox: widgetTypeString="checkbox"; break;
    case WidgetTypeMeter: widgetTypeString="meter"; break;
    case WidgetTypeScale: widgetTypeString="scale"; break;
    case WidgetTypeStatus: widgetTypeString="status"; break;
    case WidgetTypeNavigation: widgetTypeString="navigation"; break;
    case WidgetTypePathInfo: widgetTypeString="pathInfo"; break;
    case WidgetTypeCursorInfo: widgetTypeString="cursorInfo"; break;
    case WidgetTypeAddressPoint: widgetTypeString="addressPoint"; break;
    case WidgetTypeEBike: widgetTypeString="eBike"; break;
    case WidgetTypeForumslader: widgetTypeString="forumslader"; break;
    default: FATAL("unknown widget type",NULL); break;
  }
  c->setStringValue(path,"type",widgetTypeString,__FILE__, __LINE__);
  for(std::list<WidgetPosition>::iterator i=config.getPositions()->begin();i!=config.getPositions()->end();i++) {
    std::stringstream positionPath;
    positionPath << path << "/Position[@refScreenDiagonal='" << i->getRefScreenDiagonal() << "']";
    c->setDoubleValue(positionPath.str() + "/Portrait","x",i->getPortraitX(),__FILE__, __LINE__);
    c->setDoubleValue(positionPath.str() + "/Portrait","y",i->getPortraitY(),__FILE__, __LINE__);
    c->setDoubleValue(positionPath.str() + "/Portrait","xHidden",i->getPortraitXHidden(),__FILE__, __LINE__);
    c->setDoubleValue(positionPath.str() + "/Portrait","yHidden",i->getPortraitYHidden(),__FILE__, __LINE__);
    c->setIntValue(positionPath.str() + "/Portrait","z",i->getPortraitZ(),__FILE__, __LINE__);
    c->setDoubleValue(positionPath.str() + "/Landscape","x",i->getLandscapeX(),__FILE__, __LINE__);
    c->setDoubleValue(positionPath.str() + "/Landscape","y",i->getLandscapeY(),__FILE__, __LINE__);
    c->setDoubleValue(positionPath.str() + "/Landscape","xHidden",i->getLandscapeXHidden(),__FILE__, __LINE__);
    c->setDoubleValue(positionPath.str() + "/Landscape","yHidden",i->getLandscapeYHidden(),__FILE__, __LINE__);
    c->setIntValue(positionPath.str() + "/Landscape","z",i->getLandscapeZ(),__FILE__, __LINE__);
  }
  c->setGraphicColorValue(path + "/ActiveColor",GraphicColor(config.getActiveColor().getRed(),config.getActiveColor().getGreen(),config.getActiveColor().getBlue(),config.getActiveColor().getAlpha()),__FILE__, __LINE__);
  c->setGraphicColorValue(path + "/InactiveColor",GraphicColor(config.getInactiveColor().getRed(),config.getInactiveColor().getGreen(),config.getInactiveColor().getBlue(),config.getInactiveColor().getAlpha()),__FILE__, __LINE__);
  if (config.getHasSafetyColor()) {
    c->setGraphicColorValue(path + "/SafetyColor",GraphicColor(config.getSafetyColor().getRed(),config.getSafetyColor().getGreen(),config.getSafetyColor().getBlue(),config.getSafetyColor().getAlpha()),__FILE__, __LINE__);
  }
  if (config.getType()==WidgetTypeNavigation)
    c->setGraphicColorValue(path + "/BusyColor",GraphicColor(config.getBusyColor().getRed(),config.getBusyColor().getGreen(),config.getBusyColor().getBlue(),config.getBusyColor().getAlpha()),__FILE__, __LINE__);
  if ((config.getType()==WidgetTypeEBike)||(config.getType()==WidgetTypeForumslader)) {
    c->setGraphicColorValue(path + "/GaugeBackgroundColor",GraphicColor(config.getGaugeBackgroundColor().getRed(),config.getGaugeBackgroundColor().getGreen(),config.getGaugeBackgroundColor().getBlue(),config.getGaugeBackgroundColor().getAlpha()),__FILE__, __LINE__);
    c->setGraphicColorValue(path + "/GaugeForegroundColor",GraphicColor(config.getGaugeForegroundColor().getRed(),config.getGaugeForegroundColor().getGreen(),config.getGaugeForegroundColor().getBlue(),config.getGaugeForegroundColor().getAlpha()),__FILE__, __LINE__);
  }
  if (config.getType()==WidgetTypeForumslader) {
    c->setGraphicColorValue(path + "/GaugeFillgroundColor",GraphicColor(config.getGaugeFillgroundColor().getRed(),config.getGaugeFillgroundColor().getGreen(),config.getGaugeFillgroundColor().getBlue(),config.getGaugeFillgroundColor().getAlpha()),__FILE__, __LINE__);
  }
  ParameterMap::iterator i;
  for (i=config.getParameters()->begin();i!=config.getParameters()->end();i++) {
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

  // Wait until all read accesses are over
  lockAccessAfterReadComplete(__FILE__,__LINE__);

  // Get all widget pages
  // If no exist, create the default ones
  std::string deviceName = device->getName();
  std::list<std::string> pageNames=c->getAttributeValues("Graphic/Widget/Device[@name='" + deviceName + "']/Page","name",__FILE__, __LINE__);
  WidgetConfig config;
  WidgetPosition position;
  if (pageNames.size()==0) {
    double tabletPortraitButtonGridX[4];
    for (Int i=0;i<4;i++) {
      tabletPortraitButtonGridX[i] = 88.0 - i*12.0;
    }
    double tabletPortraitButtonGridY[5];
    for (Int i=0;i<5;i++) {
      tabletPortraitButtonGridY[i] = 5.0 + i*7.5;
    }
    double tabletPortraitMeterGridX[4];
    for (Int i=0;i<4;i++) {
      tabletPortraitMeterGridX[i] = 12.5 + i*20;
    }
    double tabletPortraitMeterGridY=92.0;
    double tabletLandscapeButtonGridX[8];
    for (Int i=0;i<8;i++) {
      tabletLandscapeButtonGridX[i] = 95.0 - i*7.5;
    }
    double tabletLandscapeButtonGridY[5];
    for (Int i=0;i<5;i++) {
      tabletLandscapeButtonGridY[i] = 7.5 + i*12.0;
    }
    double tabletLandscapeMeterGridX[4];
    for (Int i=0;i<4;i++) {
      tabletLandscapeMeterGridX[i] = 7.5 + i*12.5;
    }
    double tabletLandscapeMeterGridY=86.0;
    // ---------------------------------------------------------
    if (deviceName=="Default") {
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Page Left");
      config.setType(WidgetTypeButton);
      position=WidgetPosition();
      position.setRefScreenDiagonal(0);
      position.setPortraitX(3.5);
      position.setPortraitY(50.0);
      position.setPortraitZ(0);
      position.setLandscapeX(3.0);
      position.setLandscapeY(50.0);
      position.setLandscapeZ(0);
      config.addPosition(position);
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setInactiveColor(GraphicColor(255,255,255,100));
      config.setParameter("iconFilename","pageLeft");
      config.setParameter("command","setPage(Path Tools,+1)");
      config.setParameter("longPressCommand","showMenu()");
      config.setParameter("repeat","0");
      addWidgetToPage(config);
      config.setPageName("Path Tools");
      config.setParameter("command","setPage(Default,+1)");
      addWidgetToPage(config);
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Page Right");
      config.setType(WidgetTypeButton);
      position=WidgetPosition();
      position.setRefScreenDiagonal(0);
      position.setPortraitX(96.5);
      position.setPortraitY(50.0);
      position.setPortraitZ(0);
      position.setLandscapeX(97.0);
      position.setLandscapeY(50.0);
      position.setLandscapeZ(0);
      config.addPosition(position);
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setInactiveColor(GraphicColor(255,255,255,100));
      config.setParameter("iconFilename","pageRight");
      config.setParameter("command","setPage(Path Tools,-1)");
      config.setParameter("longPressCommand","showMenu()");
      config.setParameter("repeat","0");
      addWidgetToPage(config);
      config.setPageName("Path Tools");
      config.setParameter("command","setPage(Default,-1)");
      addWidgetToPage(config);
    }
    // ---------------------------------------------------------
    if ((deviceName=="Default")||(deviceName=="Watch")) {
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Zoom In");
      config.setType(WidgetTypeButton);
      position=WidgetPosition();
      if (deviceName=="Default") {
        position.setRefScreenDiagonal(4);
        position.setPortraitX(87.5);
        position.setPortraitY(23.0);
        position.setPortraitZ(0);
        position.setLandscapeX(89.5);
        position.setLandscapeY(87.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7);
        position.setPortraitX(tabletPortraitButtonGridX[0]);
        position.setPortraitY(tabletPortraitButtonGridY[0]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[0]);
        position.setLandscapeY(tabletLandscapeButtonGridY[0]);
        position.setLandscapeZ(0);
        config.addPosition(position);
      }
      if (deviceName=="Watch") {
        position.setRefScreenDiagonal(0);
        position.setPortraitX(90.0);
        position.setPortraitY(65.0);
        position.setPortraitXHidden(150.0);
        position.setPortraitYHidden(position.getPortraitY());
        position.setPortraitZ(1);
        position.setLandscapeX(position.getPortraitX());
        position.setLandscapeY(position.getPortraitY());
        position.setLandscapeY(position.getPortraitXHidden());
        position.setLandscapeY(position.getPortraitYHidden());
        position.setLandscapeZ(position.getPortraitZ());
        config.addPosition(position);
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      if (deviceName=="Watch") {
        config.setInactiveColor(GraphicColor(255,255,255,0));
      } else {
        config.setInactiveColor(GraphicColor(255,255,255,100));
      }
      config.setParameter("iconFilename","zoomIn");
      config.setParameter("command","zoom(1.02)");
      config.setParameter("repeat","1");
      addWidgetToPage(config);
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Zoom Out");
      config.setType(WidgetTypeButton);
      position=WidgetPosition();
      if (deviceName=="Default") {
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(62.5);
        position.setPortraitY(23.0);
        position.setPortraitZ(0);
        position.setLandscapeX(89.5);
        position.setLandscapeY(62.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[1]);
        position.setPortraitY(tabletPortraitButtonGridY[0]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[1]);
        position.setLandscapeY(tabletLandscapeButtonGridY[0]);
        position.setLandscapeZ(0);
        config.addPosition(position);
      }
      if (deviceName=="Watch") {
        position.setRefScreenDiagonal(0);
        position.setPortraitX(90.0);
        position.setPortraitY(35.0);
        position.setPortraitXHidden(150.0);
        position.setPortraitYHidden(position.getPortraitY());
        position.setPortraitZ(1);
        position.setLandscapeX(position.getPortraitX());
        position.setLandscapeY(position.getPortraitY());
        position.setLandscapeY(position.getPortraitXHidden());
        position.setLandscapeY(position.getPortraitYHidden());
        position.setLandscapeZ(position.getPortraitZ());
        config.addPosition(position);
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      if (deviceName=="Watch") {
        config.setInactiveColor(GraphicColor(255,255,255,0));
      } else {
        config.setInactiveColor(GraphicColor(255,255,255,100));
      }
      config.setParameter("iconFilename","zoomOut");
      config.setParameter("command","zoom(0.98)");
      config.setParameter("repeat","1");
      addWidgetToPage(config);
    }
    if (deviceName=="Default") {
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Track Recording");
      config.setType(WidgetTypeCheckbox);
      position=WidgetPosition();
      position.setRefScreenDiagonal(4.0);
      position.setPortraitX(84.0);
      position.setPortraitY(79.0);
      position.setPortraitZ(0);
      position.setLandscapeX(10.5);
      position.setLandscapeY(37.5);
      position.setLandscapeZ(0);
      config.addPosition(position);
      position=WidgetPosition();
      position.setRefScreenDiagonal(7.0);
      position.setPortraitX(tabletPortraitButtonGridX[0]);
      position.setPortraitY(tabletPortraitButtonGridY[1]);
      position.setPortraitZ(0);
      position.setLandscapeX(tabletLandscapeButtonGridX[0]);
      position.setLandscapeY(tabletLandscapeButtonGridY[1]);
      position.setLandscapeZ(0);
      config.addPosition(position);
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setInactiveColor(GraphicColor(255,255,255,100));
      config.setParameter("uncheckedIconFilename","trackRecordingOff");
      config.setParameter("uncheckedCommand","setRecordTrack(0)");
      config.setParameter("checkedIconFilename","trackRecordingOn");
      config.setParameter("checkedCommand","decideContinueOrNewTrack()");
      config.setParameter("stateConfigPath","Navigation");
      config.setParameter("stateConfigName","recordTrack");
      config.setParameter("updateInterval","250000");
      addWidgetToPage(config);
      config.setPageName("Finger Menu");
      addWidgetToPage(config);
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Change Map Layer");
      config.setType(WidgetTypeButton);
      position=WidgetPosition();
      position.setRefScreenDiagonal(4.0);
      position.setPortraitX(57.0);
      position.setPortraitY(79.0);
      position.setPortraitZ(0);
      position.setLandscapeX(10.5);
      position.setLandscapeY(12.5);
      position.setLandscapeZ(0);
      config.addPosition(position);
      position=WidgetPosition();
      position.setRefScreenDiagonal(7.0);
      position.setPortraitX(tabletPortraitButtonGridX[1]);
      position.setPortraitY(tabletPortraitButtonGridY[1]);
      position.setPortraitZ(0);
      position.setLandscapeX(tabletLandscapeButtonGridX[1]);
      position.setLandscapeY(tabletLandscapeButtonGridY[1]);
      position.setLandscapeZ(0);
      config.addPosition(position);
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setInactiveColor(GraphicColor(255,255,255,100));
      config.setParameter("iconFilename","changeMapLayer");
      config.setParameter("command","changeMapLayer()");
      config.setParameter("repeat","0");
      addWidgetToPage(config);
      config.setPageName("Finger Menu");
      addWidgetToPage(config);
      // ---------------------------------------------------------
    }
    if ((deviceName!="Watch")) {
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Address Point");
      config.setType(WidgetTypeAddressPoint);
      position=WidgetPosition();
      if (deviceName!="Default") {
        position.setRefScreenDiagonal(0.0);
        position.setPortraitX(0.0);
        position.setPortraitY(0.0);
        position.setPortraitZ(1);
        position.setLandscapeX(62.0);
        position.setLandscapeY(70.5);
        position.setLandscapeZ(1);
      } else {
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(50.0);
        position.setPortraitY(70.0);
        position.setPortraitZ(1);
        position.setLandscapeX(50.0);
        position.setLandscapeY(32.5);
        position.setLandscapeZ(1);
      }
      config.addPosition(position);
      config.setInactiveColor(GraphicColor(255,255,255,255));
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setParameter("iconFilename","addressPointBackground");
      addWidgetToPage(config);
      config.clearPositions();
      position.setLandscapeY(87.5);
      config.addPosition(position);
      config.setPageName("Path Tools");
      addWidgetToPage(config);
      if (deviceName=="Default") {
        config.setPageName("Finger Menu");
        config.clearPositions();
        position.setPortraitY(75.0);
        position.setLandscapeY(93.0);
        config.addPosition(position);
        addWidgetToPage(config);
      }
      // ---------------------------------------------------------
    }
    if ((deviceName=="Default")||(deviceName=="Watch")) {
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Cursor Info");
      config.setType(WidgetTypeCursorInfo);
      position=WidgetPosition();
      position.setRefScreenDiagonal(4.0);
      position.setPortraitX(50.0);
      if (deviceName=="Watch") {  
        position.setPortraitY(31.0);
      } else {
        position.setPortraitY(45.0);
      }
      position.setPortraitZ(0.5);
      position.setLandscapeX(50.0);
      position.setLandscapeY(41.0);
      position.setLandscapeZ(0.5);
      config.addPosition(position);
      position=WidgetPosition();
      position.setRefScreenDiagonal(7.0);
      position.setPortraitX(50.0);
      position.setPortraitY(47.0);
      position.setPortraitZ(0.5);
      position.setLandscapeX(50.0);
      position.setLandscapeY(45.0);
      position.setLandscapeZ(0.5);
      config.addPosition(position);
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setInactiveColor(GraphicColor(255,255,255,150));
      if (deviceName=="Watch") {  
        config.setParameter("width","0.7");
      } else {
        config.setParameter("width","2.0");
      }
      config.setParameter("height","0.2");
      addWidgetToPage(config);
      if (deviceName=="Default") {
        config.setPageName("Path Tools");
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
      }
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Return To Location");
      config.setType(WidgetTypeCheckbox);
      if (deviceName=="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(37.5);
        position.setPortraitY(23.0);
        position.setPortraitZ(0);
        position.setLandscapeX(89.5);
        position.setLandscapeY(37.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[2]);
        position.setPortraitY(tabletPortraitButtonGridY[0]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[2]);
        position.setLandscapeY(tabletLandscapeButtonGridY[0]);
        position.setLandscapeZ(0);
        config.addPosition(position);
      }
      if (deviceName=="Watch") {
        position.setRefScreenDiagonal(0);
        position.setPortraitX(10.0);
        position.setPortraitY(65.0);
        position.setPortraitXHidden(-50.0);
        position.setPortraitYHidden(position.getPortraitY());
        position.setPortraitZ(1);
        position.setLandscapeX(position.getPortraitX());
        position.setLandscapeY(position.getPortraitY());
        position.setLandscapeY(position.getPortraitXHidden());
        position.setLandscapeY(position.getPortraitYHidden());
        position.setLandscapeZ(position.getPortraitZ());
        config.addPosition(position);
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      if (deviceName=="Watch") {
        config.setInactiveColor(GraphicColor(255,255,255,0));
      } else {
        config.setInactiveColor(GraphicColor(255,255,255,100));
      }
      if (deviceName=="Watch") {
        config.setParameter("uncheckedIconFilename","returnToLocationOffRightHand");
        config.setParameter("checkedIconFilename","returnToLocationOnRightHand");
      } else {
        config.setParameter("uncheckedIconFilename","returnToLocationOff");
        config.setParameter("checkedIconFilename","returnToLocationOn");
      }
      config.setParameter("uncheckedCommand","setReturnToLocation(0)");
      config.setParameter("checkedCommand","setReturnToLocation(1)");
      config.setParameter("stateConfigPath","Map");
      config.setParameter("stateConfigName","returnToLocation");
      config.setParameter("updateInterval","250000");
      addWidgetToPage(config);
      if (deviceName=="Default") {
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
      }
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Zoom Level Lock");
      config.setType(WidgetTypeCheckbox);
      if (deviceName=="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(12.5);
        position.setPortraitY(23.0);
        position.setPortraitZ(0);
        position.setLandscapeX(89.5);
        position.setLandscapeY(12.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[2]);
        position.setPortraitY(tabletPortraitButtonGridY[1]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[2]);
        position.setLandscapeY(tabletLandscapeButtonGridY[1]);
        position.setLandscapeZ(0);
        config.addPosition(position);
      }
      if (deviceName=="Watch") {
        position.setRefScreenDiagonal(0);
        position.setPortraitX(10.0);
        position.setPortraitY(35.0);
        position.setPortraitXHidden(-50.0);
        position.setPortraitYHidden(position.getPortraitY());
        position.setPortraitZ(1);
        position.setLandscapeX(position.getPortraitX());
        position.setLandscapeY(position.getPortraitY());
        position.setLandscapeY(position.getPortraitXHidden());
        position.setLandscapeY(position.getPortraitYHidden());
        position.setLandscapeZ(position.getPortraitZ());
        config.addPosition(position);
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      if (deviceName=="Watch") {
        config.setInactiveColor(GraphicColor(255,255,255,0));
      } else {
        config.setInactiveColor(GraphicColor(255,255,255,100));
      }
      config.setParameter("uncheckedIconFilename","zoomLevelLockOff");
      config.setParameter("uncheckedCommand","setZoomLevelLock(0)");
      config.setParameter("checkedIconFilename","zoomLevelLockOn");
      config.setParameter("checkedCommand","setZoomLevelLock(1)");
      config.setParameter("stateConfigPath","Map");
      config.setParameter("stateConfigName","zoomLevelLock");
      config.setParameter("updateInterval","250000");
      addWidgetToPage(config);
      if (deviceName=="Default") {
        config.setPageName("Path Tools");
        config.clearPositions();
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(12.5);
        position.setPortraitY(93.0);
        position.setPortraitZ(0);
        position.setLandscapeX(10.5);
        position.setLandscapeY(12.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[3]);
        position.setPortraitY(tabletPortraitButtonGridY[1]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[3]);
        position.setLandscapeY(tabletLandscapeButtonGridY[1]);
        position.setLandscapeZ(0);
        config.addPosition(position);
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
      }
    }
    // ---------------------------------------------------------
    if (deviceName!="Watch") {
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Current altitude");
      config.setType(WidgetTypeMeter);
      if (deviceName=="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(17.0);
        position.setPortraitY(9.0);
        position.setPortraitZ(0);
        position.setLandscapeX(26.5);
        position.setLandscapeY(14.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitMeterGridX[1]);
        position.setPortraitY(tabletPortraitMeterGridY);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeMeterGridX[1]);
        position.setLandscapeY(tabletLandscapeMeterGridY);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,100));
      }
      if (deviceName!="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(0.0);
        position.setPortraitX(0.0);
        position.setPortraitY(0.0);
        position.setPortraitZ(0);
        position.setLandscapeX(36.5);
        position.setLandscapeY(87.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,255));
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setParameter("iconFilename","meterBackground");
      config.setParameter("meterType","altitude");
      config.setParameter("updateInterval","1000000");
      config.setParameter("labelY","78.0");
      config.setParameter("valueY","40.0");
      config.setParameter("unitY","10.0");
      addWidgetToPage(config);
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Current speed");
      config.setType(WidgetTypeMeter);
      if (deviceName=="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(50.0);
        position.setPortraitY(9.0);
        position.setPortraitZ(0);
        position.setLandscapeX(50.0);
        position.setLandscapeY(14.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitMeterGridX[2]);
        position.setPortraitY(tabletPortraitMeterGridY);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeMeterGridX[2]);
        position.setLandscapeY(tabletLandscapeMeterGridY);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,100));
      }
      if (deviceName!="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(0.0);
        position.setPortraitX(0.0);
        position.setPortraitY(0.0);
        position.setPortraitZ(0);
        position.setLandscapeX(81.0);
        position.setLandscapeY(87.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,255));
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setParameter("iconFilename","meterBackground");
      config.setParameter("meterType","speed");
      config.setParameter("updateInterval","1000000");
      config.setParameter("labelY","78.0");
      config.setParameter("valueY","40.0");
      config.setParameter("unitY","10.0");
      addWidgetToPage(config);
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("EBike");
      config.setType(WidgetTypeEBike);
      if (deviceName=="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(50.0);
        position.setPortraitY(9.0);
        position.setPortraitZ(1);
        position.setLandscapeX(50.0);
        position.setLandscapeY(14.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitMeterGridX[2]);
        position.setPortraitY(tabletPortraitMeterGridY);
        position.setPortraitZ(1);
        position.setLandscapeX(tabletLandscapeMeterGridX[2]);
        position.setLandscapeY(tabletLandscapeMeterGridY);
        position.setLandscapeZ(1);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,100));
      } else {
        position=WidgetPosition();
        position.setRefScreenDiagonal(0.0);
        position.setPortraitX(0.0);
        position.setPortraitY(0.0);
        position.setPortraitZ(1);
        position.setLandscapeX(81.0);
        position.setLandscapeY(87.0);
        position.setLandscapeZ(1);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,255));
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setGaugeBackgroundColor(GraphicColor(255,127,0,255));
      config.setGaugeForegroundColor(GraphicColor(255,190,127,255));
      config.setParameter("iconFilename", "eBikeBackground");
      config.setParameter("powerLevelOffsetX","35");
      config.setParameter("powerLevelOffsetY","50");
      config.setParameter("engineTemperatureOffsetX","80");
      config.setParameter("engineTemperatureOffsetY","25");
      config.setParameter("engineTemperatureBackgroundRadius","13");
      config.setParameter("engineTemperatureBackgroundWidth","15");
      config.setParameter("engineTemperatureForegroundRadius","80");
      config.setParameter("engineTemperatureForegroundWidth","60");
      config.setParameter("engineTemperatureMaxHeight","50");
      config.setParameter("distanceElectricOffsetX","35");
      config.setParameter("distanceElectricOffsetY","25");
      config.setParameter("batteryLevelRadius","25");
      config.setParameter("batteryLevelBackgroundWidth","15");
      config.setParameter("batteryLevelForegroundWidth","60");
      addWidgetToPage(config);
      if (deviceName=="Default") {
        config.clearPositions();
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(22.0);
        position.setPortraitY(75.0);
        position.setPortraitZ(1);
        position.setLandscapeX(10.0);
        position.setLandscapeY(46.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
      }
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setName("Forumslader");
      config.setType(WidgetTypeForumslader);
      if (deviceName=="Default") {
        config.setPageName("Finger Menu");
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(70.0);
        position.setPortraitY(89.0);
        position.setPortraitZ(1);
        position.setLandscapeX(86.0);
        position.setLandscapeY(78.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,100));
      } else {
        config.setPageName("Default");
        position=WidgetPosition();
        position.setRefScreenDiagonal(0.0);
        position.setPortraitX(0.0);
        position.setPortraitY(0.0);
        position.setPortraitZ(1);
        position.setLandscapeX(81.0);
        position.setLandscapeY(87.0);
        position.setLandscapeZ(1);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,255));
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setGaugeBackgroundColor(GraphicColor(255,127,0,255));
      config.setGaugeFillgroundColor(GraphicColor(0.9*255,0.9*127,0,255));
      config.setGaugeForegroundColor(GraphicColor(255,190,127,255));
      config.setParameter("iconFilename", "forumsladerBackground");
      config.setParameter("powerDrawLevelOffsetY","12");
      config.setParameter("powerDrawGaugeOffsetY","35");
      config.setParameter("powerDrawGaugeRadius","0.2");
      config.setParameter("powerDrawGaugeBackgroundWidth","0.14");
      config.setParameter("powerDrawGaugeForegroundWidth","0.08");
      config.setParameter("batteryLevelOffsetY","12");
      config.setParameter("batteryGaugeOffsetX","35");
      config.setParameter("batteryGaugeOffsetY","35");
      config.setParameter("batteryGaugeMaxHeight","33");
      config.setParameter("batteryGaugeBackgroundWidth","0.15");
      config.setParameter("batteryGaugeForegroundWidth","0.09");
      config.setParameter("batteryGaugeTipWidth","0.08");
      config.setParameter("batteryGaugeTipHeight","0.05");
      config.setParameter("batteryIconChargingFilename","charging");
      addWidgetToPage(config);
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Track length");
      config.setType(WidgetTypeMeter);
      if (deviceName=="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(83.0);
        position.setPortraitY(9.0);
        position.setPortraitZ(0);
        position.setLandscapeX(73.5);
        position.setLandscapeY(14.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitMeterGridX[3]);
        position.setPortraitY(tabletPortraitMeterGridY);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeMeterGridX[3]);
        position.setLandscapeY(tabletLandscapeMeterGridY);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,100));
      }
      if (deviceName!="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(0.0);
        position.setPortraitX(0.0);
        position.setPortraitY(0.0);
        position.setPortraitZ(0);
        position.setLandscapeX(55.0);
        position.setLandscapeY(87.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,255));
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setParameter("iconFilename","meterBackground");
      config.setParameter("meterType","trackLength");
      config.setParameter("updateInterval","1000000");
      config.setParameter("labelY","78.0");
      config.setParameter("valueY","40.0");
      config.setParameter("unitY","10.0");
      addWidgetToPage(config);
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Default");
      config.setName("Status");
      config.setType(WidgetTypeStatus);
      position=WidgetPosition();
      if (deviceName!="Default") {
        position.setRefScreenDiagonal(0.0);
        position.setPortraitX(0.0);
        position.setPortraitY(0.0);
        position.setPortraitZ(2);
        position.setLandscapeX(50.0);
        position.setLandscapeY(26.0);
        position.setLandscapeZ(2);
      } else {
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(70.0);
        position.setPortraitY(92.0);
        position.setPortraitZ(2);
        position.setLandscapeX(50.0);
        position.setLandscapeY(88.0);
        position.setLandscapeZ(2);
      }
      config.addPosition(position);
      if (deviceName=="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[1]);
        position.setPortraitY(tabletPortraitButtonGridY[3]);
        position.setPortraitZ(2);
        position.setLandscapeX(tabletLandscapeButtonGridX[1]);
        position.setLandscapeY(tabletLandscapeButtonGridY[2]);
        position.setLandscapeZ(2);
        config.addPosition(position);
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setInactiveColor(GraphicColor(255,255,255,100));
      config.setParameter("iconFilename","statusBackground");
      config.setParameter("updateInterval","100000");
      config.setParameter("labelWidth","95.0");
      addWidgetToPage(config);
      config.setPageName("Path Tools");
      config.clearPositions();
      position=WidgetPosition();
      position.setRefScreenDiagonal(4.0);
      position.setPortraitX(50.0);
      position.setPortraitY(35.0);
      position.setPortraitZ(2);
      position.setLandscapeX(50.0);
      position.setLandscapeY(88.0);
      position.setLandscapeZ(2);
      config.addPosition(position);
      position=WidgetPosition();
      position.setRefScreenDiagonal(7.0);
      position.setPortraitX((tabletPortraitButtonGridX[2]+tabletPortraitButtonGridX[1])/2);
      position.setPortraitY(tabletPortraitButtonGridY[4]);
      position.setPortraitZ(2);
      position.setLandscapeX((tabletLandscapeButtonGridX[2]+tabletLandscapeButtonGridX[1])/2);
      position.setLandscapeY(tabletLandscapeButtonGridY[2]);
      position.setLandscapeZ(2);
      config.addPosition(position);
      addWidgetToPage(config);
      if (deviceName=="Default") {
        config.setPageName("Finger Menu");
        config.clearPositions();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(50.0);
        position.setPortraitY(75.0);
        position.setPortraitZ(2);
        position.setLandscapeX(50.0);
        position.setLandscapeY(93.0);
        position.setLandscapeZ(2);
        config.addPosition(position);
        addWidgetToPage(config);
      }
      // ---------------------------------------------------------
      if (deviceName=="Default") {
        config=WidgetConfig();
        config.setPageName("Default");
        config.setName("Map Scale");
        config.setType(WidgetTypeScale);
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(70.0);
        position.setPortraitY(90.0);
        position.setPortraitZ(0);
        position.setLandscapeX(50.0);
        position.setLandscapeY(84.0);
        position.setLandscapeZ(0);      
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[1]);
        position.setPortraitY(tabletPortraitButtonGridY[2]-1.0);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[1]);
        position.setLandscapeY(tabletLandscapeButtonGridY[2]-1.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setActiveColor(GraphicColor(255,255,255,255));
        config.setInactiveColor(GraphicColor(255,255,255,100));
        config.setSafetyColor(GraphicColor(255,255,255,150));
        config.setParameter("iconFilename","scale");
        config.setParameter("updateInterval","1000000");
        config.setParameter("topLabelOffsetY","+150.0");
        config.setParameter("bottomLabelOffsetY","-130.0");
        config.setParameter("width","70");
        config.setParameter("height","480");
        config.setParameter("alpha","150");
        config.setParameter("altitudeIconFilename","mountain");
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        config.clearPositions();
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(76.0);
        position.setPortraitY(89.0);
        position.setPortraitZ(0);
        position.setLandscapeX(88.5);
        position.setLandscapeY(78.0);
        position.setLandscapeZ(0);      
        config.addPosition(position);
        addWidgetToPage(config);
      }
      // ---------------------------------------------------------
      config=WidgetConfig();
      config.setPageName("Path Tools");
      config.setName("Path Info");
      config.setType(WidgetTypePathInfo);
      if (deviceName=="Default") {
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(50.0);
        position.setPortraitY(14.0);
        position.setPortraitZ(0);
        position.setLandscapeX(50.0);
        position.setLandscapeY(23.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX((tabletPortraitButtonGridX[2]+tabletPortraitButtonGridX[1])/2);
        position.setPortraitY((tabletPortraitButtonGridY[2]+tabletPortraitButtonGridY[3])/2);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[6]);
        position.setLandscapeY((tabletLandscapeButtonGridY[0]+tabletLandscapeButtonGridY[1])/2);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,100));
        config.setParameter("iconFilename","pathInfoBackground");
        config.setParameter("pathNameOffsetX","3.0");
        config.setParameter("pathNameOffsetY","85.5");
        config.setParameter("pathNameWidth","94.0");
        config.setParameter("pathValuesWidth","20.0");
        config.setParameter("pathLengthOffsetX","78.0");
        config.setParameter("pathLengthOffsetY","65.5");
        config.setParameter("pathAltitudeUpOffsetX","78.0");
        config.setParameter("pathAltitudeUpOffsetY","45.5");
        config.setParameter("pathAltitudeDownOffsetX","78.0");
        config.setParameter("pathAltitudeDownOffsetY","26.0");
        config.setParameter("pathDurationOffsetX","78.0");
        config.setParameter("pathDurationOffsetY","6.5");
        config.setParameter("altitudeProfileWidth","55.0");
        config.setParameter("altitudeProfileHeightWithNavigationPoints","47.5");
        config.setParameter("altitudeProfileHeightWithoutNavigationPoints","56.0");
        config.setParameter("altitudeProfileOffsetX","9.5");
        config.setParameter("altitudeProfileOffsetY","14.0");
        config.setParameter("noAltitudeProfileOffsetX","36.0");
        config.setParameter("noAltitudeProfileOffsetY","43.0");
        config.setParameter("altitudeProfileXTickCount","5");
        config.setParameter("altitudeProfileYTickCount","3");
      }
      if (deviceName!="Default") {
        config.setPageName("Default");
        position=WidgetPosition();
        position.setRefScreenDiagonal(0.0);
        position.setPortraitX(0.0);
        position.setPortraitY(0.0);
        position.setPortraitZ(0);
        position.setLandscapeX(50.0);
        position.setLandscapeY(33.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setInactiveColor(GraphicColor(255,255,255,255));
        config.setParameter("iconFilename","pathInfoLargeBackground");
        config.setParameter("pathNameOffsetX","2.0");
        config.setParameter("pathNameOffsetY","92.0");
        config.setParameter("pathNameWidth","96.0");
        config.setParameter("pathValuesWidth","18.5");
        config.setParameter("pathLengthOffsetX","5.5");
        config.setParameter("pathLengthOffsetY","80.5");
        config.setParameter("pathAltitudeUpOffsetX","30.0");
        config.setParameter("pathAltitudeUpOffsetY","80.5");
        config.setParameter("pathAltitudeDownOffsetX","55.0");
        config.setParameter("pathAltitudeDownOffsetY","80.5");
        config.setParameter("pathDurationOffsetX","80.0");
        config.setParameter("pathDurationOffsetY","80.5");
        config.setParameter("altitudeProfileWidth","88.5");
        config.setParameter("altitudeProfileHeightWithNavigationPoints","57.0");
        config.setParameter("altitudeProfileHeightWithoutNavigationPoints","61.0");
        config.setParameter("altitudeProfileOffsetX","7.5");
        config.setParameter("altitudeProfileOffsetY","9.5");
        config.setParameter("noAltitudeProfileOffsetX","50.0");
        config.setParameter("noAltitudeProfileOffsetY","39.0");
        config.setParameter("altitudeProfileXTickCount","7");
        config.setParameter("altitudeProfileYTickCount","5");
      }
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setParameter("altitudeProfileXTickLabelOffsetY","1.5");
      config.setParameter("altitudeProfileYTickLabelOffsetX","1.0");
      config.setParameter("altitudeProfileXTickLabelWidth","5");
      config.setParameter("altitudeProfileYTickLabelWidth","4");
      config.setParameter("altitudeProfileLineWidth","2.0");
      config.setParameter("altitudeProfileAxisLineWidth","2.0");
      config.setParameter("altitudeProfileMinAltitudeDiff","20.0");
      config.setParameter("AltitudeProfileFillColor/red","255");
      config.setParameter("AltitudeProfileFillColor/green","190");
      config.setParameter("AltitudeProfileFillColor/blue","127");
      config.setParameter("AltitudeProfileFillColor/alpha","255");
      config.setParameter("AltitudeProfileLineColor/red","255");
      config.setParameter("AltitudeProfileLineColor/green","127");
      config.setParameter("AltitudeProfileLineColor/blue","0");
      config.setParameter("AltitudeProfileLineColor/alpha","255");
      config.setParameter("AltitudeProfileAxisColor/red","0");
      config.setParameter("AltitudeProfileAxisColor/green","0");
      config.setParameter("AltitudeProfileAxisColor/blue","0");
      config.setParameter("AltitudeProfileAxisColor/alpha","64");
      config.setParameter("locationIconFilename","pathInfoLocation");
      config.setParameter("navigationPointIconFilename","pathInfoNavigationPoint");
      addWidgetToPage(config);
      if (deviceName=="Default") {
        config.clearPositions();
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(50.0);
        position.setPortraitY(14.0);
        position.setPortraitZ(0);
        position.setLandscapeX(50.0);
        position.setLandscapeY(23.0);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
      }
      // ---------------------------------------------------------
      if (deviceName=="Default") {
        config=WidgetConfig();
        config.setPageName("Path Tools");
        config.setName("Target Visibility");
        config.setType(WidgetTypeCheckbox);
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(87.5);
        position.setPortraitY(93.0);
        position.setPortraitZ(0);
        position.setLandscapeX(10.5);
        position.setLandscapeY(87.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[0]);
        position.setPortraitY(tabletPortraitButtonGridY[1]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[0]);
        position.setLandscapeY(tabletLandscapeButtonGridY[1]);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setActiveColor(GraphicColor(255,255,255,255));
        config.setInactiveColor(GraphicColor(255,255,255,100));
        config.setParameter("uncheckedIconFilename","targetOff");
        config.setParameter("uncheckedCommand","hideTarget()");
        config.setParameter("checkedIconFilename","targetOn");
        config.setParameter("checkedCommand","showTarget()");
        config.setParameter("stateConfigPath","Navigation/Target");
        config.setParameter("stateConfigName","visible");
        config.setParameter("updateInterval","250000");
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
        // ---------------------------------------------------------
        config=WidgetConfig();
        config.setPageName("Path Tools");
        config.setName("Target At Address");
        config.setType(WidgetTypeButton);
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(62.5);
        position.setPortraitY(93.0);
        position.setPortraitZ(0);
        position.setLandscapeX(10.5);
        position.setLandscapeY(62.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[1]);
        position.setPortraitY(tabletPortraitButtonGridY[1]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[1]);
        position.setLandscapeY(tabletLandscapeButtonGridY[1]);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setActiveColor(GraphicColor(255,255,255,255));
        config.setInactiveColor(GraphicColor(255,255,255,100));
        config.setParameter("iconFilename","setTargetAtAddress");
        config.setParameter("command","setTargetAtAddress()");
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
        // ---------------------------------------------------------
        config=WidgetConfig();
        config.setPageName("Path Tools");
        config.setName("Target At Center");
        config.setType(WidgetTypeButton);
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(37.5);
        position.setPortraitY(93.0);
        position.setPortraitZ(0);
        position.setLandscapeX(10.5);
        position.setLandscapeY(37.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[2]);
        position.setPortraitY(tabletPortraitButtonGridY[1]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[2]);
        position.setLandscapeY(tabletLandscapeButtonGridY[1]);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setActiveColor(GraphicColor(255,255,255,255));
        config.setInactiveColor(GraphicColor(255,255,255,100));
        config.setParameter("iconFilename","setTargetAtMapCenter");
        config.setParameter("command","setTargetAtMapCenter()");
        config.setParameter("repeat","0");
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
        // ---------------------------------------------------------
        config=WidgetConfig();
        config.setPageName("Path Tools");
        config.setName("Set Path End Flag");
        config.setType(WidgetTypeButton);
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(87.5);
        position.setPortraitY(80.0);
        position.setPortraitZ(0);
        position.setLandscapeX(89.5);
        position.setLandscapeY(87.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[0]);
        position.setPortraitY(tabletPortraitButtonGridY[0]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[0]);
        position.setLandscapeY(tabletLandscapeButtonGridY[0]);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setActiveColor(GraphicColor(255,255,255,255));
        config.setInactiveColor(GraphicColor(255,255,255,100));
        config.setParameter("iconFilename","setPathEndFlag");
        config.setParameter("command","setPathEndFlag()");
        config.setParameter("longPressCommand","resetPathEndFlag()");
        config.setParameter("repeat","0");
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
        // ---------------------------------------------------------
        config=WidgetConfig();
        config.setPageName("Path Tools");
        config.setName("Set Path Start Flag");
        config.setType(WidgetTypeButton);
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(62.5);
        position.setPortraitY(80.0);
        position.setPortraitZ(0);
        position.setLandscapeX(89.5);
        position.setLandscapeY(62.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[1]);
        position.setPortraitY(tabletPortraitButtonGridY[0]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[1]);
        position.setLandscapeY(tabletLandscapeButtonGridY[0]);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setActiveColor(GraphicColor(255,255,255,255));
        config.setInactiveColor(GraphicColor(255,255,255,100));
        config.setParameter("iconFilename","setPathStartFlag");
        config.setParameter("command","setPathStartFlag()");
        config.setParameter("longPressCommand","resetPathStartFlag()");
        config.setParameter("repeat","0");
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
        // ---------------------------------------------------------
        config=WidgetConfig();
        config.setPageName("Path Tools");
        config.setName("Set Active Route");
        config.setType(WidgetTypeButton);
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(37.5);
        position.setPortraitY(80.0);
        position.setPortraitZ(0);
        position.setLandscapeX(89.5);
        position.setLandscapeY(37.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[2]);
        position.setPortraitY(tabletPortraitButtonGridY[0]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[2]);
        position.setLandscapeY(tabletLandscapeButtonGridY[0]);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setActiveColor(GraphicColor(255,255,255,255));
        config.setInactiveColor(GraphicColor(255,255,255,100));
        config.setParameter("iconFilename","setActiveRoute");
        config.setParameter("command","setActiveRoute()");
        config.setParameter("repeat","0");
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
        // ---------------------------------------------------------
        config=WidgetConfig();
        config.setPageName("Path Tools");
        config.setName("Path Info Lock");
        config.setType(WidgetTypeCheckbox);
        position=WidgetPosition();
        position.setRefScreenDiagonal(4.0);
        position.setPortraitX(12.5);
        position.setPortraitY(80.0);
        position.setPortraitZ(0);
        position.setLandscapeX(89.5);
        position.setLandscapeY(12.5);
        position.setLandscapeZ(0);
        config.addPosition(position);
        position=WidgetPosition();
        position.setRefScreenDiagonal(7.0);
        position.setPortraitX(tabletPortraitButtonGridX[3]);
        position.setPortraitY(tabletPortraitButtonGridY[0]);
        position.setPortraitZ(0);
        position.setLandscapeX(tabletLandscapeButtonGridX[3]);
        position.setLandscapeY(tabletLandscapeButtonGridY[0]);
        position.setLandscapeZ(0);
        config.addPosition(position);
        config.setActiveColor(GraphicColor(255,255,255,255));
        config.setInactiveColor(GraphicColor(255,255,255,100));
        config.setParameter("uncheckedIconFilename","pathInfoLockOff");
        config.setParameter("uncheckedCommand","setPathInfoLock(0)");
        config.setParameter("checkedIconFilename","pathInfoLockOn");
        config.setParameter("checkedCommand","setPathInfoLock(1)");
        config.setParameter("stateConfigPath","Navigation");
        config.setParameter("stateConfigName","pathInfoLocked");
        config.setParameter("updateInterval","250000");
        addWidgetToPage(config);
        config.setPageName("Finger Menu");
        addWidgetToPage(config);
      }
    }
    // ---------------------------------------------------------
    config=WidgetConfig();
    config.setPageName("Default");
    config.setName("Navigation");
    config.setType(WidgetTypeNavigation);
    if (deviceName=="Default") {
      position=WidgetPosition();
      position.setRefScreenDiagonal(4.0);
      position.setPortraitX(22.0);
      position.setPortraitY(89.0);
      position.setPortraitZ(0);
      position.setLandscapeX(10.0);
      position.setLandscapeY(78.0);
      position.setLandscapeZ(0);
      config.addPosition(position);
      position=WidgetPosition();
      position.setRefScreenDiagonal(7.0);
      position.setPortraitX(tabletPortraitMeterGridX[0]);
      position.setPortraitY(tabletPortraitMeterGridY);
      position.setPortraitZ(0);
      position.setLandscapeX(tabletLandscapeMeterGridX[0]);
      position.setLandscapeY(tabletLandscapeMeterGridY);
      position.setLandscapeZ(0);
      config.addPosition(position);
      config.setInactiveColor(GraphicColor(255,255,255,100));
    } else {
      config.setInactiveColor(GraphicColor(255,255,255,255));
    }
    if (deviceName=="Watch") {
      position=WidgetPosition();
      position.setRefScreenDiagonal(0.0);
      position.setPortraitX(50.0);
      position.setPortraitY(50.0);
      position.setPortraitZ(0);
      position.setLandscapeX(0.0);
      position.setLandscapeY(0.0);
      position.setLandscapeZ(0);
      config.addPosition(position);
      config.setParameter("iconFilename","navigationWatchBackground");
      config.setParameter("directionIconFilename","navigationWatchDirection");
      config.setParameter("arrowIconFilename","navigationWatchArrow");
      config.setParameter("blindIconFilename","navigationWatchBlind");
      config.setParameter("statusIconFilename","navigationWatchStatus");
      config.setParameter("touchModeIconFilename","touchMode");
      config.setParameter("orientationLabelRadius","90.0");
      config.setParameter("targetRadius","91.5");
      config.setParameter("textColumnCount","2");
      config.setParameter("textRowFirstOffsetY","74");
      config.setParameter("textRowSecondOffsetY","59");
      config.setParameter("textRowThirdOffsetY","38");
      config.setParameter("textRowFourthOffsetY","28");
      config.setParameter("clockOffsetY","10");
      config.setParameter("statusTextRadius","43.0");
      config.setParameter("statusTextWidthLimit","30.0");
      config.setParameter("statusTextAngleOffset","3.25");
      config.setParameter("clockRadius","36.0");
      config.setParameter("touchModeRadius","34.5");
      config.setParameter("batteryIconRadius","92.0");
      config.setParameter("batteryIconFullFilename","batteryDotFull");
      config.setParameter("batteryIconEmptyFilename","batteryDotEmpty");
      config.setParameter("batteryIconChargingFilename","batteryDotCharging");
      config.setParameter("separatorIconFilename","navigationWatchSeparator");
    } else {
      position=WidgetPosition();
      position.setRefScreenDiagonal(0.0);
      position.setPortraitX(0.0);
      position.setPortraitY(0.0);
      position.setPortraitZ(0);
      position.setLandscapeX(14.5);
      position.setLandscapeY(82.0);
      position.setLandscapeZ(0);
      config.addPosition(position);
      config.setParameter("iconFilename","navigationBackground");
      config.setParameter("directionIconFilename","navigationDirection");
      config.setParameter("separatorIconFilename","navigationSeparator");
      config.setParameter("orientationLabelRadius","86.5");
      config.setParameter("targetRadius","86.0");
      config.setParameter("textColumnCount","1");
      config.setParameter("textRowFirstOffsetY","71");
      config.setParameter("textRowSecondOffsetY","57");
      config.setParameter("textRowThirdOffsetY","37");
      config.setParameter("textRowFourthOffsetY","25");
    }
    config.setActiveColor(GraphicColor(255,255,255,255));
    config.setBusyColor(GraphicColor(255,0,255,255));
    config.setParameter("textColumnOffsetX","5");
    config.setParameter("targetIconFilename","navigationTarget");
    config.setParameter("navigationPointIconFilename","navigationNavigationPoint");
    config.setParameter("updateInterval","1000000");
    config.setParameter("turnDistanceValueOffsetY","25");
    config.setParameter("directionChangeDuration","500000");
    config.setParameter("turnLineWidth","15.0");
    config.setParameter("turnLineArrowOverhang","7.5");
    config.setParameter("turnLineArrowHeight","13.0");
    config.setParameter("turnLineStartHeight","20.0");
    config.setParameter("turnLineMiddleHeight","8.5");
    config.setParameter("turnLineStartX","50.0");
    config.setParameter("turnLineStartY","37.0");
    config.setParameter("TurnColor/red","0");
    config.setParameter("TurnColor/green","0");
    config.setParameter("TurnColor/blue","0");
    config.setParameter("TurnColor/alpha","255");
    config.setParameter("turnWatchAlpha","96");
    config.setParameter("minTouchDetectionRadius","60.0");
    config.setParameter("circularButtonAngle","20.0");
    addWidgetToPage(config);
    if (deviceName=="Default") {
      config.setPageName("Finger Menu");
      config.clearPositions();
      position=WidgetPosition();
      position.setRefScreenDiagonal(4.0);
      position.setPortraitX(22.0);
      position.setPortraitY(89.0);
      position.setPortraitZ(0);
      position.setLandscapeX(10.0);
      position.setLandscapeY(78.0);
      position.setLandscapeZ(0);
      config.addPosition(position);
      addWidgetToPage(config);
    }
    // ---------------------------------------------------------
    if (deviceName=="Default") {
      config=WidgetConfig();
      config.setPageName("Finger Menu");
      config.setName("Menu Right");
      config.setType(WidgetTypeButton);
      position=WidgetPosition();
      position.setRefScreenDiagonal(4.0);
      position.setPortraitX(93.0);
      position.setPortraitY(5.0);
      position.setPortraitZ(1);
      position.setLandscapeX(95.0);
      position.setLandscapeY(5.0);
      position.setLandscapeZ(1);
      config.addPosition(position);
      config.setActiveColor(GraphicColor(255,255,255,100));
      config.setInactiveColor(GraphicColor(255,255,255,100));
      config.setParameter("iconFilename","menu");
      config.setParameter("command","toggleFingerMenu()");
      config.setParameter("longPressCommand","showMenu()");
      config.setParameter("repeat","0");
      addWidgetToPage(config);
      config.setName("Menu Left");
      config.clearPositions();
      position.setPortraitX(7.0);
      position.setLandscapeX(5.0);
      config.addPosition(position);
      addWidgetToPage(config);
    }
    // ---------------------------------------------------------
    if (deviceName=="Default") {
      config=WidgetConfig();
      config.setPageName("Finger Menu");
      config.setName("Trash Path");
      config.setType(WidgetTypeButton);
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setInactiveColor(GraphicColor(255,255,255,100));
      config.setParameter("iconFilename","delete");
      config.setParameter("command","askForRouteRemovalKind()");
      config.setParameter("repeat","0");
      addWidgetToPage(config);
      config=WidgetConfig();
      config.setPageName("Finger Menu");
      config.setName("Trash Address Point");
      config.setType(WidgetTypeButton);
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setInactiveColor(GraphicColor(255,255,255,100));
      config.setSafetyColor(GraphicColor(255,63,63,255));
      config.setParameter("iconFilename","delete");
      config.setParameter("command","trashAddressPoint()");
      config.setParameter("repeat","0");
      config.setParameter("safetyStepDuration","3000000");
      addWidgetToPage(config);
      config=WidgetConfig();
      config.setPageName("Finger Menu");
      config.setName("Target At Address Point");
      config.setType(WidgetTypeButton);
      config.setActiveColor(GraphicColor(255,255,255,255));
      config.setInactiveColor(GraphicColor(255,255,255,100));
      config.setParameter("iconFilename","setTargetAtAddressPoint");
      config.setParameter("command","setTargetAtAddressPoint()");
      config.setParameter("repeat","0");
      addWidgetToPage(config);
    }
  }
  if (enableFingerMenu) {
    if (c->getStringValue("Graphic/Widget/Device[@name='" + deviceName + "']","selectedPage",__FILE__, __LINE__)!="Finger Menu")
      c->setStringValue("Graphic/Widget/Device[@name='" + deviceName + "']","selectedPage","Finger Menu",__FILE__, __LINE__);
  } else {
    if (c->getStringValue("Graphic/Widget/Device[@name='" + deviceName + "']","selectedPage",__FILE__, __LINE__)=="Finger Menu")
      c->setStringValue("Graphic/Widget/Device[@name='" + deviceName + "']","selectedPage","Default",__FILE__, __LINE__);
  }

  // Create the widgets from the config
  pageNames=c->getAttributeValues("Graphic/Widget/Device[@name='" + deviceName + "']/Page","name",__FILE__, __LINE__);
  pageNames.push_back("Empty");
  std::list<std::string>::iterator i;
  std::vector<WidgetPrimitive*> fingerMenuCircleEntries;
  fingerMenuCircleEntries.resize(7);
  std::vector<WidgetPrimitive*> fingerMenuPathRowEntries;
  fingerMenuPathRowEntries.resize(5);
  std::vector<WidgetPrimitive*> fingerMenuAddressPointRowEntries;
  fingerMenuAddressPointRowEntries.resize(2);
  for(i=pageNames.begin();i!=pageNames.end();i++) {
    //DEBUG("found a widget page with name %s",(*i).c_str());

    // Create the page
    WidgetPage *page=new WidgetPage(this,*i);
    if (!page) {
      FATAL("can not create widget page object",NULL);
      core->getThread()->unlockMutex(accessMutex);
      return;
    }
    WidgetPagePair pair=WidgetPagePair(*i,page);
    pageMap.insert(pair);
    bool isFingerMenuPage=false;
    if (page->getName()=="Finger Menu") 
      isFingerMenuPage=true;
    if (enableFingerMenu) {
      if (!isFingerMenuPage)
        continue;
    } else {
      if (isFingerMenuPage)
        continue;
    }

    // Go through all widgets of this page
    std::string path="Graphic/Widget/Device[@name='" + deviceName + "']/Page[@name='" + *i + "']/Primitive";
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
      WidgetCursorInfo *cursorInfo;
      WidgetAddressPoint *addressPoint;
      WidgetEBike *eBike;
      WidgetForumslader *forumslader;
      if (widgetType=="button") {
        WidgetContainer *container;
        if (isFingerMenuPage)
          container=fingerMenu;
        else
          container=page;
        button=new WidgetButton(container);
        primitive=button;
      }
      if (widgetType=="checkbox") {
        WidgetContainer *container;
        if (isFingerMenuPage)
          container=fingerMenu;
        else
          container=page;
        checkbox=new WidgetCheckbox(container);
        primitive=checkbox;
      }
      if (widgetType=="meter") {
        meter=new WidgetMeter(page);
        primitive=meter;
      }
      if (widgetType=="scale") {
        scale=new WidgetScale(page);
        primitive=scale;
      }
      if (widgetType=="status") {
        status=new WidgetStatus(page);
        primitive=status;
      }
      if (widgetType=="navigation") {
        navigation=new WidgetNavigation(page);
        primitive=navigation;
      }
      if (widgetType=="pathInfo") {
        pathInfo=new WidgetPathInfo(page);
        primitive=pathInfo;
      }
      if (widgetType=="cursorInfo") {
        if (isFingerMenuPage)
          cursorInfo=new WidgetCursorInfo(fingerMenu);
        else
          cursorInfo=new WidgetCursorInfo(page);
        primitive=cursorInfo;
      }
      if (widgetType=="addressPoint") {
        addressPoint=new WidgetAddressPoint(page);
        primitive=addressPoint;
      }
      if (widgetType=="eBike") {
        eBike=new WidgetEBike(page);
        primitive=eBike;
      }
      if (widgetType=="forumslader") {
        forumslader=new WidgetForumslader(page);
        primitive=forumslader;
      }

      // Set type-independent properties
      std::list<std::string> name;
      name.push_back(*j);
      primitive->setName(name);
      primitive->setActiveColor(c->getGraphicColorValue(widgetPath + "/ActiveColor",__FILE__, __LINE__));
      primitive->setInactiveColor(c->getGraphicColorValue(widgetPath + "/InactiveColor",__FILE__, __LINE__));
      primitive->setColor(primitive->getInactiveColor());

      // Load the image of the widget
      if ((widgetType=="button")||(widgetType=="meter")||(widgetType=="scale")||(widgetType=="status")||(widgetType=="navigation")||(widgetType=="pathInfo")||(widgetType=="addressPoint")||(widgetType=="eBike")||(widgetType=="forumslader")) {
        primitive->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"iconFilename",__FILE__, __LINE__));
      }
      if (widgetType=="checkbox") {
        primitive->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"checkedIconFilename",__FILE__, __LINE__));
        checkbox->setCheckedTexture(primitive->getTexture());
        primitive->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"uncheckedIconFilename",__FILE__, __LINE__));
        checkbox->setUncheckedTexture(primitive->getTexture());
        checkbox->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval",__FILE__, __LINE__));
      }
      if (widgetType=="navigation") {
        navigation->getDirectionIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"directionIconFilename",__FILE__, __LINE__));
        navigation->getDirectionIcon()->setX(-navigation->getDirectionIcon()->getIconWidth()/2);
        navigation->getDirectionIcon()->setY(-navigation->getDirectionIcon()->getIconHeight()/2);
        navigation->getTargetIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"targetIconFilename",__FILE__, __LINE__));
        navigation->getTargetIcon()->setX(-navigation->getTargetIcon()->getIconWidth()/2);
        navigation->getTargetIcon()->setY(-navigation->getTargetIcon()->getIconHeight()/2);
        navigation->getNavigationPointIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"navigationPointIconFilename",__FILE__, __LINE__));        
        navigation->getNavigationPointIcon()->setX(-navigation->getNavigationPointIcon()->getIconWidth()/2);
        navigation->getNavigationPointIcon()->setY(-navigation->getNavigationPointIcon()->getIconHeight()/2-navigation->getNavigationPointIcon()->getIconHeight()/4);
        navigation->getSeparatorIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"separatorIconFilename",__FILE__, __LINE__));
        navigation->getSeparatorIcon()->setX(0);
        navigation->getSeparatorIcon()->setY(0);
        if (deviceName=="Watch") {
          navigation->setMinTouchDetectionRadius(c->getDoubleValue(widgetPath,"minTouchDetectionRadius",__FILE__, __LINE__)*navigation->getIconHeight()/2/100.0);
          navigation->setCircularButtonAngle(c->getDoubleValue(widgetPath,"circularButtonAngle",__FILE__, __LINE__));
          navigation->getArrowIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"arrowIconFilename",__FILE__, __LINE__));
          navigation->getArrowIcon()->setX(-navigation->getDirectionIcon()->getIconWidth()/2);
          navigation->getArrowIcon()->setY(-navigation->getDirectionIcon()->getIconHeight()/2);
          navigation->getBlindIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"blindIconFilename",__FILE__, __LINE__));
          navigation->getBlindIcon()->setX(-navigation->getBlindIcon()->getIconWidth()/2);
          navigation->getBlindIcon()->setY(-navigation->getBlindIcon()->getIconHeight()/2);
          navigation->getStatusIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"statusIconFilename",__FILE__, __LINE__));
          navigation->getStatusIcon()->setX(-navigation->getStatusIcon()->getIconWidth()/2);
          navigation->getStatusIcon()->setY(-navigation->getStatusIcon()->getIconHeight()/2);
          navigation->getTouchModeIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"touchModeIconFilename",__FILE__, __LINE__));
          navigation->getTouchModeIcon()->setX(-navigation->getTouchModeIcon()->getIconWidth()/2);
          navigation->getTouchModeIcon()->setY(-navigation->getTouchModeIcon()->getIconHeight()/2);
          navigation->getBatteryIconCharging()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"batteryIconChargingFilename",__FILE__, __LINE__));
          navigation->getBatteryIconFull()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"batteryIconFullFilename",__FILE__, __LINE__));
          navigation->getBatteryIconEmpty()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"batteryIconEmptyFilename",__FILE__, __LINE__));
        }
      }
      if (widgetType=="pathInfo") {
        pathInfo->getLocationIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"locationIconFilename",__FILE__, __LINE__));
        pathInfo->getNavigationPointIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"navigationPointIconFilename",__FILE__, __LINE__));        
      }

      // Set type-dependent properties
      if (widgetType=="button") {
        button->setCommand(c->getStringValue(widgetPath,"command",__FILE__, __LINE__));
        std::string longPressCommand=c->getStringValue(widgetPath,"longPressCommand",__FILE__, __LINE__);
        if (longPressCommand!="unknown")
          button->setLongPressCommand(longPressCommand);
        button->setRepeat(c->getIntValue(widgetPath,"repeat",__FILE__, __LINE__));
        if (c->pathExists(widgetPath + "/SafetyColor", __FILE__, __LINE__)) {
          button->setSafetyStep(true);
          button->setSafetyStepColor(c->getGraphicColorValue(widgetPath + "/SafetyColor", __FILE__, __LINE__));
          button->setSafetyStepDuration(c->getIntValue(widgetPath,"safetyStepDuration",__FILE__,__LINE__));
        }
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
          core->getThread()->unlockMutex(accessMutex);
          return;
        }
        meter->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval",__FILE__, __LINE__));
        meter->setLabelY(c->getDoubleValue(widgetPath,"labelY",__FILE__, __LINE__)*meter->getIconHeight()/100.0);
        meter->setValueY(c->getDoubleValue(widgetPath,"valueY",__FILE__, __LINE__)*meter->getIconHeight()/100.0);
        meter->setUnitY(c->getDoubleValue(widgetPath,"unitY",__FILE__, __LINE__)*meter->getIconHeight()/100.0);
      }
      if (widgetType=="scale") {
        scale->setBackgroundWidth(c->getDoubleValue(widgetPath,"width",__FILE__, __LINE__)*scale->getIconWidth()/100.0);
        scale->setBackgroundHeight(c->getDoubleValue(widgetPath,"height",__FILE__, __LINE__)*scale->getIconHeight()/100.0);
        scale->setBackgroundAlpha(c->getIntValue(widgetPath,"alpha",__FILE__, __LINE__));
        scale->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval",__FILE__, __LINE__));
        scale->setTopLabelOffsetY(c->getDoubleValue(widgetPath,"topLabelOffsetY",__FILE__, __LINE__)*scale->getIconHeight()/100.0);
        scale->setBottomLabelOffsetY(c->getDoubleValue(widgetPath,"bottomLabelOffsetY",__FILE__, __LINE__)*scale->getIconHeight()/100.0);        
        scale->getAltitudeIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"altitudeIconFilename",__FILE__, __LINE__));
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
        navigation->setTextColumnCount(c->getIntValue(widgetPath,"textColumnCount",__FILE__, __LINE__));
        navigation->setTextRowFirstOffsetY(c->getDoubleValue(widgetPath,"textRowFirstOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTextRowSecondOffsetY(c->getDoubleValue(widgetPath,"textRowSecondOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTextRowThirdOffsetY(c->getDoubleValue(widgetPath,"textRowThirdOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTextRowFourthOffsetY(c->getDoubleValue(widgetPath,"textRowFourthOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setClockOffsetY(c->getDoubleValue(widgetPath,"clockOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTextColumnOffsetX(c->getDoubleValue(widgetPath,"textColumnOffsetX",__FILE__, __LINE__)*navigation->getIconWidth()/100.0);
        navigation->setTurnDistanceValueOffsetY(c->getDoubleValue(widgetPath,"turnDistanceValueOffsetY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setDirectionChangeDuration(c->getDoubleValue(widgetPath,"directionChangeDuration",__FILE__, __LINE__));
        navigation->setTargetRadius(c->getDoubleValue(widgetPath,"targetRadius",__FILE__, __LINE__)*navigation->getIconHeight()/200.0);
        navigation->setOrientationLabelRadius(c->getDoubleValue(widgetPath,"orientationLabelRadius",__FILE__, __LINE__)*navigation->getIconHeight()/200.0);
        navigation->setBatteryIconRadius(c->getDoubleValue(widgetPath,"batteryIconRadius",__FILE__, __LINE__)*navigation->getIconHeight()/200.0);
        navigation->setTurnLineWidth(c->getDoubleValue(widgetPath,"turnLineWidth",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineArrowOverhang(c->getDoubleValue(widgetPath,"turnLineArrowOverhang",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineArrowHeight(c->getDoubleValue(widgetPath,"turnLineArrowHeight",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineStartHeight(c->getDoubleValue(widgetPath,"turnLineStartHeight",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineMiddleHeight(c->getDoubleValue(widgetPath,"turnLineMiddleHeight",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineStartX(c->getDoubleValue(widgetPath,"turnLineStartX",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        navigation->setTurnLineStartY(c->getDoubleValue(widgetPath,"turnLineStartY",__FILE__, __LINE__)*navigation->getIconHeight()/100.0);
        GraphicColor turnColor=c->getGraphicColorValue(widgetPath+"/TurnColor",__FILE__, __LINE__);
        navigation->setTurnColor(turnColor);
        if (device->getIsWatch()) {
          turnColor.setAlpha(c->getIntValue(widgetPath,"turnWatchAlpha",__FILE__,__LINE__));
          navigation->setTurnColor(turnColor);
          navigation->setBusyColor(c->getGraphicColorValue(widgetPath + "/BusyColor",__FILE__, __LINE__));
          navigation->setStatusTextWidthLimit(c->getDoubleValue(widgetPath,"statusTextWidthLimit",__FILE__, __LINE__)*navigation->getIconWidth()/100.0);
          navigation->setStatusTextRadius(c->getDoubleValue(widgetPath,"statusTextRadius",__FILE__, __LINE__)*navigation->getIconWidth()/100.0);
          navigation->setStatusTextAngleOffset(c->getDoubleValue(widgetPath,"statusTextAngleOffset",__FILE__, __LINE__));
          navigation->setClockRadius(c->getDoubleValue(widgetPath,"clockRadius",__FILE__, __LINE__)*navigation->getIconWidth()/100.0);
          navigation->setTouchModeRadius(c->getDoubleValue(widgetPath,"touchModeRadius",__FILE__, __LINE__)*navigation->getIconWidth()/100.0);
        }
      }
      if (widgetType=="pathInfo") {
        pathInfo->setPathNameOffsetX(c->getDoubleValue(widgetPath,"pathNameOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathNameOffsetY(c->getDoubleValue(widgetPath,"pathNameOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathNameWidth(c->getDoubleValue(widgetPath,"pathNameWidth",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathValuesWidth(c->getDoubleValue(widgetPath,"pathValuesWidth",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathNameOffsetX(c->getDoubleValue(widgetPath,"pathNameOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathNameOffsetY(c->getDoubleValue(widgetPath,"pathNameOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathLengthOffsetX(c->getDoubleValue(widgetPath,"pathLengthOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathLengthOffsetY(c->getDoubleValue(widgetPath,"pathLengthOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathAltitudeUpOffsetX(c->getDoubleValue(widgetPath,"pathAltitudeUpOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathAltitudeUpOffsetY(c->getDoubleValue(widgetPath,"pathAltitudeUpOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathAltitudeDownOffsetX(c->getDoubleValue(widgetPath,"pathAltitudeDownOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathAltitudeDownOffsetY(c->getDoubleValue(widgetPath,"pathAltitudeDownOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setPathDurationOffsetX(c->getDoubleValue(widgetPath,"pathDurationOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setPathDurationOffsetY(c->getDoubleValue(widgetPath,"pathDurationOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setAltitudeProfileWidth(c->getDoubleValue(widgetPath,"altitudeProfileWidth",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setAltitudeProfileHeightWithNavigationPoints(c->getDoubleValue(widgetPath,"altitudeProfileHeightWithNavigationPoints",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setAltitudeProfileHeightWithoutNavigationPoints(c->getDoubleValue(widgetPath,"altitudeProfileHeightWithoutNavigationPoints",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setAltitudeProfileOffsetX(c->getDoubleValue(widgetPath,"altitudeProfileOffsetX",__FILE__, __LINE__)*pathInfo->getIconWidth()/100.0);
        pathInfo->setAltitudeProfileOffsetY(c->getDoubleValue(widgetPath,"altitudeProfileOffsetY",__FILE__, __LINE__)*pathInfo->getIconHeight()/100.0);
        pathInfo->setAltitudeProfileLineWidth(c->getDoubleValue(widgetPath,"altitudeProfileLineWidth",__FILE__, __LINE__)*((double)device->getScreen()->getDPI())/160.0);
        pathInfo->setAltitudeProfileAxisLineWidth(c->getDoubleValue(widgetPath,"altitudeProfileAxisLineWidth",__FILE__, __LINE__)*((double)device->getScreen()->getDPI())/160.0);
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
      }
      if (widgetType=="cursorInfo") {
        cursorInfo->setWidth(c->getDoubleValue(widgetPath,"width",__FILE__, __LINE__)*device->getDPI());
        cursorInfo->setHeight(c->getDoubleValue(widgetPath,"height",__FILE__, __LINE__)*device->getDPI());
        GraphicColor c=cursorInfo->getColor();
        c.setAlpha(0);
        cursorInfo->setColor(c);
      }
      if (widgetType=="eBike") {
        eBike->setGaugeBackgroundColor(c->getGraphicColorValue(widgetPath + "/GaugeBackgroundColor",__FILE__, __LINE__));        
        eBike->setGaugeForegroundColor(c->getGraphicColorValue(widgetPath + "/GaugeForegroundColor",__FILE__, __LINE__));        
        eBike->setPowerLevelOffsetX(c->getDoubleValue(widgetPath,"powerLevelOffsetX",__FILE__, __LINE__)*eBike->getIconWidth()/100.0);
        eBike->setPowerLevelOffsetY(c->getDoubleValue(widgetPath,"powerLevelOffsetY",__FILE__, __LINE__)*eBike->getIconHeight()/100.0);
        eBike->setEngineTemperatureOffsetX(c->getDoubleValue(widgetPath,"engineTemperatureOffsetX",__FILE__, __LINE__)*eBike->getIconWidth()/100.0);
        eBike->setEngineTemperatureOffsetY(c->getDoubleValue(widgetPath,"engineTemperatureOffsetY",__FILE__, __LINE__)*eBike->getIconHeight()/100.0);
        eBike->setEngineTemperatureBackgroundRadius(c->getDoubleValue(widgetPath,"engineTemperatureBackgroundRadius",__FILE__, __LINE__)*eBike->getIconHeight()/100.0);
        eBike->setEngineTemperatureForegroundRadius(c->getDoubleValue(widgetPath,"engineTemperatureForegroundRadius",__FILE__, __LINE__)*eBike->getEngineTemperatureBackgroundRadius()/100.0);
        eBike->setEngineTemperatureMaxHeight(c->getDoubleValue(widgetPath,"engineTemperatureMaxHeight",__FILE__, __LINE__)*eBike->getIconHeight()/100.0);
        eBike->setEngineTemperatureBackgroundWidth(c->getDoubleValue(widgetPath,"engineTemperatureBackgroundWidth",__FILE__, __LINE__)*eBike->getIconHeight()/100.0);
        eBike->setEngineTemperatureForegroundWidth(c->getDoubleValue(widgetPath,"engineTemperatureForegroundWidth",__FILE__, __LINE__)*eBike->getEngineTemperatureBackgroundWidth()/100.0);
        eBike->setDistanceElectricOffsetX(c->getDoubleValue(widgetPath,"distanceElectricOffsetX",__FILE__, __LINE__)*eBike->getIconWidth()/100.0);
        eBike->setDistanceElectricOffsetY(c->getDoubleValue(widgetPath,"distanceElectricOffsetY",__FILE__, __LINE__)*eBike->getIconHeight()/100.0);
        eBike->setBatteryLevelRadius(c->getDoubleValue(widgetPath,"batteryLevelRadius",__FILE__, __LINE__)*eBike->getIconHeight()/100.0);
        eBike->setBatteryLevelBackgroundWidth(c->getDoubleValue(widgetPath,"batteryLevelBackgroundWidth",__FILE__, __LINE__)*eBike->getIconHeight()/100.0);
        eBike->setBatteryLevelForegroundWidth(c->getDoubleValue(widgetPath,"batteryLevelForegroundWidth",__FILE__, __LINE__)*eBike->getBatteryLevelBackgroundWidth()/100.0);        
      }
      if (widgetType=="forumslader") {
        forumslader->setGaugeBackgroundColor(c->getGraphicColorValue(widgetPath + "/GaugeBackgroundColor",__FILE__, __LINE__));        
        forumslader->setGaugeFillgroundColor(c->getGraphicColorValue(widgetPath + "/GaugeFillgroundColor",__FILE__, __LINE__));        
        forumslader->setGaugeForegroundColor(c->getGraphicColorValue(widgetPath + "/GaugeForegroundColor",__FILE__, __LINE__));        
        forumslader->setPowerDrawLevelOffsetY(c->getDoubleValue(widgetPath,"powerDrawLevelOffsetY",__FILE__,__LINE__)*forumslader->getIconHeight()/100.0);
        forumslader->setPowerDrawGaugeOffsetY(c->getDoubleValue(widgetPath,"powerDrawGaugeOffsetY",__FILE__,__LINE__)*forumslader->getIconHeight()/100.0);
        forumslader->setPowerDrawGaugeRadius(c->getDoubleValue(widgetPath,"powerDrawGaugeRadius",__FILE__,__LINE__)*getScreen()->getDPI());
        forumslader->setPowerDrawGaugeBackgroundWidth(c->getDoubleValue(widgetPath,"powerDrawGaugeBackgroundWidth",__FILE__,__LINE__)*getScreen()->getDPI());
        forumslader->setPowerDrawGaugeForegroundWidth(c->getDoubleValue(widgetPath,"powerDrawGaugeForegroundWidth",__FILE__,__LINE__)*getScreen()->getDPI());
        forumslader->setBatteryLevelOffsetY(c->getDoubleValue(widgetPath,"batteryLevelOffsetY",__FILE__,__LINE__)*forumslader->getIconHeight()/100.0);
        forumslader->setBatteryGaugeOffsetX(c->getDoubleValue(widgetPath,"batteryGaugeOffsetX",__FILE__,__LINE__)*forumslader->getIconWidth()/100.0);
        forumslader->setBatteryGaugeOffsetY(c->getDoubleValue(widgetPath,"batteryGaugeOffsetY",__FILE__,__LINE__)*forumslader->getIconHeight()/100.0);
        forumslader->setBatteryGaugeMaxHeight(c->getDoubleValue(widgetPath,"batteryGaugeMaxHeight",__FILE__,__LINE__)*forumslader->getIconHeight()/100.0);
        forumslader->setBatteryGaugeBackgroundWidth(c->getDoubleValue(widgetPath,"batteryGaugeBackgroundWidth",__FILE__,__LINE__)*getScreen()->getDPI());
        forumslader->setBatteryGaugeForegroundWidth(c->getDoubleValue(widgetPath,"batteryGaugeForegroundWidth",__FILE__,__LINE__)*getScreen()->getDPI());
        forumslader->setBatteryGaugeTipWidth(c->getDoubleValue(widgetPath,"batteryGaugeTipWidth",__FILE__,__LINE__)*getScreen()->getDPI());
        forumslader->setBatteryGaugeTipHeight(c->getDoubleValue(widgetPath,"batteryGaugeTipHeight",__FILE__,__LINE__)*getScreen()->getDPI());
        forumslader->getBatteryChargingIcon()->setTextureFromIcon(device->getScreen(),c->getStringValue(widgetPath,"batteryIconChargingFilename",__FILE__, __LINE__));
      }

      // Add the widget to the page
      bool skipPageAdd=false;
      if (isFingerMenuPage) {
        if (*j=="Track Recording")          { fingerMenuCircleEntries[2]=primitive; skipPageAdd=true; }
        if (*j=="Change Map Layer")         { fingerMenuCircleEntries[1]=primitive; skipPageAdd=true;  }
        if (*j=="Return To Location")       { fingerMenuCircleEntries[3]=primitive; skipPageAdd=true;  }
        if (*j=="Zoom Level Lock")          { fingerMenuCircleEntries[0]=primitive; skipPageAdd=true;  }
        if (*j=="Target Visibility")        { fingerMenuCircleEntries[4]=primitive; skipPageAdd=true;  }
        if (*j=="Target At Address")        { fingerMenuCircleEntries[5]=primitive; skipPageAdd=true;  }
        if (*j=="Target At Center")         { fingerMenuCircleEntries[6]=primitive; skipPageAdd=true;  }
        if (*j=="Set Path End Flag")        { fingerMenuPathRowEntries[0]=primitive; skipPageAdd=true;  }
        if (*j=="Set Path Start Flag")      { fingerMenuPathRowEntries[1]=primitive; skipPageAdd=true;  }
        if (*j=="Set Active Route")         { fingerMenuPathRowEntries[2]=primitive; skipPageAdd=true;  }
        if (*j=="Path Info Lock")           { fingerMenuPathRowEntries[3]=primitive; skipPageAdd=true;  }
        if (*j=="Trash Path")               { fingerMenuPathRowEntries[4]=primitive; skipPageAdd=true;  }
        if (*j=="Target At Address Point")  { fingerMenuAddressPointRowEntries[0]=primitive; skipPageAdd=true;  }
        if (*j=="Trash Address Point")      { fingerMenuAddressPointRowEntries[1]=primitive; skipPageAdd=true;  }
        if (*j=="Cursor Info")              { fingerMenu->setCursorInfoWidget((WidgetCursorInfo*)primitive); skipPageAdd=true; }
        if (*j=="Menu Right")               { fingerMenu->addWidget((WidgetCursorInfo*)primitive); skipPageAdd=true; }
        if (*j=="Menu Left")                { fingerMenu->addWidget((WidgetCursorInfo*)primitive); skipPageAdd=true; }
      }
      if (!skipPageAdd) {
        //DEBUG("pageName=%s widgetName=%s",page->getName().c_str(),primitive->getName().front().c_str());
        page->addWidget(primitive);
      }
      /*if ((((widgetType=="checkbox")||(widgetType=="button")))&&(primitive->getName().front().find("Page")==std::string::npos)&&(device==core->getDefaultDevice())) {
        fingerMenu->addWidget(primitive);
      } else {
        page->addWidget(primitive);
      }*/
    }
  }

  // Add all the finger menu entries
  if (enableFingerMenu) {
    for (int i=fingerMenuCircleEntries.size()-1;i>=0;i--) {
      fingerMenu->addWidgetToCircle(fingerMenuCircleEntries[i]);
    }
    for (int i=fingerMenuPathRowEntries.size()-1;i>=0;i--) {
      fingerMenu->addWidgetToPathRow(fingerMenuPathRowEntries[i]);
    }
    for (int i=fingerMenuAddressPointRowEntries.size()-1;i>=0;i--) {
      fingerMenu->addWidgetToAddressPointRow(fingerMenuAddressPointRowEntries[i]);
    }
  }

  // Set the default page on the graphic engine
  WidgetPageMap::iterator j;
  j=pageMap.find(c->getStringValue("Graphic/Widget/Device[@name='" + deviceName + "']","selectedPage",__FILE__, __LINE__));
  if (j==pageMap.end()) {
    FATAL("default page does not exist",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return;
  } else {
    currentPage=j->second;
    visiblePages.addPrimitive(currentPage->getGraphicObject());
    getGraphicEngine()->lockDrawing(__FILE__,__LINE__);
    device->setVisibleWidgetPages(&visiblePages);
    getGraphicEngine()->unlockDrawing();
  }

  // Set the finger menu on the graphic engine
  if ((enableFingerMenu)&&(device==core->getDefaultDevice())) {
    getGraphicEngine()->lockDrawing(__FILE__,__LINE__);
    device->setFingerMenu(fingerMenu->getGraphicObject());
    getGraphicEngine()->unlockDrawing();
  }

  // Allow access by the next thread
  core->getThread()->unlockMutex(accessMutex);

  // Inform the widgets about the current app status
  std::list<NavigationPath*> *routes=core->getNavigationEngine()->lockRoutes(__FILE__,__LINE__);
  for (std::list<NavigationPath*>::iterator i=routes->begin();i!=routes->end();i++) {
    onPathChange(*i,NavigationPathChangeTypeWidgetEngineInit);
  }
  core->getNavigationEngine()->unlockRoutes();
  MapPosition pos = *core->getNavigationEngine()->lockLocationPos(__FILE__,__LINE__);
  core->getNavigationEngine()->unlockLocationPos();
  onLocationChange(pos);
  pos = *core->getMapEngine()->lockMapPos(__FILE__,__LINE__);
  core->getMapEngine()->unlockMapPos();
  std::list<MapTile*> *centerMapTiles = core->getMapEngine()->lockCenterMapTiles(__FILE__,__LINE__);
  onMapChange(pos,centerMapTiles);
  core->getMapEngine()->unlockCenterMapTiles();

  // Set the positions of the widgets
  updateWidgetPositions();
}

// Updates the positions of the widgets in dependence of the current device->getScreen() dimension
void WidgetEngine::updateWidgetPositions() {
  closeFingerMenu();
  Int width=device->getScreen()->getWidth();
  Int height=device->getScreen()->getHeight();
  updateWidgetPositions(0,0,width,height,false);
}

// Updates the positions of widgets that shall be positioned relative to a window
void WidgetEngine::setWindow(Int x, Int y, Int width, Int height) {
  //DEBUG("window: x=%d y=%d width=%d height=%d",x,y,width,height);
  updateWidgetPositions(x,y,width,height,true);
}

// Updates the positions of widgets with the given dimension
void WidgetEngine::updateWidgetPositions(Int x, Int y, Int width, Int height, bool onlyWindowWidgets) {

  // Only one thread please
  lockAccessAfterReadComplete(__FILE__,__LINE__);

  // Find out the orientation for which we need to update the positioning
  std::string orientation="Unknown";
  switch(device->getScreen()->getOrientation()) {
    case GraphicScreenOrientationLandscape: orientation="Landscape"; break;
    case GraphicScreenOrientationProtrait: orientation="Portrait"; break;
  }
  //DEBUG("orientation=%s",orientation.c_str());
  double diagonal=device->getScreen()->getDiagonal();

  // Go through all pages and the cursor info  
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  ConfigStore *c=core->getConfigStore();
  std::list<WidgetContainer*> containers;
  if (fingerMenu) containers.push_back(fingerMenu);
  WidgetPageMap::iterator j;
  for(j=pageMap.begin();j!=pageMap.end();j++) {
    containers.push_back(j->second);
  }
  std::list<WidgetContainer*>::iterator i;
  for(i=containers.begin();i!=containers.end();i++) {
    WidgetContainer *container=*i;
    //DEBUG("container=%s",container->getName().c_str());

    // Go through all widgets
    std::list<WidgetPrimitive*> primitives;
    getGraphicEngine()->lockDrawing(__FILE__,__LINE__);
    GraphicPrimitiveMap *widgetMap=container->getGraphicObject()->getPrimitiveMap();
    GraphicPrimitiveMap::iterator j;
    for(j=widgetMap->begin();j!=widgetMap->end();j++) {
      WidgetPrimitive *primitive=(WidgetPrimitive*)j->second;

      // Check if widget needs to be processed
      boolean changePosition=true;
      std::string path="Graphic/Widget/Device[@name='" + device->getName() + "']/Page[@name='" + container->getName() + "']/Primitive[@name='" + primitive->getName().front() + "']";
      if (onlyWindowWidgets) {
        //DEBUG("path=%s",path.c_str());
        if (c->getIntValue(path,"relativeToWindow",__FILE__, __LINE__)==0)
          changePosition=false;
        //else
        //  DEBUG("name=%s",primitive->getName().front().c_str());
      }
      path+="/Position";
      if (changePosition) {

        // Find the position that is closest to the reference device->getScreen() diagonal
        std::list<std::string> refScreenDiagonals = c->getAttributeValues(path,"refScreenDiagonal",__FILE__,__LINE__);
        double nearestValue = std::numeric_limits<double>::max();
        std::string nearestString;
        double smallestDiagonal = std::numeric_limits<double>::max();
        std::string smallestString;
        for (std::list<std::string>::iterator k=refScreenDiagonals.begin();k!=refScreenDiagonals.end();k++) {
          double currentDiagonal = atof(k->c_str());
          if (diagonal>currentDiagonal) {
            double value = fabs(diagonal - currentDiagonal);
            if (value<nearestValue) {
              nearestValue=value;
              nearestString=*k;
            }
          }
          if (currentDiagonal<smallestDiagonal) {
            smallestDiagonal=currentDiagonal;
            smallestString=*k;
          }
        }
        if (nearestValue==std::numeric_limits<double>::max()) {
          nearestString=smallestString;
        }
        path=path + "[@refScreenDiagonal='" + nearestString + "']/" + orientation;
        //DEBUG("path=%s",path.c_str());
        //DEBUG("x=%f y=%f",c->getDoubleValue(path,"x",__FILE__, __LINE__),c->getDoubleValue(path,"y",__FILE__, __LINE__));

        // Update the position
        primitive->updatePosition(x+width*c->getDoubleValue(path,"x",__FILE__, __LINE__)/100.0-width/2-primitive->getIconWidth()/2,y+height*c->getDoubleValue(path,"y",__FILE__, __LINE__)/100.0-height/2-primitive->getIconHeight()/2,c->getIntValue(path,"z",__FILE__, __LINE__));
        double xHiddenPercent=c->getDoubleValue(path,"xHidden",__FILE__, __LINE__);
        double yHiddenPercent=c->getDoubleValue(path,"yHidden",__FILE__, __LINE__);
        if ((xHiddenPercent!=0.0)||(yHiddenPercent!=0.0)) {
          primitive->setXHidden(x+width*xHiddenPercent/100.0-width/2-primitive->getIconWidth()/2);
          primitive->setYHidden(y+height*yHiddenPercent/100.0-height/2-primitive->getIconHeight()/2);
        }
      }
      primitives.push_back(primitive);
    }

    // Re-add the widget to the graphic object to get them sorted
    container->deinit(false);
    for(std::list<WidgetPrimitive*>::iterator i=primitives.begin();i!=primitives.end();i++) {
      container->addWidget(*i);
    }
    getGraphicEngine()->unlockDrawing();

  }

  // Allow access by the next thread
  core->getThread()->unlockMutex(accessMutex);
}

// Clears all widget pages
void WidgetEngine::deinit() {

  // Clear the widget page
  getGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  device->setVisibleWidgetPages(NULL);
  getGraphicEngine()->unlockDrawing();

  // Only one thread
  lockAccessAfterReadComplete(__FILE__,__LINE__);

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

  // RE-create the finger menu
  if (fingerMenu) {
    delete fingerMenu;
    if (!(fingerMenu=new WidgetFingerMenu(this))) {
      FATAL("can not create finger menu object",NULL);
    }
  }

  // Allow access by the next thread
  core->getThread()->unlockMutex(accessMutex);
}

// Called when the screen is touched
bool WidgetEngine::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {

  // Indicate access to the object
  readAccessStart(__FILE__,__LINE__);

  // Do we have an active page?
  if (!currentPage) {
    readAccessStop();
    return false;
  }

  // Shall we ignore touches?
  if (t<=ignoreTouchesEnd) {
    readAccessStop();
    return false;
  }

  // First check if a widget in the finger menu was touched
  if ((fingerMenu)&&(fingerMenu->onTouchDown(t,x,y))) {
    core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
    isTouched=false;
    core->getThread()->unlockMutex(accessMutex);
    GraphicPosition *visPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
    visPos->updateLastUserModification();
    core->getDefaultGraphicEngine()->unlockPos();
    readAccessStop();
    return true;
  } else {


    // Then check if widget on page is touched
    if (((!fingerMenu)||(!fingerMenu->isOpen()))&&(currentPage->onTouchDown(t,x,y))) {
      core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
      isTouched=false;
      core->getThread()->unlockMutex(accessMutex);
      readAccessStop();
      return true;
    }
  }

  // Read is over
  readAccessStop();
  
  return false;
}

// Called when the screen is untouched
bool WidgetEngine::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {
  readAccessStart(__FILE__,__LINE__);
  if (!currentPage) {
    readAccessStop();
    return false;
  }
  if (t<=ignoreTouchesEnd) {
    readAccessStop();
    return false;
  }
  deselectPage();
  if ((!fingerMenu)||(!fingerMenu->isOpen())) currentPage->onTouchUp(t,x,y,cancel);
  if (fingerMenu) fingerMenu->onTouchUp(t,x,y,cancel);
  readAccessStop();
  return true;
}

// Called when the screen is touched
bool WidgetEngine::onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff) {

  // Indicate access to the object
  readAccessStart(__FILE__,__LINE__);

  // Do we have an active page?
  if (!currentPage) {
    readAccessStop();
    return false;
  }

  // Shall we ignore touches?
  if (t<=ignoreTouchesEnd) {
    readAccessStop();
    return false;
  }

  // First check if a widget on the page was touched
  if (currentPage->onTwoFingerGesture(t,dX,dY,angleDiff,scaleDiff)) {
    readAccessStop();
    return true;
  }


  // Read access is over
  readAccessStop();
  
  return false;
}

// Deselects the currently selected page
void WidgetEngine::deselectPage() {
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  isTouched=false;
  core->getThread()->unlockMutex(accessMutex);
}

// Sets a new page
void WidgetEngine::setPage(std::string name, Int direction) {

  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);

  // Do we have an active page?
  if (!currentPage) {
    core->getThread()->unlockMutex(accessMutex);
    return;
  }

  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  Int width = device->getScreen()->getWidth();
  Int height = device->getScreen()->getHeight();

  // Check if the requested page exists
  if (pageMap.find(name)==pageMap.end()) {
    WARNING("page <%s> does not exist",name.c_str());
    core->getThread()->unlockMutex(accessMutex);
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
  if (abs(direction)==2) {
    if (device->getScreen()->getOrientation()==GraphicScreenOrientationProtrait) {
      translateParameter.setEndX(0);
      translateParameter.setEndY(direction/2*height);
    } else {
      translateParameter.setEndX(-direction/2*width);
      translateParameter.setEndY(0);
    }
  } else {
    translateParameter.setEndX(direction*(width+changePageOvershoot));
    translateParameter.setEndY(0);
  }
  translateParameter.setDuration(changePageDurationStep1);
  translateParameter.setInfinite(false);
  translateParameter.setAnimationType(GraphicTranslateAnimationTypeAccelerated);
  translateAnimationSequence.push_back(translateParameter);
  getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  prevPageGraphicObject->setTranslateAnimationSequence(translateAnimationSequence);
  prevPageGraphicObject->setLifeEnd(t+changePageDurationStep1);
  getGraphicEngine()->unlockDrawing();

  // Let the new page move inside the window
  translateAnimationSequence.clear();
  GraphicObject *nextPageGraphicObject=nextPage->getGraphicObject();
  translateParameter.setStartTime(t);
  if (abs(direction)==2) {
    if (device->getScreen()->getOrientation()==GraphicScreenOrientationProtrait) {
      translateParameter.setStartX(0);
      translateParameter.setStartY(-direction/2*height);
      translateParameter.setEndX(0);
      translateParameter.setEndY(0);
    } else {
      translateParameter.setStartX(direction/2*width);
      translateParameter.setStartY(0);
      translateParameter.setEndX(0);
      translateParameter.setEndY(0);
    }
  } else {
    translateParameter.setStartX(-1*direction*(width));
    translateParameter.setStartY(0);
    translateParameter.setEndX(+1*direction*(changePageOvershoot));
    translateParameter.setEndY(0);
  }
  translateParameter.setDuration(changePageDurationStep1);
  translateParameter.setInfinite(false);
  translateParameter.setAnimationType(GraphicTranslateAnimationTypeAccelerated);
  translateAnimationSequence.push_back(translateParameter);
  if (abs(direction)==1) {
    translateParameter.setStartTime(t+changePageDurationStep1);
    translateParameter.setStartX(+1*direction*(changePageOvershoot));
    translateParameter.setStartY(0);
    translateParameter.setEndX(0);
    translateParameter.setEndY(0);
    translateParameter.setDuration(changePageDurationStep2);
    translateParameter.setInfinite(false);
    translateParameter.setAnimationType(GraphicTranslateAnimationTypeAccelerated);
    translateAnimationSequence.push_back(translateParameter);
  }
  nextPageGraphicObject->setTranslateAnimationSequence(translateAnimationSequence);
  nextPageGraphicObject->setLifeEnd(0);
  getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  visiblePages.addPrimitive(nextPageGraphicObject);
  getGraphicEngine()->unlockDrawing();

  // Ignore any touches during transition
  ignoreTouchesEnd=t+changePageDurationStep1+changePageDurationStep2;

  // Set the new page
  currentPage=nextPage;
  nextPage->setWidgetsActive(t,true);
  core->getConfigStore()->setStringValue("Graphic/Widget/Device[@name='" + device->getName() + "']","selectedPage",name,__FILE__, __LINE__);
  core->getThread()->unlockMutex(accessMutex);
}

// Updates the nearest path
void WidgetEngine::updateNearestPath(MapPosition mapPos, std::list<MapTile*> *centerMapTiles) {

  // Compute the overlap in meters for the given map state
  double overlapInMeters;
  if (!core->getMapEngine()->calculateMaxDistanceInMeters(maxPathDistance,overlapInMeters)) {
    overlapInMeters=0;
  }
  //DEBUG("mapPos=(%f,%f) centerMapTiles.size()=%d",mapPos.getLat(),mapPos.getLng(),centerMapTiles->size());

  // Find the nearest path in the currently visible map tile
  NavigationPath *nearestPath=NULL;
  Int nearestPathIndex=0;
  MapPosition nearestPathMapPos;
  double minDistance=std::numeric_limits<double>::max();
  for(std::list<MapTile*>::iterator j=centerMapTiles->begin();j!=centerMapTiles->end();j++) {
    std::list<NavigationPathSegment*> nearbyPathSegments;
    nearbyPathSegments=(*j)->getCrossingNavigationPathSegments();
    for(std::list<NavigationPathSegment*>::iterator i=nearbyPathSegments.begin();i!=nearbyPathSegments.end();i++) {
      NavigationPathSegment *s=*i;
      core->getMapSource()->lockAccess(__FILE__, __LINE__);
      MapPosition prevVisPos=NavigationPath::getPathInterruptedPos();
      Int startIndex=s->getStartIndex();
      if (startIndex>0) startIndex--;
      Int endIndex=s->getEndIndex();
      if (endIndex<s->getVisualization()->getPointsSize()-1) endIndex++;
      //DEBUG("startIndex=%d endIndex=%d pointsSize=%d",startIndex,endIndex,s->getVisualization()->getPointsSize());
      for(Int j=startIndex;j<=endIndex;j++) {
        MapPosition visPos=s->getVisualization()->getPoint(j);
        if ((visPos!=NavigationPath::getPathInterruptedPos())&&(prevVisPos!=NavigationPath::getPathInterruptedPos())) {
          //DEBUG("j=%d",j);
          MapPosition normalPos;
          double d=visPos.computeNormalDistance(prevVisPos,mapPos,overlapInMeters,true,false,&normalPos);   
          //DEBUG("d=%f minDistance=%f",d,minDistance);         
          if ((d!=std::numeric_limits<double>::max())&&(d<minDistance)) {
            /*d=visPos.computeNormalDistance(prevVisPos,mapPos,overlapInMeters,true,true,&normalPos);           
            DEBUG("d=%f",d); */
            minDistance=d;
            nearestPath=s->getPath();
            nearestPathMapPos=normalPos;
            nearestPathIndex=visPos.getIndex();
          }
        }
        prevVisPos=visPos;
      }
      core->getMapSource()->unlockAccess();
    }
  }

  // Store the result
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  //DEBUG("nearestPath=%08x nearestPathIndex=%d",nearestPath,nearestPathIndex);
  this->nearestPath=nearestPath;
  this->nearestPathIndex=nearestPathIndex;
  this->nearestPathMapPos=nearestPathMapPos;
  core->getThread()->unlockMutex(accessMutex);
}

// Informs the engine that the map has changed
void WidgetEngine::onMapChange(MapPosition mapPos, std::list<MapTile*> *centerMapTiles) {

  // Update the nearest path
  updateNearestPath(mapPos,centerMapTiles);

  // Inform the widget
  readAccessStart(__FILE__,__LINE__);
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    i->second->onMapChange(currentPage==i->second ? true : false, mapPos);
  }
  if (fingerMenu) fingerMenu->onMapChange(currentPage==i->second ? true : false, mapPos);
  readAccessStop();
}

// Informs the engine that the location has changed
void WidgetEngine::onLocationChange(MapPosition mapPos) {
  readAccessStart(__FILE__,__LINE__);
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    i->second->onLocationChange(currentPage==i->second ? true : false, mapPos);
  }
  if (fingerMenu) fingerMenu->onLocationChange(currentPage==i->second ? true : false, mapPos);
  readAccessStop();
}

// Informs the engine that a path has changed
void WidgetEngine::onPathChange(NavigationPath *path, NavigationPathChangeType changeType) {

  // First inform all widgets
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  if (changeType==NavigationPathChangeTypeWillBeRemoved) {
    if (nearestPath==path)
      nearestPath=NULL;
  }
  core->getThread()->unlockMutex(accessMutex);
  readAccessStart(__FILE__,__LINE__);
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    i->second->onPathChange(currentPage==i->second ? true : false, path, changeType);
  }
  if (fingerMenu) fingerMenu->onPathChange(currentPage==i->second ? true : false, path, changeType);
  readAccessStop();

  // Then update the nearest path  
  core->getMapEngine()->setForceMapUpdate(__FILE__,__LINE__);
  /*MapPosition pos = *core->getMapEngine()->lockMapPos(__FILE__,__LINE__);
  core->getMapEngine()->unlockMapPos();
  std::list<MapTile*> *centerMapTiles = core->getMapEngine()->lockCenterMapTiles(__FILE__,__LINE__);
  onMapChange(pos,centerMapTiles);
  core->getMapEngine()->unlockCenterMapTiles();*/
}

// Informs the engine that some data has changed
void WidgetEngine::onDataChange() {
  readAccessStart(__FILE__,__LINE__);
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    i->second->onDataChange();
  }
  if (fingerMenu) fingerMenu->onDataChange();
  readAccessStop();
}

// Informs the engine about the touch mode
void WidgetEngine::setTouchMode(int mode) {
  readAccessStart(__FILE__,__LINE__);
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    i->second->setTouchMode(mode);
  }
  if (fingerMenu) fingerMenu->onDataChange();
  readAccessStop();
}

// Sets the widgets of the current page active
void WidgetEngine::setWidgetsActive(bool widgetsActive) {
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  if (currentPage) {
    currentPage->setWidgetsActive(core->getClock()->getMicrosecondsSinceStart(),widgetsActive);
  }
  core->getThread()->unlockMutex(accessMutex);
}

// Let the engine work
bool WidgetEngine::work(TimestampInMicroseconds t) {
  bool result=false;
  if (fingerMenu) 
    result=fingerMenu->work(t);
  if (currentPage) {
    return result | currentPage->work(t);
  } else {
    return result;
  }
}

// Returns the font engine
FontEngine *WidgetEngine::getFontEngine() {
  return device->getFontEngine();
}

// Returns the graphic engine
GraphicEngine *WidgetEngine::getGraphicEngine() {
  return device->getGraphicEngine();
}

// Returns the screen
Screen *WidgetEngine::getScreen() {
  return device->getScreen();
}

// Returns the device
Device *WidgetEngine::getDevice() {
  return device;
}

// Opens the finger menu
void WidgetEngine::openFingerMenu() {
  if (!enableFingerMenu)
    return;
  setWidgetsActive(false);
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  fingerMenu->open();
  core->getDefaultGraphicEngine()->unlockDrawing();
  GraphicPosition *visPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
  visPos->updateLastUserModification();
  core->getDefaultGraphicEngine()->unlockPos();
}

// Closes the finger menu
void WidgetEngine::closeFingerMenu() {
  //DEBUG("close finger menu called",NULL);
  if (!enableFingerMenu)
    return;
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__,__LINE__);
  fingerMenu->close();
  core->getDefaultGraphicEngine()->unlockDrawing();
}

// Open or close the finger menu
void WidgetEngine::toggleFingerMenu() {
  if (!enableFingerMenu)
    return;
  if (fingerMenu->isOpen())
    closeFingerMenu();
  else
    openFingerMenu();
}

// Indicates that a read access has started
void WidgetEngine::readAccessStart(const char *file, int line) {
  core->getThread()->lockMutex(accessMutex,file,line);
  readAccessCount++;
  core->getThread()->unlockMutex(accessMutex);
}

// Indicates that a read access has ended
void WidgetEngine::readAccessStop() {
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  readAccessCount--;
  if (readAccessCount<0) {
    FATAL("too many read access stops",NULL);
  }
  core->getThread()->unlockMutex(accessMutex);
}

// Waits until all read accesses are over
void WidgetEngine::lockAccessAfterReadComplete(const char *file, int line) {
  core->getThread()->lockMutex(accessMutex,file,line);
  while (true) {
    if (readAccessCount<=0)
      break;
    core->getThread()->unlockMutex(accessMutex);
    usleep(1);
    core->getThread()->lockMutex(accessMutex,file,line);
  } 
}

// Schedules a command to execute after graphic operation is done
void WidgetEngine::queueCommand(std::string command, WidgetCheckbox *checkbox) {
  queuedCommands.push_back(WidgetCommandPair(command,checkbox));
}

// Execizes all scheduled commands
void WidgetEngine::executeCommands() {
  for (std::list<WidgetCommandPair>::iterator i=queuedCommands.begin();i!=queuedCommands.end();i++) {
    std::string result=core->getCommander()->execute(i->first);
    if (i->second!=NULL) {
      if (result!="false") {
        device->getGraphicEngine()->lockDrawing(__FILE__,__LINE__);
        i->second->updateCheckedState();
        device->getGraphicEngine()->unlockDrawing();
      }
    }
  }
  queuedCommands.clear();
}

}