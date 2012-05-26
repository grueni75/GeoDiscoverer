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
std::string MapSourceMercatorTiles::getNodeText(XMLNode node)
{
  xmlNodePtr n;
  for (n = node->children; n; n = n->next) {
    std::string name=std::string((char*)n->name);
    if (name=="text") {
      return std::string((char*)n->content);
    }
  }
  return "";
}

// Reads information about the map
bool MapSourceMercatorTiles::readGDSInfo()
{
  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode,gdsNode,n;
  bool result=false;
  std::string version;
  bool tileServerURLFound=false;
  std::string name;
  std::string infoFilePath=getFolderPath() + "/info.gds";
  bool minZoomLevelFound,maxZoomLevelFound;
  std::stringstream in;
  xmlChar *text;

  // Read the XML file
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
  tileServerURLFound=false;
  minZoomLevelFound=false;
  maxZoomLevelFound=false;
  for (n = gdsNode; n; n = n->next) {
    if (n->type == XML_ELEMENT_NODE) {
      name=std::string((char*)n->name);
      in.str(getNodeText(n));
      in.clear();
      if (name=="tileServerURL") {
        tileServerURLFound=true;
        tileServerURL=getNodeText(n);
      }
      if (name=="minZoomLevel") {
        minZoomLevelFound=true;
        in >> minZoomLevel;
      }
      if (name=="maxZoomLevel") {
        maxZoomLevelFound=true;
        in >> maxZoomLevel;
      }
    }
  }
  if (!tileServerURLFound) {
    ERROR("tileServerURL not found in <%s>",infoFilePath.c_str());
    goto cleanup;
  }
  if (!minZoomLevelFound) {
    ERROR("minZoomLevel not found in <%s>",infoFilePath.c_str());
    goto cleanup;
  }
  if (!maxZoomLevelFound) {
    ERROR("maxZoomLevel not found in <%s>",infoFilePath.c_str());
    goto cleanup;
  }
  result=true;

cleanup:

  // Clean up
  if (doc)
    xmlFreeDoc(doc);
  //xmlCleanupParser(); // will be done by config store

  return result;
}

}
