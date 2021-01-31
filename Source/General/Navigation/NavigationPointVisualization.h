//============================================================================
// Name        : NavigationPointVisualization.h
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
//============================================================================

#include <GraphicObject.h>
#include <MapPosition.h>
#include <MapArea.h>

#ifndef NAVIGATIONPOINTVISUALIZATION_H_
#define NAVIGATIONPOINTVISUALIZATION_H_

namespace GEODISCOVERER {

typedef enum {NavigationPointVisualizationTypeUnknown=0, NavigationPointVisualizationTypePoint=1, NavigationPointVisualizationTypeStartFlag=2, NavigationPointVisualizationTypeEndFlag=3 } NavigationPointVisualizationType;

class NavigationPointVisualization {

  // Visualization object of this
  GraphicObject *visualizationObject;

  // Position of the point
  MapPosition pos;

  // Type of visulization to use
  NavigationPointVisualizationType visualizationType;

  // Key of the graphic primitive that represents this point
  GraphicPrimitiveKey graphicPrimitiveKey;

  // Name of the point
  std::string name;

  // Reference to the object this visualization point belongs to
  void *reference;

  // Time the point was created
  TimestampInMicroseconds createTime;

  // Duration in microseconds that the point animation shall last
  TimestampInMicroseconds animationDuration;

  // Code shared by the constructors
  void init(GraphicObject *visualizationObject, TimestampInMicroseconds createTime);

public:

  // Constructor
  NavigationPointVisualization(GraphicObject *visualizationObject);

  // Constructor
  NavigationPointVisualization(GraphicObject *visualizationObject, double lat, double lng, NavigationPointVisualizationType visualizationType, std::string name, void *reference);

  // Destructor
  virtual ~NavigationPointVisualization();

  // Updates the visualization
  void updateVisualization(TimestampInMicroseconds t, MapPosition mapPos, MapArea displayArea);

  // Indicates that textures and buffers have been cleared
  void destroyGraphic();

  // Indicates that textures and buffers shall be created
  void createGraphic();

  // Stores the content into a file
  void store(std::ofstream *ofs);

  // Recreates the content from a binary file
  void retrieve(char *&data, Int &size);

  // Getters and setters
  const NavigationPointVisualizationType getVisualizationType() const {
    return visualizationType;
  }
  const GraphicPrimitiveKey getGraphicPrimitiveKey() const {
    return graphicPrimitiveKey;
  }
  const void *getReference() const {
    return reference;
  }
  MapPosition getPos() const {
    return pos;
  }

};

} /* namespace GEODISCOVERER */

#endif /* NAVIGATIONPOINTVISUALIZATION_H_ */
