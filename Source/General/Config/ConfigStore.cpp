//============================================================================
// Name        : ConfigStore.cpp
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

// Indicates that the xml parser is initialized
bool ConfigStore::parserInitialized = false;

// Constructor
ConfigStore::ConfigStore() {
  configFilepath=core->getHomePath() + "/config.xml";
  schemaFilepath=core->getHomePath() + "/config.xsd";
  configValueWidth=40;
  config=NULL;
  schema=NULL;
  xpathConfigCtx=NULL;
  xpathSchemaCtx=NULL;
  mutex=core->getThread()->createMutex();
  init();
  read();
}

// Destructor
ConfigStore::~ConfigStore() {
  deinit();
  core->getThread()->destroyMutex(mutex);
}

// Sets an integer value in the config
void ConfigStore::setIntValue(std::string path, std::string name, Int value)
{
  std::stringstream out;
  out << value;
  std::string valueString = out.str();
  setStringValue(path,name,valueString);
}

// Sets an integer value in the config
void ConfigStore::setDoubleValue(std::string path, std::string name, double value)
{
  std::stringstream out;
  out << value;
  std::string valueString = out.str();
  setStringValue(path,name,valueString);
}

// Sets a color value in the config
void ConfigStore::setGraphicColorValue(std::string path, GraphicColor value) {
  setIntValue(path,"red",value.getRed());
  setIntValue(path,"green",value.getGreen());
  setIntValue(path,"blue",value.getBlue());
  setIntValue(path,"alpha",value.getAlpha());
}

// Gets a integer value from the config
Int ConfigStore::getIntValue(std::string path, std::string name)
{
  std::string value;
  Int valueInt;
  value=getStringValue(path,name);
  std::istringstream in(value);
  in >> valueInt;
  return valueInt;

}

// Gets a double value from the config
double ConfigStore::getDoubleValue(std::string path, std::string name)
{
  std::string value;
  double valueDouble;
  value=getStringValue(path,name);
  std::istringstream in(value);
  in >> valueDouble;
  return valueDouble;

}

// Gets a color value from the config
GraphicColor ConfigStore::getGraphicColorValue(std::string path) {
  UByte red=getIntValue(path,"red");
  UByte green=getIntValue(path,"green");
  UByte blue=getIntValue(path,"blue");
  UByte alpha=getIntValue(path,"alpha");
  return GraphicColor(red,green,blue,alpha);
}

// Test if a path exists
bool ConfigStore::pathExists(std::string path) {
  core->getThread()->lockMutex(mutex);
  std::list<XMLNode> nodes=findConfigNodes("/GDC/" + path);
  bool result;
  if (nodes.size()>0) {
    result=true;
  } else {
    result=false;
  }
  core->getThread()->unlockMutex(mutex);
  return result;
}

}
