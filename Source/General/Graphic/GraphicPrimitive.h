//============================================================================
// Name        : GraphicPrimitive.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef GRAPHICPRIMITIVE_H_
#define GRAPHICPRIMITIVE_H_

namespace GEODISCOVERER {

typedef enum {GraphicTypePrimitive, GraphicTypeRectangle, GraphicTypeRectangleList, GraphicTypeText, GraphicTypeWidget, GraphicTypeLine, GraphicTypeObject } GraphicType;

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
  TimestampInMicroseconds textureDuration;              // Duration of a texture animation
  TimestampInMicroseconds textureStartTime;             // Start of the texture animation
  TimestampInMicroseconds textureEndTime;               // End of the texture animation
  GraphicTextureInfo textureStartInfo;                  // Start texture of the texture animation
  GraphicTextureInfo textureEndInfo;                    // Stop texture of the texture animation
  bool textureInfinite;                                 // Texture animation is repeated infinitely if set
  TimestampInMicroseconds fadeDuration;                 // Duration of a fade operation
  TimestampInMicroseconds fadeStartTime;                // Start of the fade operation
  TimestampInMicroseconds fadeEndTime;                  // End of the fade operation
  GraphicColor fadeStartColor;                          // Start color of a fade operation
  GraphicColor fadeEndColor;                            // Stop color of a fade operation
  bool fadeInfinite;                                    // Fade is repeated infinitely if set
  GraphicRotateAnimationType rotateAnimationType;       // Type of rotate animation
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
  GraphicTranslateAnimationType translateAnimationType; // Type of translate animation
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
  TimestampInMicroseconds lifeEnd;                      // Time point at which the object can be removed from the parent container

  // List of texture animations to execute on this object
  std::list<GraphicTextureAnimationParameter> textureAnimationSequence;

  // List of fade animations to execute on this object
  std::list<GraphicFadeAnimationParameter> fadeAnimationSequence;

  // List of scale animations to execute on this object
  std::list<GraphicScaleAnimationParameter> scaleAnimationSequence;

  // List of translate animations to execute on this object
  std::list<GraphicTranslateAnimationParameter> translateAnimationSequence;

  // List of rotate animations to execute on this object
  std::list<GraphicRotateAnimationParameter> rotateAnimationSequence;

  // Sets the next texture animation step from the sequence
  void setNextTextureAnimationStep();

  // Sets the next fade animation step from the sequence
  void setNextFadeAnimationStep();

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

  // Sets a new texture target
  void setTextureAnimation(TimestampInMicroseconds startTime, GraphicTextureInfo startTexture, GraphicTextureInfo endTexture, bool infinite, TimestampInMicroseconds duration);

  // Sets a new fade target
  virtual void setFadeAnimation(TimestampInMicroseconds startTime, GraphicColor startColor, GraphicColor endColor, bool infinite, TimestampInMicroseconds duration);

  // Sets a new rotation target
  void setRotateAnimation(TimestampInMicroseconds startTime, double startAngle, double endAngle, bool infinite, TimestampInMicroseconds duration, GraphicRotateAnimationType animationType);

  // Sets a new scale target
  void setScaleAnimation(TimestampInMicroseconds startTime, double startFactor, double endFactor, bool infinite, TimestampInMicroseconds duration);

  // Sets a new translate target
  void setTranslateAnimation(TimestampInMicroseconds startTime, Int startX, Int startY, Int endX, Int endY, bool infinite, TimestampInMicroseconds duration, GraphicTranslateAnimationType animationType);

  // Let the primitive work
  // For example, animations are handled in this method
  virtual bool work(TimestampInMicroseconds currentTime);

  // Recreates any textures or buffers
  virtual void invalidate();

  // Reduce the number of buffers required
  virtual void optimize();

  // Getters and setters
  void setTextureAnimationSequence(std::list<GraphicTextureAnimationParameter> fadeAnimationSequence);

  void setFadeAnimationSequence(std::list<GraphicFadeAnimationParameter> fadeAnimationSequence);

  void setScaleAnimationSequence(std::list<GraphicScaleAnimationParameter> scaleAnimationSequence);

  void setTranslateAnimationSequence(std::list<GraphicTranslateAnimationParameter> scaleAnimationSequence);

  void setRotateAnimationSequence(std::list<GraphicRotateAnimationParameter> rotateAnimationSequence);

  std::list<GraphicTextureAnimationParameter> getTextureAnimationSequence() const {
    return textureAnimationSequence;
  }

  std::list<GraphicFadeAnimationParameter> getFadeAnimationSequence() const {
    return fadeAnimationSequence;
  }

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

  TimestampInMicroseconds getLifeEnd() const {
    return lifeEnd;
  }

  void setLifeEnd(TimestampInMicroseconds lifeEnd) {
    this->lifeEnd = lifeEnd;
  }

  Int getTranslateEndX() const {
    return translateEndX;
  }

  void setTranslateEndX(Int translateEndX) {
    this->translateEndX = translateEndX;
  }

  Int getTranslateEndY() const {
    return translateEndY;
  }

  void setTranslateEndY(Int translateEndY) {
    this->translateEndY = translateEndY;
  }

  Int getTranslateStartX() const {
    return translateStartX;
  }

  void setTranslateStartX(Int translateStartX) {
    this->translateStartX = translateStartX;
  }

  Int getTranslateStartY() const {
    return translateStartY;
  }

  void setTranslateStartY(Int translateStartY) {
    this->translateStartY = translateStartY;
  }

  TimestampInMicroseconds getTranslateEndTime() const {
    return translateEndTime;
  }

  void setTranslateEndTime(TimestampInMicroseconds translateEndTime) {
    this->translateEndTime = translateEndTime;
  }

  TimestampInMicroseconds getTranslateStartTime() const {
    return translateStartTime;
  }

  void setTranslateStartTime(TimestampInMicroseconds translateStartTime) {
    this->translateStartTime = translateStartTime;
  }
};

}

#endif /* GRAPHICPRIMITIVE_H_ */
