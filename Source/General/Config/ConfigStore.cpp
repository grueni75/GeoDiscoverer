//============================================================================
// Name        : ConfigStore.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include <Core.h>

namespace GEODISCOVERER {

// Config write thread
void *configStoreWriteThread(void *args) {
  ((ConfigStore*)args)->writeConfig();
  return NULL;
}

// Indicates that the xml parser is initialized
bool ConfigStore::parserInitialized = false;

// Constructor
ConfigStore::ConfigStore() {
  configFilepath=core->getHomePath() + "/config.xml";
  schemaShippedFilepath=core->getHomePath() + "/config.shipped.xsd";
  schemaCurrentFilepath=core->getHomePath() + "/config.current.xsd";
  configValueWidth=40;
  config=NULL;
  schema=NULL;
  xpathConfigCtx=NULL;
  xpathSchemaCtx=NULL;
  accessMutex=core->getThread()->createMutex("config store access mutex");
  quitWriteConfigThread=false;
  writeConfigSignal=core->getThread()->createSignal();
  skipWaitSignal=core->getThread()->createSignal();
  writeConfigThreadInfo=core->getThread()->createThread("config store write config thread",configStoreWriteThread,this);

  init();
  read();

  writeConfigMinWaitTime=getIntValue("General","writeConfigMinWaitTime", __FILE__, __LINE__);
}

// Destructor
ConfigStore::~ConfigStore() {

  // Quit the write config thread
  quitWriteConfigThread=true;
  core->getThread()->issueSignal(writeConfigSignal);
  core->getThread()->issueSignal(skipWaitSignal);
  core->getThread()->waitForThread(writeConfigThreadInfo);
  core->getThread()->destroyThread(writeConfigThreadInfo);
  core->getThread()->destroySignal(writeConfigSignal);
  core->getThread()->destroySignal(skipWaitSignal);
  deinit();
  core->getThread()->destroyMutex(accessMutex);
}

// Sets an integer value in the config
void ConfigStore::setIntValue(std::string path, std::string name, Int value, const char *file, int line)
{
  std::stringstream out;
  out << value;
  std::string valueString = out.str();
  setStringValue(path,name,valueString,file,line);
}

// Sets an integer value in the config
void ConfigStore::setDoubleValue(std::string path, std::string name, double value, const char *file, int line)
{
  std::stringstream out;
  out << value;
  std::string valueString = out.str();
  setStringValue(path,name,valueString,file,line);
}

// Sets a color value in the config
void ConfigStore::setGraphicColorValue(std::string path, GraphicColor value, const char *file, int line) {
  setIntValue(path,"red",value.getRed(),file,line);
  setIntValue(path,"green",value.getGreen(),file,line);
  setIntValue(path,"blue",value.getBlue(),file,line);
  setIntValue(path,"alpha",value.getAlpha(),file,line);
}

// Gets a integer value from the config
Int ConfigStore::getIntValue(std::string path, std::string name, const char *file, int line)
{
  std::string value;
  Int valueInt;
  value=getStringValue(path,name,file,line);
  std::istringstream in(value);
  in >> valueInt;
  return valueInt;

}

// Gets a double value from the config
double ConfigStore::getDoubleValue(std::string path, std::string name, const char *file, int line)
{
  std::string value;
  double valueDouble;
  value=getStringValue(path,name,file,line);
  std::istringstream in(value);
  in >> valueDouble;
  return valueDouble;

}

// Gets a color value from the config
GraphicColor ConfigStore::getGraphicColorValue(std::string path, const char *file, int line) {
  UByte red=getIntValue(path,"red",file,line);
  UByte green=getIntValue(path,"green",file,line);
  UByte blue=getIntValue(path,"blue",file,line);
  UByte alpha=getIntValue(path,"alpha",file,line);
  return GraphicColor(red,green,blue,alpha);
}

// Test if a path exists
bool ConfigStore::pathExists(std::string path, const char *file, int line) {
  core->getThread()->lockMutex(accessMutex,file,line);
  std::list<XMLNode> nodes=findConfigNodes("/GDC/" + path);
  bool result;
  if (nodes.size()>0) {
    result=true;
  } else {
    result=false;
  }
  core->getThread()->unlockMutex(accessMutex);
  return result;
}

// Writes the config if triggered
void ConfigStore::writeConfig() {

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // Do an endless loop
  while (1) {

    // Wait for an update trigger
    core->getThread()->waitForSignal(writeConfigSignal);

    // Write the configuration
    core->getThread()->lockMutex(accessMutex, __FILE__, __LINE__);
    write();
    core->getThread()->unlockMutex(accessMutex);

    // Shall we quit?
    if (quitWriteConfigThread)
      return;

    // Wait a minimum time before writing next
    core->getThread()->waitForSignal(skipWaitSignal,writeConfigMinWaitTime*1000);

  }
}

}
