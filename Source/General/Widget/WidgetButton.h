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

#include <WidgetPrimitive.h>

#ifndef WIDGETBUTTON_H_
#define WIDGETBUTTON_H_

namespace GEODISCOVERER {

class WidgetButton : public WidgetPrimitive {

protected:

  std::string command;                        // Command to execute when clicked
  std::string longPressCommand;               // Command to execute when long pressed
  TimestampInMicroseconds nextDispatchTime;   // Time when to execute the next command
  TimestampInMicroseconds longPressTime;      // Time when to execute the long press command
  bool repeat;                                // Decides if commands are repeatedly executed
  bool skipCommand;                           // Decides if the command shall be executed when widget is untouched
  bool safetyStep;                            // Indicates that the command is only executed after the user confirms
  GraphicColor safetyStepColor;               // Color to use for the safety step
  TimestampInMicroseconds safetyStepDuration; // Duration the safety step remains active
  bool safetyState;                           // Indicates that the widget is in the safety state
  TimestampInMicroseconds safetyStateStart;   // Time when to start the safety state
  TimestampInMicroseconds safetyStateEnd;     // Time when to end the safety state

public:

  // Constructors and destructor
  WidgetButton(WidgetContainer *widgetContainer);
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

  void setLongPressCommand(std::string longPressCommand)
  {
      this->longPressCommand = longPressCommand;
  }

  void setRepeat(bool repeat) {
    this->repeat = repeat;
  }

  void setSafetyStep(bool safetyStep) {
    this->safetyStep=safetyStep;
  }

  void setSafetyStepColor(GraphicColor safetyStepColor) {
    this->safetyStepColor=safetyStepColor;
  }

  void setSafetyStepDuration(TimestampInMicroseconds safetyStepDuration) {
    this->safetyStepDuration=safetyStepDuration;
  }
};

}

#endif /* WIDGETBUTTON_H_ */
