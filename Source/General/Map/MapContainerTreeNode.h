//============================================================================
// Name        : MapContainerTreeNode.h
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


#ifndef MAPCONTAINERTREE_H_
#define MAPCONTAINERTREE_H_

namespace GEODISCOVERER {

class MapContainerTreeNode {

protected:

  // Indicates if the object has been alloacted by an own memory handler
  bool doNotDelete;

  // Current node
  MapContainer *contents;

  // Left child
  MapContainerTreeNode *leftChild;

  // Right child
  MapContainerTreeNode *rightChild;

public:

  // Constructor
  MapContainerTreeNode(bool doNotDelete=false);

  // Destructor
  virtual ~MapContainerTreeNode();

  // Destructs the objects correctly (i.e., if memory has not been allocated by new)
  static void destruct(MapContainerTreeNode *object);

  // Setters and getters
  MapContainerTreeNode *getLeftChild() const
  {
      return leftChild;
  }

  MapContainer *getContents() const
  {
      return contents;
  }

  MapContainerTreeNode *getRightChild() const
  {
      return rightChild;
  }

  void setLeftChild(MapContainerTreeNode *leftChild)
  {
      this->leftChild = leftChild;
  }

  void setContents(MapContainer *contents)
  {
      this->contents = contents;
  }

  void setRightChild(MapContainerTreeNode *rightChild)
  {
      this->rightChild = rightChild;
  }

};

}

#endif /* MAPCONTAINERTREE_H_ */
