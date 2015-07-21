//============================================================================
// Name        : ConfigStore.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
  void setDoubleValue(std::string path, std::string name, double value, const char *file, int line);
  void setGraphicColorValue(std::string path, GraphicColor value, const char *file, int line);
  std::string getStringValue(std::string path, std::string name, const char *file, int line);
  Int getIntValue(std::string path, std::string name, const char *file, int line);
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

};

}

#endif /* CONFIGSTORE_H_ */
