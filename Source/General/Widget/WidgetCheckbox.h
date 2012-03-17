//============================================================================
// Name        : WidgetCheckbox.h
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


#ifndef WIDGETCHECKBOX_H_
#define WIDGETCHECKBOX_H_

namespace GEODISCOVERER {

class WidgetCheckbox : public WidgetPrimitive {

protected:

  GraphicTextureInfo checkedTexture;     // Texture when the box is ticked
  GraphicTextureInfo uncheckedTexture;   // Texture when the box is unticked
  std::string checkedCommand;            // Command to execute when the box becomes ticked
  std::string uncheckedCommand;          // Command to execute when the box becomes unticked
  std::string stateConfigPath;           // Path to the config entry that indicates the state
  std::string stateConfigName;           // Name of the config entry that indicates the state
  bool firstTime;                        // Indicates if the work routine has already been called
  std::string configPath;                // Path to this widget in the config

  // Updates the state of the check box
  void update(bool checked, bool executeCommand);

public:

  // Constructor
  WidgetCheckbox();

  // Destructor
  virtual ~WidgetCheckbox();

  // Let the button work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget is touched
  virtual void onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  virtual void onTouchUp(TimestampInMicroseconds t, Int x, Int y);

  // Getters and setters
  void setCheckedCommand(std::string checkedCommand)
  {
      this->checkedCommand = checkedCommand;
  }

  void setCheckedTexture(GraphicTextureInfo checkedTexture)
  {
      this->checkedTexture = checkedTexture;
  }

  void setUncheckedCommand(std::string uncheckedCommand)
  {
      this->uncheckedCommand = uncheckedCommand;
  }

  void setUncheckedTexture(GraphicTextureInfo uncheckedTexture)
  {
      this->uncheckedTexture = uncheckedTexture;
  }

  void setConfigPath(std::string configPath)
  {
      this->configPath = configPath;
  }

  void setStateConfigName(std::string stateConfigName)
  {
      this->stateConfigName = stateConfigName;
  }

  void setStateConfigPath(std::string stateConfigPath)
  {
      this->stateConfigPath = stateConfigPath;
  }

};

}

#endif /* WIDGETCHECKBOX_H_ */
