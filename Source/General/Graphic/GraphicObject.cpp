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
  type=GraphicTypeObject;
  nextPrimitiveKey=1;
  isUpdated=false;
  this->deletePrimitivesOnDestruct=deletePrimitivesOnDestruct;
  primitiveMap.clear();

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
      zStartIteratorMap[primitive->getZ()]=drawList.insert(i,primitive);
      break;
    }
  }
  if (!inserted) {
    drawList.push_back(primitive);
    std::list<GraphicPrimitive*>::iterator i=drawList.end();
    --i;
    zStartIteratorMap[primitive->getZ()]=i;
  }
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
    GraphicPrimitive *primitive = i->second;
    GraphicZMap::iterator j=zStartIteratorMap.find(primitive->getZ());
    if (j!=zStartIteratorMap.end()) {
      std::list<GraphicPrimitive*>::iterator k=j->second;
      if (*k==primitive) {
        k++;
        if ((k==drawList.end())||((*k)->getZ()!=primitive->getZ()))
          zStartIteratorMap.erase(j);
        else
          zStartIteratorMap[(*k)->getZ()]=k;
      }
    }
    drawList.remove(i->second);
    primitiveMap.erase(key);
    isUpdated=true;
    if (deletePrimitive)
      delete primitive;
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

  std::list<GraphicPrimitive*> toBeRemovedPrimitives;

  // Let this object work
  GraphicPrimitive::work(currentTime);

  // Let the primitives work
  bool redrawScene=false;
  for(std::list<GraphicPrimitive*>::iterator i=drawList.begin();i!=drawList.end();i++) {
    GraphicPrimitive *p=*i;
    if ((p->getLifeEnd()!=0)&&(currentTime>p->getLifeEnd())) {
      toBeRemovedPrimitives.push_back(p);
    } else {
      GraphicObject *o=NULL;
      if (p->getType()==GraphicTypeObject) {
        o=(GraphicObject*)p;
      }
      if (p->work(currentTime)) {
        //DEBUG("requesting scene redraw due to animation",NULL);
        redrawScene=true;
      }
    }
  }

  // Remove any primitives whose life end has been reached
  for(std::list<GraphicPrimitive*>::iterator i=toBeRemovedPrimitives.begin();i!=toBeRemovedPrimitives.end();i++) {
    for(GraphicPrimitiveMap::iterator j=primitiveMap.begin(); j!=primitiveMap.end(); j++) {
      GraphicPrimitiveKey key;
      GraphicPrimitive *primitive;
      key=j->first;
      primitive=j->second;
      if (primitive==*i) {
        removePrimitive(key,deletePrimitivesOnDestruct);
        break;
      }
    }
  }

  return redrawScene;
}

// Destructor
GraphicObject::~GraphicObject() {
  deinit(deletePrimitivesOnDestruct);
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
void GraphicObject::recreateGraphic() {
  GraphicPrimitiveMap *primitiveMap=getPrimitiveMap();
  GraphicPrimitiveMap::iterator i;
  for(i = primitiveMap->begin(); i!=primitiveMap->end(); i++) {
    i->second->invalidate();
  }
}

}
