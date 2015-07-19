//============================================================================
// Name        : MapSourceMercatorTiles.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include "Core.h"

namespace GEODISCOVERER {

// Returns the text contained in a xml node
std::string MapSource::getNodeText(XMLNode node)
{
  xmlNodePtr n;
  for (n = node; n; n = n->next) {
    std::string name=std::string((char*)n->name);
    if (name=="text") {
      return std::string((char*)n->content);
    }
  }
  return "";
}

// Reads elements from the GDS XML structure
bool MapSource::resolveGDSInfo(XMLNode startNode, std::vector<std::string> path) {
  bool result=false;
  //DEBUG("path.size()=%d",path.size());
  for (XMLNode n = startNode; n; n = n->next) {
    if (n->type == XML_ELEMENT_NODE) {
      std::string name=std::string((char*)n->name);
      if (n->children) {
        std::vector<std::string> newPath = path;
        newPath.push_back(name);
        //DEBUG("name=%s",name.c_str());
        if (readGDSInfo(n->children,newPath)) {
          result=true;
          if (gdsElements.back().size()!=newPath.size()+1) {
            newPath[newPath.size()-1]="/"+name;
            //DEBUG("end of name=%s",newPath[newPath.size()-1].c_str());
            gdsElements.push_back(newPath);
          }
        }
      }
    }
    if (n->type == XML_TEXT_NODE) {
      std::string value = getNodeText(n);
      bool emptyString=true;
      for (Int i=0;i<value.length();i++) {
        if ((value[i]!='\r')&&(value[i]!='\n')&&(value[i]!=' ')&&(value[i]!='\t'))
          emptyString=false;
      }
      if (!emptyString) {
        std::vector<std::string> element;
        for (Int i=0;i<path.size();i++)
          element.push_back(path[i]);
        element.push_back(value);
        //DEBUG("value=%s",value.c_str());
        gdsElements.push_back(element);
      }
      result=true;
    }
  }
  return result;
}

// Reads information about the map
bool MapSource::resolveGDSInfo(std::string infoFilePath)
{
  std::string mapSourceSchemaFilePath = core->getHomePath() +"/source.xsd";

  // Read the info file
  if (!resolvedGDSInfo.readSchema(mapSourceSchemaFilePath))
    return false;
  if (!resolvedGDSInfo.readConfig(infoFilePath))
    return false;
  resolvedGDSInfo.prepareXPath("http://www.untouchableapps.de/GeoDiscoverer/source/1/0");

  // Check the version
  text = xmlGetProp(rootNode,BAD_CAST "version");
  if (strcmp((char*)text,"1.0")!=0) {
    ERROR("the version (%s) of the GDS file <%s> is not supported",text,infoFilePath.c_str());
    goto cleanup;
  }
  xmlFree(text);

  replace all refs
  decide on zoom level and remove any zoom level element in doc
  fill the layer name map and remove any laver name map in doc

}

// Reads information about the map
void MapSource::readAvailableGDSInfos() {

  std::string mapSourceFolderPath = core->getHomePath() + "/Source";
  std::string mapSourceSchemaFilePath = core->getHomePath() +"/source.xsd";

  // First get a list of all available route filenames
  DIR *dp = opendir( mapSourceFolderPath.c_str() );
  struct dirent *dirp;
  struct stat filestat;
  std::list<std::string> routes;
  if (dp == NULL){
    FATAL("can not open directory <%s> for reading available map sources",mapSourceFolderPath.c_str());
    return;
  }
  while ((dirp = readdir( dp )))
  {
    std::string filepath = mapSourceFolderPath + "/" + dirp->d_name + "/info.gds";

    // Only look for directories
    if (stat( filepath.c_str(), &filestat ))        continue;
    if (S_ISDIR( filestat.st_mode )) {

      // Read the info file
      ConfigSection c;
      if (!c.readSchema(mapSourceSchemaFilePath))
        return;
      if (c.readConfig(filepath)) {
        c.prepareXPath("http://www.untouchableapps.de/GeoDiscoverer/source/1/0");

        // Check the version
        char *text = xmlGetProp(xmlDocGetRootElement(c.getConfig()),BAD_CAST "version");
        if (strcmp(text,"1.0")==0) {
          availableGDSInfos.push_back(c);
        } else {
          ERROR("the version (%s) of the GDS file <%s> is not supported",text,filepath.c_str());
        }
        xmlFree(text);
      }
    }
  }
  closedir(dp);
}

}
