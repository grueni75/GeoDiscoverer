//============================================================================
// Name        : WidgetCheckbox.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
  std::string configPath;                // Path to this widget in the config
  TimestampInMicroseconds updateInterval; // Update interval of the widget
  TimestampInMicroseconds nextUpdateTime;   // Next timestamp when to update the widget

  // Updates the state of the check box
  bool update(bool checked, bool executeCommand);

public:

  // Constructor
  WidgetCheckbox(WidgetPage *widgetPage);

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

  void setUpdateInterval(TimestampInMicroseconds updateInterval) {
    this->updateInterval = updateInterval;
  }
};

}

#endif /* WIDGETCHECKBOX_H_ */
