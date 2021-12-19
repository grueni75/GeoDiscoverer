//============================================================================
// Name        : ConfigStore.cpp
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

// Config write thread
void *configStoreWriteThread(void *args) {
  ((ConfigStore*)args)->writeConfig();
  return NULL;
}

// Clean up the backups
void ConfigStore::maintenance() {

  // Clean up backup directory
  //DEBUG("cleaning up backup directory",NULL);
  std::string backupPath=core->getHomePath()+"/Backup";
  struct dirent *dp;
  DIR *dfd;
  TimestampInSeconds t;
  std::list<std::string> backupFiles;
  dfd=core->openDir(backupPath.c_str());
  if (dfd==NULL) {
    FATAL("can not read directory <%s>",backupPath.c_str());
    return;
  }
  while ((dp = readdir(dfd)) != NULL)
  {
    if (sscanf(dp->d_name,"config.%ld.gda",&t)==1) {
      backupFiles.push_back(dp->d_name);
    }
  }
  closedir(dfd);
  //DEBUG("backupFiles.size()=%d",backupFiles.size());
  if (backupFiles.size()>maxConfigBackups) {
    backupFiles.sort();
    while (backupFiles.size()>maxConfigBackups) {
      std::string path=core->getHomePath()+"/Backup/"+backupFiles.front();
      //DEBUG("removing <%s>",path.c_str());
      unlink(path.c_str());
      backupFiles.pop_front();
    }
  }
}

// Indicates that the xml parser is initialized
bool ConfigStore::parserInitialized = false;

// Constructor
ConfigStore::ConfigStore() {

  // Init the store
  configFilepath=core->getHomePath() + "/config.xml";
  schemaShippedFilepath=core->getHomePath() + "/config.shipped.xsd";
  schemaCurrentFilepath=core->getHomePath() + "/config.current.xsd";
  configValueWidth=40;
  accessMutex=core->getThread()->createMutex("config store access mutex");
  quitWriteConfigThread=false;
  writeConfigSignal=core->getThread()->createSignal();
  skipWaitSignal=core->getThread()->createSignal();
  writeConfigThreadInfo=core->getThread()->createThread("config store write config thread",configStoreWriteThread,this);
  if (!(configSection=new ConfigSection())) {
    FATAL("can not create config section object",NULL);
  }
  init();
  read();

  // Read parameters
  writeConfigMinWaitTime=getIntValue("General","writeConfigMinWaitTime", __FILE__, __LINE__);
  maxConfigBackups=getIntValue("General","maxConfigBackups", __FILE__, __LINE__);

  // Check if the Backup dir exists
  struct stat st;
  std::string backupPath=core->getHomePath()+"/Backup";
  if (core->statFile(backupPath, &st) != 0)
  {
    if (mkdir(backupPath.c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      puts("FATAL: can not create backup directory!");
      exit(1);
    }
  }
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
  delete configSection;
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

// Sets an long integer value in the config
void ConfigStore::setLongValue(std::string path, std::string name, long value, const char *file, int line)
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

// Gets a long integer value from the config
long ConfigStore::getLongValue(std::string path, std::string name, const char *file, int line)
{
  std::string value;
  long valueInt;
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
