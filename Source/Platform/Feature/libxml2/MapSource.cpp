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

/* Reads elements from the GDS XML structure
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
}*/

// Reads information about the map
bool MapSource::resolveGDSInfo(std::string infoFilePath)
{
  std::string mapSourceSchemaFilePath = core->getHomePath() +"/source.xsd";

  // Create a new resolvedGDSInfo object or merge the new info file if object already exists
  if (!resolvedGDSInfo) {

    // Create the object
    if (!(resolvedGDSInfo=new ConfigSection())) {
      FATAL("can not create config section object",NULL);
    }

    // Read the schema file
    if (!resolvedGDSInfo->readSchema(mapSourceSchemaFilePath))
      return false;
  }

  // Read the info file
  if (!resolvedGDSInfo->readConfig(infoFilePath))
    return false;
  resolvedGDSInfo->prepareXPath("http://www.untouchableapps.de/GeoDiscoverer/source/1/0");

  // Check the version
  char *text = (char *)xmlGetProp(xmlDocGetRootElement(resolvedGDSInfo->getConfig()),BAD_CAST "version");
  if (strcmp(text,"1.0")!=0) {
    ERROR("the version <%s> of the GDS file <%s> is not supported",text,infoFilePath.c_str());
    xmlFree(text);
    return false;
  }
  xmlFree(text);

  // Find all MapSourceRef and replace them with the referenced one
  std::list<XMLNode> mapSourceRefNodes=resolvedGDSInfo->findConfigNodes("/GDS/MapSourceRef");
  for (std::list<XMLNode>::iterator i=mapSourceRefNodes.begin();i!=mapSourceRefNodes.end();i++) {

    // Get the fields from the MapSourceRef
    std::string name;
    bool nameFound=resolvedGDSInfo->getNodeText(*i,"name",name);
    double overlayAlpha;
    bool overlayAlphaFound=resolvedGDSInfo->getNodeText(*i,"overlayAlpha",overlayAlpha);
    Int refMinZoomLevel=minZoomLevelDefault;
    bool refMinZoomLevelSet=resolvedGDSInfo->getNodeText(*i,"minZoomLevel",refMinZoomLevel);
    Int refMaxZoomLevel=maxZoomLevelDefault;
    bool refMaxZoomLevelSet=resolvedGDSInfo->getNodeText(*i,"maxZoomLevel",refMaxZoomLevel);
    std::string refLayerGroupName="";
    bool refLayerGroupNameSet=resolvedGDSInfo->getNodeText(*i,"layerGroupName",refLayerGroupName);

    // Does thie entry have a name?
    if (nameFound) {

      // Find the referenced map source
      bool found=false;
      for (std::list<ConfigSection*>::iterator j=availableGDSInfos.begin();j!=availableGDSInfos.end();j++) {
        std::string refName="";
        (*j)->getNodeText("/GDS/name",refName);
        std::string refType="";
        if (!(*j)->getNodeText("/GDS/type",refType))
          refType="tileServer";
        if ((refType=="tileServer")&&(name==refName)) {
          found=true;

          // Update the min and max zoom level
          Int localMinZoomLevel=refMinZoomLevel;
          bool localMinZoomLevelSet=(*j)->getNodeText("/GDS/minZoomLevel",localMinZoomLevel);
          if (refMinZoomLevelSet) {
            localMinZoomLevel=refMinZoomLevel;
            localMinZoomLevelSet=true;
          }
          Int localMaxZoomLevel=refMaxZoomLevel;
          bool localMaxZoomLevelSet=(*j)->getNodeText("/GDS/maxZoomLevel",localMaxZoomLevel);
          if (refMaxZoomLevelSet) {
            localMaxZoomLevel=refMaxZoomLevel;
            localMaxZoomLevelSet=true;
          }

          // Copy the tile server entries from the referenced map source
          std::list<XMLNode> n=(*j)->findConfigNodes("/GDS/TileServer");
          XMLNode currentSibling=*i;
          for (std::list<XMLNode>::iterator k=n.begin();k!=n.end();k++) {

            // Copy the TileServer entry
            XMLNode n2=xmlCopyNode(*k,true);
            if (n2==NULL)
              FATAL("can not copy node",NULL);

            // Update all fields
            Int minZoomLevel=localMinZoomLevel;
            (*j)->getNodeText(n2,"minZoomLevel",minZoomLevel);
            if (localMinZoomLevelSet)
              minZoomLevel=localMinZoomLevel;
            Int maxZoomLevel=localMaxZoomLevel;
            (*j)->getNodeText(n2,"maxZoomLevel",maxZoomLevel);
            if (localMaxZoomLevelSet)
              maxZoomLevel=localMaxZoomLevel;
            std::string layerGroupName=refLayerGroupName;
            (*j)->getNodeText(n2,"layerGroupName",layerGroupName);
            if (refLayerGroupNameSet)
              layerGroupName=refLayerGroupName;

            // Add the min and max zoom level
            if (!(resolvedGDSInfo->setNodeText(n2,"minZoomLevel",localMinZoomLevel)))
              FATAL("can not set minZoomLevel",NULL);
            if (!(resolvedGDSInfo->setNodeText(n2,"maxZoomLevel",localMaxZoomLevel)))
              FATAL("can not set maxZoomLevel",NULL);
            if (layerGroupName!="") {
              if (!(resolvedGDSInfo->setNodeText(n2,"layerGroupName",layerGroupName))) {
                FATAL("can not set layerGroupName",NULL);
              }
            }

            // Update overlay alpha if necessary
            if (overlayAlphaFound) {
              XMLNode n3;
              n3=resolvedGDSInfo->findNode(n2,"overlayAlpha");
              if (n3==NULL) {
                ERROR("one of the tile server entries in the map source <%s> has no overlayAlpha element",name.c_str());
                return false;
              } else {
                double t;
                resolvedGDSInfo->getNodeText(n3,t);
                t*=overlayAlpha;
                std::stringstream t2;
                t2<<t;
                resolvedGDSInfo->setNodeText(n3,t2.str());
              }
            }
            xmlAddNextSibling(currentSibling,n2);
            currentSibling=n2;
          }

          // Unlink the map source ref
          xmlUnlinkNode(*i);
          xmlFreeNode(*i);

          break;
        }
      }

      // Referenced source not found?
      if (!found) {
        ERROR("the referenced map source <%s> in the GDS file <%s> can not be found",name.c_str(),infoFilePath.c_str());
        return false;
      }
    } else {
      ERROR("the GDS file <%s> contains a MapSourceRef that has no name element",infoFilePath.c_str());
      return false;
    }
  }

  // Get the global min and max zoom level
  Int globalMinZoomLevel=minZoomLevelDefault;
  bool globalMinZoomLevelSet=resolvedGDSInfo->getNodeText("/GDS/minZoomLevel",globalMinZoomLevel);
  Int globalMaxZoomLevel=maxZoomLevelDefault;
  bool globalMaxZoomLevelSet=resolvedGDSInfo->getNodeText("/GDS/maxZoomLevel",globalMaxZoomLevel);

  // Go through all tile servers and update the min and max zoom level
  // Find all MapSourceRef and replace them with the referenced one
  std::list<XMLNode> tileServerNodes=resolvedGDSInfo->findConfigNodes("/GDS/TileServer");
  for (std::list<XMLNode>::iterator i=tileServerNodes.begin();i!=tileServerNodes.end();i++) {
    if (globalMaxZoomLevelSet) {
      resolvedGDSInfo->setNodeText(*i,"maxZoomLevel",globalMaxZoomLevel);
    }
    if (globalMinZoomLevelSet) {
      resolvedGDSInfo->setNodeText(*i,"minZoomLevel",globalMinZoomLevel);
    }
    /*std::string t;
    resolvedGDSInfo->getNodeText(*i,"serverURL",t);
    DEBUG("%s",t.c_str());*/
  }
  return true;
}

