//============================================================================
// Name        : MapSourceMercatorTiles.cpp
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
bool MapSource::readGDSInfo(XMLNode startNode, std::vector<std::string> path) {
  bool result=false;
  for (XMLNode n = startNode; n; n = n->next) {
    if (n->type == XML_ELEMENT_NODE) {
      std::string name=std::string((char*)n->name);
      if (n->children) {
        std::vector<std::string> newPath = path;
        newPath.push_back(name);
        if (readGDSInfo(n->children,newPath)) {
          result=true;
          if (gdsElements.back().size()!=newPath.size()+1) {
            newPath[newPath.size()-1]="/"+name;
            gdsElements.push_back(newPath);
          }
        }
      }
    }
    if (n->type == XML_TEXT_NODE) {
      std::vector<std::string> element;
      for (Int i=0;i<path.size();i++)
        element.push_back(path[i]);
      std::string value = getNodeText(n);
      element.push_back(value);
      gdsElements.push_back(element);
      result=true;
    }
  }
  return result;
}

// Reads information about the map
bool MapSource::readGDSInfo(std::string infoFilePath)
{
  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode,gdsNode,n;
  bool result=false;
  std::string version;
  bool tileServerFound=false;
  std::string name;
  bool minZoomLevelFound,maxZoomLevelFound;
  std::stringstream in;
  xmlChar *text;
  std::vector<std::string> emptyPath;

  // Read the XML file
  gdsElements.clear();
  doc = xmlReadFile(infoFilePath.c_str(), NULL, 0);
  if (!doc) {
    ERROR("can not open file <%s> for reading map source information",infoFilePath.c_str());
    goto cleanup;
  }
  rootNode = xmlDocGetRootElement(doc);
  if (!rootNode) {
    FATAL("could not extract root node",NULL);
    goto cleanup;
  }

  // Check the version
  text = xmlGetProp(rootNode,BAD_CAST "version");
  if (strcmp((char*)text,"1.0")!=0) {
    ERROR("the version (%s) of the GDS file <%s> is not supported",text,infoFilePath.c_str());
    goto cleanup;
  }
  xmlFree(text);

  // Find the GDS node
  gdsNode=NULL;
  for (n = rootNode; n; n = n->next) {
    if (n->type == XML_ELEMENT_NODE) {
      name=std::string((char*)n->name);
      if (name=="GDS") {
        gdsNode=n->children;
        break;
      }
    }
  }
  if (!gdsNode) {
    ERROR("can not find GDS node in <%s>",infoFilePath.c_str());
    goto cleanup;
  }

  // Loop over the root node to extract the information
  readGDSInfo(gdsNode,emptyPath);
  result=true;

  // Clean up
cleanup:
  if (doc)
    xmlFreeDoc(doc);

  return result;
}

}
