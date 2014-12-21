//============================================================================
// Name        : ConfigStorePlatform.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef CONFIGSTORE_PLATFORM_H_
#define CONFIGSTORE_PLATFORM_H_

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

namespace GEODISCOVERER {

typedef xmlDocPtr XMLDocument;
typedef xmlNodePtr XMLNode;
typedef xmlAttrPtr XMLAttribute;
typedef xmlNsPtr XMLNamespace;
typedef xmlXPathContextPtr XMLXPathContext;

}

#endif /* CONFIGSTORE_PLATFORM_H_ */
