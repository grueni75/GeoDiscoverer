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
#include <openssl/evp.h>
#include <Storage.h>

namespace GEODISCOVERER {

// Computes a hash of a file
std::string Storage::computeMD5(std::string filepath) {
  unsigned char *md5_digest;
  unsigned int md5_digest_len = EVP_MD_size(EVP_md5());
  int i;
  EVP_MD_CTX *mdContext;
  int bytes;
  unsigned char data[1024];

  // Open the file
  FILE *inFile = fopen(filepath.c_str(), "rb");
  if (!inFile) {
    DEBUG("can not open <%s> for reading",filepath.c_str());
    return "";
  }

  // Compute the checksum
  mdContext = EVP_MD_CTX_new();
  EVP_DigestInit_ex(mdContext, EVP_md5(), NULL);
  while ((bytes = fread (data, 1, 1024, inFile)) != 0) {
      EVP_DigestUpdate(mdContext, data, bytes);
      std::stringstream s;
      for (int i=0;i<bytes;i++) {
        s << std::setfill('0') << std::setw(2) << std::hex << (int)data[i] << " ";
      }
  }
  md5_digest = (unsigned char *)OPENSSL_malloc(md5_digest_len);
  EVP_DigestFinal_ex(mdContext, md5_digest, &md5_digest_len);
  fclose (inFile);

  // Store the result
  std::stringstream result;
  for(i = 0; i < md5_digest_len; i++) {
    result << std::setfill('0') << std::setw(2) << std::hex << (Int)md5_digest[i];
  }
  OPENSSL_free(md5_digest);
  EVP_MD_CTX_free(mdContext);
  return result.str();
}

}
