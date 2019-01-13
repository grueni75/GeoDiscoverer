//============================================================================
// Name        : Device.h
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

#ifndef DEVICE_H_
#define DEVICE_H_

namespace GEODISCOVERER {

class Device {

protected:

  // Name of the device
  std::string name;

  // Use white background
  bool whiteBackground;

  // Device can do animations nicely
  bool animationFriendly;

  // IP address of the server
  std::string host;

  // TCP port at the server
  Int port;

  // File descriptor of the socket that communicates with the server
  Int socketfd;

  // Density of the screen
  Int DPI;

  // Defines how long the socket shall wait for a progressing communication
  TimestampInSeconds socketTimeout;

  // Diagonal of the screen
  double diagonal;

  // Components of the device
  Screen *screen;
  FontEngine *fontEngine;
  WidgetEngine *widgetEngine;
  GraphicEngine *graphicEngine;

  // Number of frames without any change
  Int noChangeFrameCount;

  // The currently visible pages
  GraphicObject *visibleWidgetPages;

  // Properties of the screen
  GraphicScreenOrientation orientation;
  Int width;
  Int height;

  // Indicates that the device is initialized
  bool initDone;

  // Indicates if this device is a watch
  bool isWatch;

public:

  // Constructor
  Device(std::string name, bool whiteBackround, bool animationFriendly);

  // Destructor
  virtual ~Device();

  // Opens a connection to the device
  bool openSocket();

  // Closes the connection to the device
  void closeSocket();

  // Creates the components
  void init();

  // Reconfigures the device based on the new dimension
  void reconfigure();

  // Finds out the device details from a network device
  bool discover();

  // Performs the drawing on this device
  bool draw();

  // Informs the device that a PNG is sent
  bool announcePNGImage();

  // Sends data to a network device
  bool send(UByte *buffer, Int length);

  // Destroys the graphic of the device
  void destroyGraphic(bool contextLost);

  // Creates the graphic of the device
  void createGraphic();

  // Getters and setters
  Screen* getScreen() {
    return screen;
  }

  FontEngine *getFontEngine() {
    return fontEngine;
  }

  WidgetEngine *getWidgetEngine() {
    return widgetEngine;
  }

  GraphicEngine *getGraphicEngine() {
    return graphicEngine;
  }

  Int getNoChangeFrameCount() const {
    return noChangeFrameCount;
  }

  void setNoChangeFrameCount(Int noChangeFrameCount) {
    this->noChangeFrameCount = noChangeFrameCount;
  }

  const std::string& getName() const {
    return name;
  }

  GraphicObject *getVisibleWidgetPages() {
    return visibleWidgetPages;
  }

  void setVisibleWidgetPages(GraphicObject *visiblePages) {
    this->visibleWidgetPages = visiblePages;
  }

  void setHeight(Int height) {
    this->height = height;
  }

  void setOrientation(GraphicScreenOrientation orientation) {
    this->orientation = orientation;
  }

  void setWidth(Int width) {
    this->width = width;
  }

  void setHost(std::string host) {
    this->host = host;
  }

  void setPort(Int port) {
    this->port = port;
  }

  void setDiagonal(double diagonal) {
    this->diagonal = diagonal;
  }

  void setDPI(Int DPI) {
    this->DPI = DPI;
  }

  double getDiagonal() const {
    return diagonal;
  }

  Int getDPI() const {
    return DPI;
  }

  bool getWhiteBackground() const {
    return whiteBackground;
  }

  bool isAnimationFriendly() const {
    return animationFriendly;
  }

  void setInitDone() {
    this->initDone = true;
  }

  bool isInitDone() const {
    return initDone;
  }

  bool getIsWatch() const {
    return isWatch;
  }
};

} /* namespace GEODISCOVERER */

#endif /* DEVICE_H_ */
