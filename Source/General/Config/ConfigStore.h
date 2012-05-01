//============================================================================
// Name        : ConfigStore.h
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

#ifndef CONFIGSTORE_H_
#define CONFIGSTORE_H_

#include <ConfigStorePlatform.h>

namespace GEODISCOVERER {

typedef std::map<std::string, std::string> StringMap;
typedef std::pair<std::string, std::string> StringPair;

class ConfigStore {

protected:

  // Name of the config file
  std::string configFilepath;

  // Name of the schema file
  std::string schemaFilepath;

  // Width of the value field in config file
  Int configValueWidth;

  // Pointer to the schema
  XMLDocument schema;

  // Pointer to the data
  XMLDocument config;

  // Context for using xpath
  XMLXPathContext xpathConfigCtx;
  XMLXPathContext xpathSchemaCtx;

  // Mutex to sequentialize changes in the config
  ThreadMutexInfo *mutex;

  // Inits the data
  void init();

  // Deinits the data
  void deinit();

  // Writes the config to disk
  void write();

  // Reads the config
  void read();

  // Creates a node inclusive its path
  XMLNode createNodeWithPath(XMLNode parentNode, std::string path, std::string name, std::string value);

  // Finds a set of config nodes at the given path
  std::list<XMLNode> findConfigNodes(std::string path);

  // Finds a set of schema nodes at the given path
  std::list<XMLNode> findSchemaNodes(std::string path, std::string extension="");

public:

  // Constructor
  ConfigStore();

  // Destructor
  virtual ~ConfigStore();

  // Test if a path exists
  bool pathExists(std::string path);

  // Access methods to config values
  void setStringValue(std::string path, std::string name, std::string value, bool innerCall=false);
  void setIntValue(std::string path, std::string name, Int value);
  void setDoubleValue(std::string path, std::string name, double value);
  void setGraphicColorValue(std::string path, GraphicColor value);
  std::string getStringValue(std::string path, std::string name);
  Int getIntValue(std::string path, std::string name);
  double getDoubleValue(std::string path, std::string name);
  GraphicColor getGraphicColorValue(std::string path);

  // Returns a list of attribute values for a given path and attribute name
  std::list<std::string> getAttributeValues(std::string path, std::string attributeName);

  // Lists all elements for the given path
  std::list<std::string> getNodeNames(std::string path);

  // Returns information about the given node
  StringMap getNodeInfo(std::string path);

  // Removes the node from the config
  void removePath(std::string path);

};

}

#endif /* CONFIGSTORE_H_ */
