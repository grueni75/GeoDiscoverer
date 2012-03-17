//============================================================================
// Name        : GraphicObject.cpp
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


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
GraphicObject::GraphicObject(bool deletePrimitivesOnDestruct) : GraphicPrimitive() {

  // Init variables
  type=GraphicObjectType;
  nextPrimitiveKey=1;
  isUpdated=false;
  this->deletePrimitivesOnDestruct=deletePrimitivesOnDestruct;
  primitiveMap.clear();
  accessMutex=core->getThread()->createMutex();

}

// Adds a primitive
GraphicPrimitiveKey GraphicObject::addPrimitive(GraphicPrimitive *primitive)
{
  GraphicPrimitiveKey currentPrimitiveKey=nextPrimitiveKey;
  GraphicPrimitivePair p=GraphicPrimitivePair(currentPrimitiveKey,primitive);
  primitiveMap.insert(p);
  nextPrimitiveKey++;
  if (nextPrimitiveKey==0)
    nextPrimitiveKey=1;
  bool inserted=false;
  for(std::list<GraphicPrimitive*>::iterator i=drawList.begin();i!=drawList.end();i++) {
    GraphicPrimitive *p=*i;
    if (!GraphicPrimitive::zSortPredicate(primitive,p)) {
      inserted=true;
      drawList.insert(i,primitive);
      break;
    }
  }
  if (!inserted)
    drawList.push_back(primitive);
  /*DEBUG("sorted list",NULL);
  for(std::list<GraphicPrimitive*>::iterator i=drawList.begin();i!=drawList.end();i++) {
    GraphicPrimitive *p=*i;
    DEBUG("z=%d",p->getZ());
  }*/
  isUpdated=true;
  return currentPrimitiveKey;
}

// Removes a primitive
void GraphicObject::removePrimitive(GraphicPrimitiveKey key, bool deletePrimitive)
{
  GraphicPrimitiveMap::iterator i;
  i=primitiveMap.find(key);
  if (i!=primitiveMap.end()) {
    drawList.remove(i->second);
    primitiveMap.erase(key);
    isUpdated=true;
    if (deletePrimitive)
      delete i->second;
  }
}

// Clears the object
void GraphicObject::deinit(bool deletePrimitives) {

  // Delete all primitives
  GraphicPrimitiveMap *primitiveMap=getPrimitiveMap();
  GraphicPrimitiveMap::iterator i;
  for(i = primitiveMap->begin(); i!=primitiveMap->end(); i++) {
    GraphicPrimitiveKey key;
    GraphicPrimitive *primitive;
    key=i->first;
    primitive=i->second;
    if (deletePrimitives)
      delete primitive;
  }
  primitiveMap->clear();
  drawList.clear();

}

// Updates all primitives contained in the object
bool GraphicObject::work(TimestampInMicroseconds currentTime) {

  // Let the primitives work
  bool redrawScene=false;
  for(std::list<GraphicPrimitive*>::iterator i=drawList.begin();i!=drawList.end();i++) {
    GraphicPrimitive *p=*i;
    if (p->work(currentTime)) {
      //DEBUG("requesting scene redraw due to animation",NULL);
      redrawScene=true;
    }
  }
  return redrawScene;
}

// Destructor
GraphicObject::~GraphicObject() {
  deinit(deletePrimitivesOnDestruct);
  core->getThread()->destroyMutex(accessMutex);
}

// Returns the primitive with the given key
GraphicPrimitive *GraphicObject::getPrimitive(GraphicPrimitiveKey key) {
  GraphicPrimitiveMap *primitiveMap=getPrimitiveMap();
  GraphicPrimitiveMap::iterator i;
  i=primitiveMap->find(key);
  if (i!=primitiveMap->end())
    return i->second;
  else
    return NULL;
}

// Recreates any textures or buffers
void GraphicObject::invalidate() {
  GraphicPrimitiveMap *primitiveMap=getPrimitiveMap();
  GraphicPrimitiveMap::iterator i;
  for(i = primitiveMap->begin(); i!=primitiveMap->end(); i++) {
    i->second->invalidate();
  }
}

}
