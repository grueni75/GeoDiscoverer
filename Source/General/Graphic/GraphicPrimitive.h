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

  GraphicType type;                                     // Type of primitive
  GraphicPrimitive *animator;                           // Object to use as the animator
  std::list<std::string> name;                          // Multiline name of primitive
  Int x;                                                // X coordinate
  Int y;                                                // Y coordinate
  Int z;                                                // Z coordinate
  double angle;                                         // Rotation angle in degress
  double scale;                                         // Scale factor
  GraphicColor color;                                   // Color of the primitive
  GraphicTextureInfo texture;                           // Texture of the primitive
  TimestampInMicroseconds fadeDuration;                 // Duration of a fade operation
  TimestampInMicroseconds fadeStartTime;                // Start of the fade operation
  TimestampInMicroseconds fadeEndTime;                  // End of the fade operation
  GraphicColor fadeStartColor;                          // Start color of a fade operation
  GraphicColor fadeEndColor;                            // Stop color of a fade operation
  TimestampInMicroseconds blinkDuration;                // Duration to show the highlight color
  TimestampInMicroseconds blinkPeriod;                  // Distance between two blinks
  GraphicColor blinkHighlightColor;                     // Color to use when blink is active
  GraphicColor blinkOriginalColor;                      // Color before the blink started
  GraphicBlinkMode blinkMode;                           // Current blink mode
  TimestampInMicroseconds rotateDuration;               // Duration of a rotate operation
  TimestampInMicroseconds rotateStartTime;              // Start of the rotate operation
  TimestampInMicroseconds rotateEndTime;                // End of the rotate operation
  double rotateStartAngle;                              // Start angle of rotation
  double rotateEndAngle;                                // End angle of rotation
  bool rotateInfinite;                                  // Rotation is repeated infinitely if set
  TimestampInMicroseconds scaleDuration;                // Duration of a scale operation
  TimestampInMicroseconds scaleStartTime;               // Start of the scale operation
  TimestampInMicroseconds scaleEndTime;                 // End of the scale operation
  double scaleStartFactor;                              // Start factor of scale operation
  double scaleEndFactor;                                // End factor of scale operation
  bool scaleInfinite;                                   // Rotation is repeated infinitely if set
  TimestampInMicroseconds translateDuration;            // Duration of a translate operation
  TimestampInMicroseconds translateStartTime;           // Start of the translate operation
  TimestampInMicroseconds translateEndTime;             // End of the translate operation
  Int translateStartX;                                  // Start x coordinate of translate operation
  Int translateStartY;                                  // Start y coordinate of translate operation
  Int translateEndX;                                    // End x coordinate of translate operation
  Int translateEndY;                                    // End y coordinate of translate operation
  bool translateInfinite;                               // Translation is repeated infinitely if set
  bool isUpdated;                                       // Indicates that the primitive has been changed
  bool destroyTexture;                                  // Indicates if the texture shall be destroyed if the object is deleted

  // List of scale animations to execute on this object
  std::list<GraphicScaleAnimationParameter> scaleAnimationSequence;

  // List of translate animations to execute on this object
  std::list<GraphicTranslateAnimationParameter> translateAnimationSequence;

  // List of rotate animations to execute on this object
  std::list<GraphicRotateAnimationParameter> rotateAnimationSequence;

  // Sets the next scale animation step from the sequence
  void setNextScaleAnimationStep();

  // Sets the next translate animation step from the sequence
  void setNextTranslateAnimationStep();

  // Sets the next rotate animation step from the sequence
  void setNextRotateAnimationStep();

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
  virtual void setFadeAnimation(TimestampInMicroseconds startTime, GraphicColor startColor, GraphicColor endColor);

  // Activates or disactivates blinking
  void setBlinkAnimation(bool active, GraphicColor highlightColor);

  // Sets a new rotation target
  void setRotateAnimation(TimestampInMicroseconds startTime, double startAngle, double endAngle, bool infinite, TimestampInMicroseconds duration);

  // Sets a new scale target
  void setScaleAnimation(TimestampInMicroseconds startTime, double startFactor, double endFactor, bool infinite, TimestampInMicroseconds duration);

  // Sets a new translate target
  void setTranslateAnimation(TimestampInMicroseconds startTime, Int startX, Int startY, Int endX, Int endY, bool infinite, TimestampInMicroseconds duration);

  // Let the primitive work
  // For example, animations are handled in this method
  virtual bool work(TimestampInMicroseconds currentTime);

  // Recreates any textures or buffers
  virtual void invalidate();

  // Reduce the number of buffers required
  virtual void optimize();

  // Getters and setters
  void setScaleAnimationSequence(std::list<GraphicScaleAnimationParameter> scaleAnimationSequence);

  void setTranslateAnimationSequence(std::list<GraphicTranslateAnimationParameter> scaleAnimationSequence);

  void setRotateAnimationSequence(std::list<GraphicRotateAnimationParameter> rotateAnimationSequence);

  std::list<GraphicRotateAnimationParameter> getRotateAnimationSequence() const {
    return rotateAnimationSequence;
  }

  std::list<GraphicScaleAnimationParameter> getScaleAnimationSequence() const {
    return scaleAnimationSequence;
  }

  std::list<GraphicTranslateAnimationParameter> getTranslateAnimationSequence() const {
    return translateAnimationSequence;
  }

  double getScale() const {
    return scale;
  }

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

  void setScale(double scale)
  {
      this->scale = scale;
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
