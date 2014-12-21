//============================================================================
// Name        : WidgetButton.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef WIDGETBUTTON_H_
#define WIDGETBUTTON_H_

namespace GEODISCOVERER {

class WidgetButton : public WidgetPrimitive {

protected:

  std::string command;                        // Command to execute when clicked
  TimestampInMicroseconds nextDispatchTime;   // Time when to execute the next command
  bool repeat;                                // Decides if commands are repeatedly executed

public:

  // Constructors and destructor
  WidgetButton();
  virtual ~WidgetButton();

  // Let the button work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget is touched
  virtual void onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  virtual void onTouchUp(TimestampInMicroseconds t, Int x, Int y);

  // Getters and setters
  void setCommand(std::string command)
  {
      this->command = command;
  }

  void setRepeat(bool repeat) {
    this->repeat = repeat;
  }
};

}

#endif /* WIDGETBUTTON_H_ */
