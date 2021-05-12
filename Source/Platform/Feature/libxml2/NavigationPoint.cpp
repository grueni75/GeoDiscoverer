//============================================================================
// Name        : NavigationPoint.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2019 Matthias Gruenewald
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
#include <NavigationPoint.h>
#include <MapPosition.h>

namespace GEODISCOVERER {

// Checks if the namespace of the given xml node is a valid gpx namespace
bool NavigationPoint::isGPXNameSpace(XMLNode node) {
  if (node->ns!=NULL) {
    std::string href=(char*)node->ns->href;
    if (href == GPX10Namespace)
      return true;
    if (href == GPX11Namespace)
      return true;
    return false;
  } else {
    return true;
  }
}

// Extracts the point from the gpx xml node list
bool NavigationPoint::readGPX(XMLNode wptNode, std::string group, std::string defaultName, std::string &error) {

  // Check namespace
  if (!isGPXNameSpace(wptNode)) {
    FATAL("xml node has unexpected namespace",NULL);
    return false;
  }

  // Fill out already known fields
  this->address="GPX Waypoint has no address";
  setName(defaultName);
  this->group=group;

  // Extract longitude and latitude from attributes
  xmlChar *text;
  std::istringstream iss;
  if (!(text=xmlGetProp(wptNode,BAD_CAST "lat"))) {
    error="contains a waypoint that has no latitude";
    return false;
  }
  iss.str((char*)text); iss.clear(); iss >> lat;
  if (text) xmlFree(text);
  if (!(text=xmlGetProp(wptNode,BAD_CAST "lon"))) {
    error="contains a waypoint that has no longitude";
    return false;
  }
  iss.str((char*)text); iss.clear(); iss >> lng;
  if (text) xmlFree(text);

  // If there are no children stop here
  if (!wptNode->children)
    return true;

  // Go through children and extract standard information
  XMLNode extensionsNode=NULL;
  std::string gpxName="";
  std::string gpxComment="";
  std::string gpxDescription="";
  std::string gpxType="";
  for (XMLNode node=wptNode->children;node!=NULL;node=node->next) {
    if ((isGPXNameSpace(node))&&(node->type==XML_ELEMENT_NODE)) {
      std::string name=(char*)node->name;
      std::string text;
      if (ConfigSection::getNodeText(node,text)) {
        if (name == "name") {
          gpxName=text;
        }
        if (name == "desc") {
          gpxDescription=text;
        }
        if (name == "cmt") {
          gpxComment=text;
        }
        if (name == "type") {
          gpxType=text;
        }
      }
    }
  }
  if (gpxType!="") {
    error="contains a routing waypoint";
    return false;
  }
  if (gpxDescription!="")
    setName(gpxDescription);
  else if (gpxName!="")
    setName(gpxName);
  else if (gpxComment!="")
    setName(gpxComment);
  return true;
}

}
