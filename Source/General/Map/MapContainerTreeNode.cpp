//============================================================================
// Name        : MapContainerTreeNode.cpp
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
MapContainerTreeNode::MapContainerTreeNode(bool doNotDelete) {
  this->doNotDelete=doNotDelete;
  contents=NULL;
  leftChild=NULL;
  rightChild=NULL;
}

// Destructor
MapContainerTreeNode::~MapContainerTreeNode() {
  if (leftChild)
    MapContainerTreeNode::destruct(leftChild);
  if (rightChild)
    MapContainerTreeNode::destruct(rightChild);
}

// Destructs the objects correctly (i.e., if memory has not been allocated by new)
void MapContainerTreeNode::destruct(MapContainerTreeNode *object) {
  if (object->doNotDelete) {
    object->~MapContainerTreeNode();
  } else {
    delete object;
  }
}

}