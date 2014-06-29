//============================================================================
// Name        : MapContainer.cpp
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

// Writes a calibration file
void MapContainer::writeCalibrationFile()
{
  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode = NULL, node;
  std::stringstream out;
  xmlChar *buffer;
  Int size;

  // Init
  //xmlInitParser(); // already done by config store
  out.precision(12);

  // Create an empty XML document
  doc = xmlNewDoc(BAD_CAST "1.0");
  if (!doc) {
    FATAL("can not create xml document",NULL);
    return;
  }
  rootNode = xmlNewNode(NULL, BAD_CAST "GDM");
  if (!rootNode) {
    FATAL("can not create xml root node",NULL);
    return;
  }
  if (!xmlNewProp(rootNode, BAD_CAST "version", BAD_CAST "1.0")) {
    FATAL("can not create xml property",NULL);
    return;
  }
  xmlDocSetRootElement(doc, rootNode);

  // Add the map projection
  std::string mapProjection;
  switch(mapCalibrator->getType()) {
    case MapCalibratorTypeLinear:
      mapProjection="linear";
      break;
    case MapCalibratorTypeSphericalNormalMercator:
      mapProjection="sphericalNormalMercator";
      break;
    case MapCalibratorTypeProj4:
      mapProjection="proj4";
      break;
    default:
      FATAL("unsupported map calibrator type",NULL);
      break;
  }
  if (!xmlNewChild(rootNode,NULL,BAD_CAST "mapProjection",BAD_CAST mapProjection.c_str())) {
    FATAL("can not create xml child node",NULL);
    return;
  }

  // Add the image file name
  if (!xmlNewChild(rootNode,NULL,BAD_CAST "imageFileName",BAD_CAST imageFileName)) {
    FATAL("can not create xml child node",NULL);
    return;
  }

  // Add the zoom level
  out << zoomLevel;
  if (!xmlNewChild(rootNode,NULL,BAD_CAST "zoomLevel",BAD_CAST out.str().c_str())) {
    FATAL("can not create xml child node",NULL);
    return;
  }

  // Add the calibration points
  std::list<MapPosition*> *calibrationPoints=mapCalibrator->getCalibrationPoints();
  for (std::list<MapPosition*>::const_iterator i=calibrationPoints->begin();i!=calibrationPoints->end();i++) {
    MapPosition *p=*i;
    node=xmlNewChild(rootNode,NULL,BAD_CAST "calibrationPoint", NULL);
    if (!node) {
      FATAL("can not create xml child node",NULL);
      break;
    }
    out.str("");
    out << p->getX();
    if (!xmlNewChild(node,NULL,BAD_CAST "x",BAD_CAST out.str().c_str())) {
      FATAL("can not create xml child node",NULL);
      break;
    }
    out.str("");
    out << p->getY();
    if (!xmlNewChild(node,NULL,BAD_CAST "y",BAD_CAST out.str().c_str())) {
      FATAL("can not create xml child node",NULL);
      break;
    }
    out.str("");
    out << p->getLng();
    if (!xmlNewChild(node,NULL,BAD_CAST "longitude",BAD_CAST out.str().c_str())) {
      FATAL("can not create xml child node",NULL);
      break;
    }
    out.str("");
    out << p->getLat();
    if (!xmlNewChild(node,NULL,BAD_CAST "latitude",BAD_CAST out.str().c_str())) {
      FATAL("can not create xml child node",NULL);
      break;
    }
  }

  // Write the file
  xmlDocDumpFormatMemoryEnc(doc, &buffer, &size, "UTF-8", 1);
  std::list<ZipArchive*> *mapArchives=core->getMapSource()->lockMapArchives();
  if (!mapArchives->back()->addEntry(calibrationFilePath,(void *)buffer,size)) {
    ERROR("can not add calibration file <%s> to map archive",calibrationFilePath);
  }
  core->getMapSource()->unlockMapArchives();

  // Clean up
  xmlFreeDoc(doc);
  //xmlCleanupParser(); // will be done by config store

}

// Returns the text contained in a xml node
std::string MapContainer::getNodeText(XMLNode node)
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

