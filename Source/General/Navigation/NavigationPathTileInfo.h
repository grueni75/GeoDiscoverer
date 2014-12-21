//============================================================================
// Name        : NavigationPathTileInfo.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef NAVIGATIONPATHTILEINFO_H_
#define NAVIGATIONPATHTILEINFO_H_

namespace GEODISCOVERER {

class NavigationPathTileInfo {

  // Info about the graphic line object
  GraphicPrimitiveKey pathLineKey;
  GraphicLine *pathLine;

  // Info about the graphic rectangle list object
  GraphicPrimitiveKey pathArrowListKey;
  GraphicRectangleList *pathArrowList;

  // Info about the start flag
  GraphicPrimitiveKey pathStartFlagKey;
  GraphicRectangle *pathStartFlag;

  // Info about the end flag
  GraphicPrimitiveKey pathEndFlagKey;
  GraphicRectangle *pathEndFlag;

public:

  // Constructor
  NavigationPathTileInfo();

  // Destructor
  virtual ~NavigationPathTileInfo();

  // Getters and setters
  GraphicLine *getPathLine() const
  {
      return pathLine;
  }

  GraphicPrimitiveKey getPathLineKey() const
  {
      return pathLineKey;
  }

  void setPathLine(GraphicLine *pathLine)
  {
      this->pathLine = pathLine;
  }

  void setPathLineKey(GraphicPrimitiveKey pathLineKey)
  {
      this->pathLineKey = pathLineKey;
  }

  GraphicRectangleList *getPathArrowList() const
  {
      return pathArrowList;
  }

  GraphicPrimitiveKey getPathArrowListKey() const
  {
      return pathArrowListKey;
  }

  void setPathArrowList(GraphicRectangleList *pathArrowList)
  {
      this->pathArrowList = pathArrowList;
  }

  void setPathArrowListKey(GraphicPrimitiveKey pathArrowListKey)
  {
      this->pathArrowListKey = pathArrowListKey;
  }

  GraphicRectangle* getPathEndFlag() const {
    return pathEndFlag;
  }

  void setPathEndFlag(GraphicRectangle* pathEndFlag) {
    this->pathEndFlag = pathEndFlag;
  }

  GraphicPrimitiveKey getPathEndFlagKey() const {
    return pathEndFlagKey;
  }

  void setPathEndFlagKey(GraphicPrimitiveKey pathEndFlagKey) {
    this->pathEndFlagKey = pathEndFlagKey;
  }

  GraphicRectangle* getPathStartFlag() const {
    return pathStartFlag;
  }

  void setPathStartFlag(GraphicRectangle* pathStartFlag) {
    this->pathStartFlag = pathStartFlag;
  }

  GraphicPrimitiveKey getPathStartFlagKey() const {
    return pathStartFlagKey;
  }

  void setPathStartFlagKey(GraphicPrimitiveKey pathStartFlagKey) {
    this->pathStartFlagKey = pathStartFlagKey;
  }
};

}

#endif /* NAVIGATIONPATHTILEINFO_H_ */
