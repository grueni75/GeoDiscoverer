//============================================================================
// Name        : WidgetButton.h
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
  WidgetButton(WidgetPage *widgetPage);
  virtual ~WidgetButton();

  // Let the button work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget is touched
  virtual void onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  virtual void onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel);

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