// Reads a gdm file
bool MapContainer::readGDMCalibrationFile()
{
  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode,gdmNode,n;
  bool result=false;
  std::string version;
  bool imageFileNameFound=false;
  bool zoomLevelFound=false;
  Int x,y;
  double longitude,latitude;
  bool xFound=false;
  bool yFound=false;
  bool longitudeFound=false;
  bool latitudeFound=false;
  bool mapProjectionFound=false;
  bool mapProjectionArgsFound=false;
  std::string name,name2;
  std::stringstream in;
  MapPosition pos;
  MapCalibratorType calibratorType=MapCalibratorTypeLinear;
  std::string mapProjectionArgs;
  std::list<MapPosition> calibrationPoints;
  xmlChar *text;
  void *buffer;
  Int size;
  std::list<ZipArchive*> *mapArchives;

  // Init
  //xmlInitParser(); // already done by config store

  // Get the contents of the XML file
  mapArchives = core->getMapSource()->lockMapArchives();
  for (std::list<ZipArchive*>::iterator i=mapArchives->begin();i!=mapArchives->end();i++) {
    size = (*i)->getEntrySize(calibrationFilePath);
    if (size>0) {
      if (!(buffer=malloc(size))) {
        FATAL("can not allocate memory",NULL);
        goto cleanup;
      }
      ZipArchiveEntry entry;
      if (!(entry=(*i)->openEntry(calibrationFilePath))) {
        ERROR("can not open file <%s> in map archive for reading map calibration",calibrationFilePath);
        goto cleanup;
      }
      (*i)->readEntry(entry,buffer,size);
      (*i)->closeEntry(entry);
    }
  }
  core->getMapSource()->unlockMapArchives();

  // Read the XML file
  doc = xmlReadMemory((const char *)buffer,size,calibrationFilePath,NULL,0);
  free(buffer);
  if (!doc) {
    ERROR("can not decode file <%s> for reading map calibration",calibrationFilePath);
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
    ERROR("the version (%s) of the GDM file <%s> is not supported",text,calibrationFilePath);
    goto cleanup;
  }
  xmlFree(text);

  // Find the GDM node
  gdmNode=NULL;
  for (n = rootNode; n; n = n->next) {
    if (n->type == XML_ELEMENT_NODE) {
      name=std::string((char*)n->name);
      if (name=="GDM") {
        gdmNode=n->children;
        break;
      }
    }
  }
  if (!gdmNode) {
    ERROR("can not find GDM node in <%s>",calibrationFilePath);
    goto cleanup;
  }

  // Loop over the root node to extract the information
  imageFileNameFound=false;
  zoomLevelFound=false;
  for (n = gdmNode; n; n = n->next) {
    if (n->type == XML_ELEMENT_NODE) {
      name=std::string((char*)n->name);
      if (name=="imageFileName") {
        imageFileNameFound=true;
        setImageFileName(getNodeText(n));
      }
      if (name=="mapProjection") {
        std::string mapProjection=getNodeText(n);
        if (mapProjection=="linear") {
          calibratorType=MapCalibratorTypeLinear;
        } else if ((mapProjection=="mercator")||(mapProjection=="sphericalNormalMercator")) {
          calibratorType=MapCalibratorTypeSphericalNormalMercator;
        } else if (mapProjection=="proj4") {
          calibratorType=MapCalibratorTypeProj4;
        } else {
          ERROR("map projection <%s> defined in <%s> not supported",mapProjection.c_str(),calibrationFilePath);
          goto cleanup;
        }
      }
      if (name=="mapProjectionArgs") {
        mapProjectionArgs=getNodeText(n);
        mapProjectionArgsFound=true;
      }
      if (name=="zoomLevel") {
        zoomLevelFound=true;
        in.str(getNodeText(n));
        in.clear();
        in >> zoomLevel;
      }
      if (name=="calibrationPoint") {
        xFound=false;
        yFound=false;
        longitudeFound=false;
        latitudeFound=false;
        for (xmlNodePtr n2 = n->children; n2; n2 = n2->next) {
          name2=std::string((char*)n2->name);
          in.str(getNodeText(n2));
          in.clear();
          if (name2=="x") {
            xFound=true;
            in >> x;
          }
          if (name2=="y") {
            yFound=true;
            in >> y;
          }
          if (name2=="longitude") {
            longitudeFound=true;
            in >> longitude;
          }
          if (name2=="latitude") {
            latitudeFound=true;
            in >> latitude;
          }
        }
        if ((!xFound)||(!yFound)||(!longitudeFound)||(!latitudeFound)) {
          ERROR("calibration point defined in <%s> does not contain all fields",calibrationFilePath);
          goto cleanup;
        }
        pos.setX(x);
        pos.setY(y);
        pos.setLat(latitude);
        pos.setLng(longitude);
        calibrationPoints.push_back(pos);
      }
    }
  }
  if (!imageFileNameFound) {
    ERROR("image file name not found in <%s>",calibrationFilePath);
    goto cleanup;
  }
  if (!zoomLevelFound) {
    ERROR("zoom level not found in <%s>",calibrationFilePath);
    goto cleanup;
  }
  result=true;

  // Create the calibrator
  if (calibratorType==MapCalibratorTypeProj4) {
    if (!mapProjectionArgsFound) {
      ERROR("map projection arguments not found for map projection type \"proj4\" in <$s>",calibrationFilePath);
      goto cleanup;
    }
  }
  mapCalibrator=MapCalibrator::newMapCalibrator(calibratorType);
  if (!mapCalibrator) {
    FATAL("can not create map calibrator",NULL);
    return false;
  }
  if (mapProjectionArgsFound)
    mapCalibrator->setArgs(mapProjectionArgs);
  mapCalibrator->init();
  for(std::list<MapPosition>::iterator i=calibrationPoints.begin();i!=calibrationPoints.end();i++) {
    mapCalibrator->addCalibrationPoint(*i);
  }

cleanup:

  // Clean up
  if (doc)
    xmlFreeDoc(doc);
  //xmlCleanupParser(); // will be done by config store

  return result;
}


}



