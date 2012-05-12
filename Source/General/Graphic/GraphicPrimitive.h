//============================================================================
// Name        : GraphicPrimitive.h
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


#ifndef GRAPHICPRIMITIVE_H_
#define GRAPHICPRIMITIVE_H_

namespace GEODISCOVERER {

typedef enum {GraphicTypePrimitive, GraphicTypeRectangle, GraphicTypeRectangleList, GraphicTypeText, GraphicTypeWidget, GraphicTypeLine, GraphicTypeObject } GraphicType;

typedef enum {GraphicBlinkIdle, GraphicBlinkFadeToHighlightColor, GraphicBlinkFadeToOriginalColor} GraphicBlinkMode;

class GraphicPrimitive {

protected:

  GraphicType type;                         // Type of primitive
  GraphicPrimitive *animator;               // Object to use as the animator
  std::list<std::string> name;              // Multiline name of primitive
  Int x;                                    // X coordinate
  Int y;                                    // Y coordinate
  Int z;                                    // Z coordinate
  double angle;                             // Rotation angle in degress
  GraphicColor color;                       // Color of the primitive
  GraphicTextureInfo texture;               // Texture of the primitive
  TimestampInMicroseconds fadeDuration;     // Duration of a fade operation
  TimestampInMicroseconds fadeStartTime;    // Start of the fade operation
  TimestampInMicroseconds fadeEndTime;      // End of the fade operation
  GraphicColor fadeStartColor;              // Start color of a fade operation
  GraphicColor fadeEndColor;                // Stop color of a fade operation
  TimestampInMicroseconds blinkDuration;    // Duration to show the highlight color
  TimestampInMicroseconds blinkPeriod;      // Distance between two blinks
  GraphicColor blinkHighlightColor;         // Color to use when blink is active
  GraphicColor blinkOriginalColor;          // Color before the blink started
  GraphicBlinkMode blinkMode;               // Current blink mode
  bool isUpdated;                           // Indicates that the primitive has been changed
  bool destroyTexture;                      // Indicates if the texture shall be destroyed if the object is deleted

public:

  // Constructor
  GraphicPrimitive();

  // Destructor
  virtual ~GraphicPrimitive();

  // Deinits the primitive
  virtual void deinit();

  // Compares two primitives according to their z value
  static bool zSortPredicate(const GraphicPrimitive *lhs, const GraphicPrimitive *rhs)
  {
    return lhs->getZ() > rhs->getZ();
  }

  // Sets a new fade target
  virtual void setFadeAnimation(TimestampInMicroseconds start, GraphicColor startColor, GraphicColor endColor);

  // Activates or disactivates blinking
  void setBlinkAnimation(bool active, GraphicColor highlightColor);

  // Let the primitive work
  // For example, animations are handled in this method
  virtual bool work(TimestampInMicroseconds currentTime);

  // Recreates any textures or buffers
  virtual void invalidate();

  // Reduce the number of buffers required
  virtual void optimize();

  // Getters and setters
  GraphicColor getColor() const
  {
      return color;
  }

  Int getX() const
  {
      return x;
  }

  Int getY() const
  {
      return y;
  }

  void setColor(GraphicColor color)
  {
      this->color = color;
  }

  void setX(Int x)
  {
      this->x = x;
  }

  virtual void setY(Int y)
  {
      this->y = y;
  }

  Int getZ() const
  {
      return z;
  }

  void setZ(Int z)
  {
      this->z = z;
  }

  GraphicTextureInfo getTexture() const
  {
      return texture;
  }

  void setTexture(GraphicTextureInfo texture)
  {
      this->texture = texture;
  }

  std::list<std::string> getName() const
  {
      return name;
  }

  void setName(std::list<std::string> name)
  {
      this->name = name;
  }

  GraphicType getType() const
  {
      return type;
  }

  bool getIsUpdated() const
  {
      return isUpdated;
  }

  bool getFadeStartTime() const
  {
      return fadeStartTime;
  }

  void setIsUpdated(bool isUpdated)
  {
      this->isUpdated = isUpdated;
  }

  double getAngle() const
  {
      return angle;
  }

  void setAngle(double angle)
  {
      this->angle = angle;
  }

  void setDestroyTexture(bool destroyTexture)
  {
      this->destroyTexture = destroyTexture;
  }

  GraphicPrimitive *getAnimator() const
  {
      return animator;
  }

  void setAnimator(GraphicPrimitive *animator)
  {
      this->animator = animator;
  }
};

}

#endif /* GRAPHICPRIMITIVE_H_ */
