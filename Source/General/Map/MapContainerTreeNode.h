//============================================================================
// Name        : MapContainerTreeNode.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef MAPCONTAINERTREE_H_
#define MAPCONTAINERTREE_H_

namespace GEODISCOVERER {

class MapContainerTreeNode {

protected:

  // Current node
  MapContainer *contents;

  // Left child
  MapContainerTreeNode *leftChild;

  // Right child
  MapContainerTreeNode *rightChild;

public:

  // Constructor
  MapContainerTreeNode();

  // Destructor
  virtual ~MapContainerTreeNode();

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
