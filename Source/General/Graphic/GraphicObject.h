//============================================================================
// Name        : GraphicObject.h
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


#ifndef GRAPHICOBJECT_H_
#define GRAPHICOBJECT_H_

namespace GEODISCOVERER {

// Types
typedef UInt GraphicPrimitiveKey;
typedef std::map<GraphicPrimitiveKey, GraphicPrimitive*> GraphicPrimitiveMap;
typedef std::pair<GraphicPrimitiveKey, GraphicPrimitive*> GraphicPrimitivePair;

class GraphicObject : public GraphicPrimitive {

protected:

  // Holds all primitives to draw
  GraphicPrimitiveMap primitiveMap;

  // A list of all primitives sorted according to their z value
  std::list<GraphicPrimitive*> drawList;

  // Next key to use for a rectangle
  GraphicPrimitiveKey nextPrimitiveKey;

  // Indicates if the contained primitives shall be removed if the object is destructed
  bool deletePrimitivesOnDestruct;

  // Indicates that the object has been changed
  bool isUpdated;

public:

  // Constructor
  GraphicObject(bool deletePrimitivesOnDestruct=false);

  // Destructor
  virtual ~GraphicObject();

  // Adds a primitive to the object
  GraphicPrimitiveKey addPrimitive(GraphicPrimitive *primitive);

  // Removes a primitive from the object
  void removePrimitive(GraphicPrimitiveKey key, bool deletePrimitive=false);

  // Clears the object
  void deinit(bool deletePrimitives=true);

  // Updates all primitives contained in the object
  virtual bool work(TimestampInMicroseconds currentTime);

  // Recreates any textures or buffers
  virtual void recreateGraphic();

  // Getters and setters
  GraphicPrimitiveMap *getPrimitiveMap()
  {
      return &primitiveMap;
  }

  bool getIsUpdated() const
  {
      return isUpdated;
  }

  void setIsUpdated(bool isUpdated)
  {
      this->isUpdated = isUpdated;
  }

  std::list<GraphicPrimitive*> *getDrawList()
  {
      return &drawList;
  }

  GraphicPrimitive *getPrimitive(GraphicPrimitiveKey key);

};

}

#endif /* GRAPHICOBJECT_H_ */
