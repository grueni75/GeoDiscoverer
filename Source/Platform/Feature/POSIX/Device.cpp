//============================================================================
// Name        : Device.cpp
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

namespace GEODISCOVERER {

// Opens a connection to the device
bool Device::openSocket() {
  struct sockaddr_in serv_addr;
  struct hostent *server;
  struct addrinfo *result;
  struct addrinfo *res;
  int error;
  struct addrinfo hints;

  // Get the address info
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  std::stringstream portStringStream;
  portStringStream << port;
  if (getaddrinfo(host.c_str(), portStringStream.str().c_str(), &hints, &result)) {
    DEBUG("can not resolve host %s",host.c_str());
    return false;
  }

  // Try all found addresses
  for (addrinfo *r = result; r != NULL; r = r->ai_next) {

    // Open the connection to the host
    socketfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    if (socketfd < 0) {
      DEBUG("can not create socket",NULL);
      continue;
    }
    if (connect(socketfd,r->ai_addr,r->ai_addrlen) < 0) {
      DEBUG("can not connect socket",NULL);
      close(socketfd);
      socketfd=-1;
      continue;
    }

    break;
  }
  freeaddrinfo(result);

  // No connection found
  if (socketfd<0) {
    DEBUG("can not connect to host %s:%d",host.c_str(),port);
    return false;
  }

  // Set socket timeout
  struct timeval tv;
  memset(&tv,0,sizeof(timeval));
  tv.tv_sec=1;
  if (setsockopt(socketfd,SOL_SOCKET,SO_RCVTIMEO,(const void *)&tv,sizeof(tv))!=0) {
    FATAL("can not set time out on socket",NULL);
    return false;
  }
  if (setsockopt(socketfd,SOL_SOCKET,SO_SNDTIMEO,(const void *)&tv,sizeof(tv))!=0) {
    FATAL("can not set time out on socket",NULL);
    return false;
  }
  return true;
}

// Closes the connection to the device
void Device::closeSocket() {
  if (socketfd>=0) {
    close(socketfd);
    socketfd=-1;
  }
}

// Finds out the device details from a network device
bool Device::discover() {

  Int n;
  UByte buffer[64];

  // Was the device info already discovered?
  if (width>0)
    return true;

  // Ask the server for the screen configuration
  if (!openSocket())
    return false;
  buffer[0]=0x01;
  n = write(socketfd,buffer,1);
  if (n != 1) {
    DEBUG("can not write to host %s:%d",host.c_str(),port);
    close(socketfd);
    socketfd=-1;
    return false;
  }
  n = read(socketfd,buffer,12);
  //DEBUG("n=%d",NULL);
  if (n != 12) {
    DEBUG("can not read from host %s:%d",host.c_str(),port);
    close(socketfd);
    socketfd=-1;
    return false;
  }
  closeSocket();
  n=0;
  orientation=(GraphicScreenOrientation)ntohl(*((uint32_t*)&buffer[n])); n+=4;
  width=ntohl(*((uint32_t*)&buffer[n])); n+=4;
  height=ntohl(*((uint32_t*)&buffer[n])); n+=4;
  DEBUG("DPI=%d orientation=%d width=%d height=%d",DPI,orientation,width,height);

  // Init the device
  init();
  getScreen()->init(orientation,width,height);
  getScreen()->setAllowAllocation(true);
  getScreen()->createGraphic();
  getFontEngine()->createGraphic();
  getWidgetEngine()->createGraphic();
  getGraphicEngine()->createGraphic();
  getScreen()->setAllowAllocation(false);
  NavigationPath *path = core->getDefaultWidgetEngine()->getNearestPath();
  if (path!=NULL)
    getWidgetEngine()->onPathChange(path,NavigationPathChangeTypeWidgetEngineInit);
  MapPosition pos = *core->getNavigationEngine()->lockLocationPos(__FILE__,__LINE__);
  core->getNavigationEngine()->unlockLocationPos();
  getWidgetEngine()->onLocationChange(pos);
  pos = *core->getMapEngine()->lockMapPos(__FILE__,__LINE__);
  core->getMapEngine()->unlockMapPos();
  std::list<MapTile*> *centerMapTiles = core->getMapEngine()->lockCenterMapTiles(__FILE__,__LINE__);
  getWidgetEngine()->onMapChange(pos,centerMapTiles);
  core->getMapEngine()->unlockCenterMapTiles();
  return true;
}

// Sends data to a network device
bool Device::send(UByte *buffer, Int length) {
  Int n = write(socketfd,buffer,length);
  //DEBUG("n=%d length=%d",n,length);
  if (n != length) {
    DEBUG("can not write to host %s:%d",host.c_str(),port);
    close(socketfd);
    socketfd=-1;
    return false;
  }
  return true;
}

// Informs the device that a PNG is sent
bool Device::announcePNGImage() {

  // Tell the server that it will get a PNG image
  if (!openSocket())
    return false;
  //DEBUG("sending cmd to display PNG",NULL);
  UByte cmd=0x02;
  Int n = write(socketfd,&cmd,1);
  if (n != 1) {
    DEBUG("can not write to host %s:%d",host.c_str(),port);
    close(socketfd);
    socketfd=-1;
    return false;
  }
  return true;
}


} /* namespace GEODISCOVERER */