// Reads information about the map
void MapSource::readAvailableGDSInfos() {

  std::string mapSourceFolderPath = core->getHomePath() + "/Server";
  std::string mapSourceSchemaFilePath = core->getHomePath() +"/source.xsd";

  // Free the available list
  for (std::list<ConfigSection*>::iterator i=availableGDSInfos.begin();i!=availableGDSInfos.end();i++) {
    delete *i;
  }
  availableGDSInfos.clear();

  // First get a list of all available route filenames
  DIR *dp = opendir( mapSourceFolderPath.c_str() );
  struct dirent *dirp;
  struct stat filestat;
  if (dp == NULL){
    FATAL("can not open directory <%s> for reading available map sources",mapSourceFolderPath.c_str());
    return;
  }
  while ((dirp = readdir( dp )))
  {
    std::string filepath = mapSourceFolderPath + "/" + dirp->d_name + "/info.gds";

    // Only look for directories
    if (stat( filepath.c_str(), &filestat ))        continue;
    if (S_ISREG( filestat.st_mode )) {

      // Read the info file
      ConfigSection *c;
      if (!(c=new ConfigSection())) {
        FATAL("can not create config section object",NULL);
      }
      if (!c->readSchema(mapSourceSchemaFilePath))
        return;
      if (c->readConfig(filepath)) {
        c->prepareXPath("http://www.untouchableapps.de/GeoDiscoverer/source/1/0");

        // Check the version
        char *text = (char *)xmlGetProp(xmlDocGetRootElement(c->getConfig()),BAD_CAST "version");
        if (strcmp(text,"1.0")==0) {
          availableGDSInfos.push_back(c);
        } else {
          ERROR("the version (%s) of the GDS file <%s> is not supported",text,filepath.c_str());
          delete c;
        }
        xmlFree(text);
      }
    }
  }
  closedir(dp);
}

}
