//============================================================================
// Name        : WidgetPage.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef WIDGETPAGE_H_
#define WIDGETPAGE_H_

namespace GEODISCOVERER {

class WidgetPage {

protected:

  WidgetEngine *widgetEngine;                           // Widget engine this page belongs to
  std::string name;                                     // Name of the page
  GraphicObject graphicObject;                          // Contains the widgets of this page
  bool widgetsActive;                                   // Indicates if the widgets are active
  bool touchStartedOutside;                             // Indicates that the widgets were not touched directly at the beginning
  bool firstTouch;                                      // Indicates that no touch was done before
  WidgetPrimitive *selectedWidget;                      // The currently selected widget
  TimestampInMicroseconds touchEndTime;                 // Last time no widget was touched
  bool lastTouchStartedOutside;                         // Indicates if the last touch was not hitting any widgets

public:

  // Constructors and destructor
  WidgetPage(WidgetEngine *widgetEngine, std::string name);
  virtual ~WidgetPage();

  // Adds a widget to the page
  void addWidget(WidgetPrimitive *primitive);

  // Removes all widgets
  void deinit(bool deleteWidgets=true);

  // Called when the screen is touched
  bool onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff);

  // Called when the widget is touched
  bool onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  void onTouchUp(TimestampInMicroseconds t, Int x, Int y);

  // Called when the map has changed
  void onMapChange(bool pageVisible, MapPosition pos);

  // Called when the location has changed
  void onLocationChange(bool pageVisible, MapPosition pos);

  // Called when a path has changed
  void onPathChange(bool pageVisible, NavigationPath *path, NavigationPathChangeType changeType);

  // Let the page work
  bool work(TimestampInMicroseconds t);

  // Deselects currently selected widget
  void deselectWidget(TimestampInMicroseconds t);

  // Sets the active state of the widgets
  void setWidgetsActive(TimestampInMicroseconds t, bool widgetsActive);

  // Getters and setters
  GraphicObject *getGraphicObject()
  {
      return &graphicObject;
  }
  std::string getName() const
  {
      return name;
  }

  bool getWidgetsActive() const {
    return widgetsActive;
  }

  FontEngine *getFontEngine();

  WidgetEngine *getWidgetEngine();

  GraphicEngine *getGraphicEngine();

  Screen *getScreen();
};

}

#endif /* WIDGETPAGE_H_ */
