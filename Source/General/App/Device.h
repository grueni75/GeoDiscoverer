//============================================================================
// Name        : Device.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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

  // Opens a connection to the device
  bool openSocket();

  // Closes the connection to the device
  void closeSocket();

public:

  // Constructor
  Device(std::string name, bool whiteBackround, bool animationFriendly);

  // Destructor
  virtual ~Device();

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

  // Destroys the device
  void destroy(bool contextLost);

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
};

} /* namespace GEODISCOVERER */

#endif /* DEVICE_H_ */
