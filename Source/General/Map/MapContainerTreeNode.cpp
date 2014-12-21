//============================================================================
// Name        : MapContainerTreeNode.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
MapContainerTreeNode::MapContainerTreeNode() {
  contents=NULL;
  leftChild=NULL;
  rightChild=NULL;
}

// Destructor
MapContainerTreeNode::~MapContainerTreeNode() {
  if (leftChild)
    delete leftChild;
  if (rightChild)
    delete rightChild;
}

}
