//============================================================================
// Name        : Storage.cpp
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
//
//============================================================================

#include <Core.h>
#include <openssl/md5.h>
#include <Storage.h>

namespace GEODISCOVERER {

// Computes a hash of a file
std::string Storage::computeMD5(std::string filepath) {
  unsigned char c[MD5_DIGEST_LENGTH];
  int i;
  MD5_CTX mdContext;
  int bytes;
  unsigned char data[1024];

  // Open the file
  FILE *inFile = fopen(filepath.c_str(), "rb");
  if (!inFile) {
    DEBUG("can not open <%s> for reading",filepath.c_str());
    return "";
  }

  // Compute the checksum
  MD5_Init (&mdContext);
  while ((bytes = fread (data, 1, 1024, inFile)) != 0) {
      MD5_Update (&mdContext, data, bytes);
      std::stringstream s;
      for (int i=0;i<bytes;i++) {
        s << std::setfill('0') << std::setw(2) << std::hex << (int)data[i] << " ";
      }
  }
  MD5_Final (c,&mdContext);
  fclose (inFile);

  // Store the result
  std::stringstream result;
  for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
    result << std::setfill('0') << std::setw(2) << std::hex << (Int)c[i];
  }
  return result.str();
}

}
