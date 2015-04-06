//============================================================================
// Name        : GraphicObject.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef GRAPHICOBJECT_H_
#define GRAPHICOBJECT_H_

namespace GEODISCOVERER {

// Types
typedef UInt GraphicPrimitiveKey;
typedef std::map<GraphicPrimitiveKey, GraphicPrimitive*> GraphicPrimitiveMap;
typedef std::pair<GraphicPrimitiveKey, GraphicPrimitive*> GraphicPrimitivePair;
typedef std::map<Int, std::list<GraphicPrimitive*>::iterator> GraphicZMap;
typedef std::pair<Int, std::list<GraphicPrimitive*>::iterator> GraphicZPair;

class GraphicObject : public GraphicPrimitive {

protected:

  // Holds all primitives to draw
  GraphicPrimitiveMap primitiveMap;

  // A list of all primitives sorted according to their z value
  std::list<GraphicPrimitive*> drawList;

  // A map of iterators for the z value
  GraphicZMap zStartIteratorMap;

  // Next key to use for a rectangle
  GraphicPrimitiveKey nextPrimitiveKey;

  // Indicates if the contained primitives shall be removed if the object is destructed
  bool deletePrimitivesOnDestruct;

  // Indicates that the object has been changed
  bool isUpdated;

public:

  // Constructor
  GraphicObject(Screen *screen, bool deletePrimitivesOnDestruct=false);

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

  // Returns a drawing list with no graphic objects inside
  std::list<GraphicPrimitive*> getFlattenDrawList();

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

  std::list<GraphicPrimitive*>::iterator getFirstElementInDrawList(Int z)
  {
      GraphicZMap::iterator i=zStartIteratorMap.find(z);
      if (i!=zStartIteratorMap.end())
        return i->second;
      else
        return drawList.end();
  }

  GraphicPrimitive *getPrimitive(GraphicPrimitiveKey key);
};

}

#endif /* GRAPHICOBJECT_H_ */
