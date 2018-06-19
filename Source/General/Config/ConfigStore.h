//============================================================================
// Name        : ConfigStore.h
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

#ifndef CONFIGSTORE_H_
#define CONFIGSTORE_H_

namespace GEODISCOVERER {

typedef std::map<std::string, std::string> StringMap;
typedef std::pair<std::string, std::string> StringPair;

class ConfigStore {

protected:

  // Indicates that the xml parser is initialized
  static bool parserInitialized;

  // Name of the config file
  std::string configFilepath;

  // Name of the shipped schema file
  std::string schemaShippedFilepath;

  // Name of the current schema file
  std::string schemaCurrentFilepath;

  // Width of the value field in config file
  Int configValueWidth;

  // Mutex to sequentialize changes in the config
  ThreadMutexInfo *accessMutex;

  // Main config
  ConfigSection *configSection;

  // Minimum distance in seconds between writes of the config store
  Int writeConfigMinWaitTime;

  // Indicates if the config has changed
  bool hasChanged;

  // Variables for the write config thread
  ThreadSignalInfo *writeConfigSignal;
  ThreadSignalInfo *skipWaitSignal;
  ThreadInfo *writeConfigThreadInfo;
  bool quitWriteConfigThread;

  // List of user data extracted from old config
  std::list<std::vector<std::string> > migratedUserConfig;

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

  // Reads the config with a given schema
  void read(std::string schemaFilepath, bool recreateConfig);

  // Extracts all schema nodes that hold user-definable values
  void rememberUserConfig(std::string path, XMLNode nodes);

public:

  // Called when the core is unloaded (process is killed)
  static void unload();

  // Constructor
  ConfigStore();

  // Destructor
  virtual ~ConfigStore();

  // Test if a path exists
  bool pathExists(std::string path, const char *file, int line);

  // Config write thread function
  void writeConfig();

  // Access methods to config values
  void setStringValue(std::string path, std::string name, std::string value, const char *file, int line);
  void setIntValue(std::string path, std::string name, Int value, const char *file, int line);
  void setLongValue(std::string path, std::string name, long value, const char *file, int line);
  void setDoubleValue(std::string path, std::string name, double value, const char *file, int line);
  void setGraphicColorValue(std::string path, GraphicColor value, const char *file, int line);
  std::string getStringValue(std::string path, std::string name, const char *file, int line);
  Int getIntValue(std::string path, std::string name, const char *file, int line);
  long getLongValue(std::string path, std::string name, const char *file, int line);
  double getDoubleValue(std::string path, std::string name, const char *file, int line);
  GraphicColor getGraphicColorValue(std::string path, const char *file, int line);

  // Returns a list of attribute values for a given path and attribute name
  std::list<std::string> getAttributeValues(std::string path, std::string attributeName, const char *file, int line);

  // Lists all elements for the given path
  std::list<std::string> getNodeNames(std::string path);

  // Returns information about the given node
  StringMap getNodeInfo(std::string path);

  // Removes the node from the config
  void removePath(std::string path);

  // Encodes a string into a compatible representation
  std::string encodeString(std::string name);

};

}

#endif /* CONFIGSTORE_H_ */
