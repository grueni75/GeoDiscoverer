//============================================================================
// Name        : Core.cpp
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

#include <curl/curl.h>
#include <Core.h>

namespace GEODISCOVERER {

struct curlMemoryStruct {
  UByte *memory;
  size_t size;
};

static size_t
curlMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct curlMemoryStruct *mem = (struct curlMemoryStruct *)userp;
  mem->memory = (UByte*)realloc((UByte*)mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    FATAL("not enough memory",NULL);
    return 0;
  }
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  return realsize;
}


// Inits curl
void Core::initCURL() {
  curl_global_init(CURL_GLOBAL_ALL);
}

// Downloads a URL
UByte *Core::downloadURL(std::string url, DownloadResult &result, UInt &size, bool generateMessages, bool ignoreFileNotFoundErrors, std::list<std::string> *httpHeader) {

  char curlErrorBuffer[CURL_ERROR_SIZE];
  result = DownloadResultOtherFail;
  long curlResponseCode = 0;
  struct curlMemoryStruct out;
  struct curl_slist *header = NULL;

  // Reserve initial memory for download
  out.memory = (UByte*)malloc(1);
  out.size = 0;

  // Prepare header
  if (httpHeader) {
    for (std::list<std::string>::iterator i=httpHeader->begin();i!=httpHeader->end();i++) {
      header = curl_slist_append(header, (*i).c_str());
    }
  }

  // Download it with curl
  CURL *curl;
  curl = curl_easy_init();
  if (!curl) {
    FATAL("can not initialize curl library",NULL);
    result=DownloadResultOtherFail;
    return NULL;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Geo Discoverer (build on "  __DATE__  ")");
  if (header) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,curlErrorBuffer);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);

  //curl_easy_setopt(curl, CURLOPT_CAINFO, (core->getHomePath() + "/Misc/ca-certificates.crt").c_str());
  DEBUG("downloading url %s",url.c_str());
  CURLcode curlResult = curl_easy_perform(curl);
  curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE, &curlResponseCode);
  curl_easy_cleanup(curl);
  if (header) curl_slist_free_all(header);

  // Handle errors
  size=0;
  switch(curlResult) {
    case CURLE_REMOTE_FILE_NOT_FOUND:
      result=DownloadResultFileNotFound;
      break;
    case CURLE_OK:
      switch (curlResponseCode) {
        case 200:
          result=DownloadResultSuccess;
          size=out.size;
          break;
        case 404:
          result=DownloadResultFileNotFound;
          break;
        default:
          result=DownloadResultOtherFail;
          break;
      }
      snprintf(curlErrorBuffer,CURL_ERROR_SIZE,"response code = %ld",curlResponseCode);
      break;
    default:
      result=DownloadResultOtherFail;
      break;
  }
  if (result!=DownloadResultSuccess) {

    // Free the downloaded memory
    free(out.memory);
    out.memory=NULL;

    // Output warning
    if (generateMessages) {
      bool outputWarning=true;
      if ((ignoreFileNotFoundErrors)&&(result=DownloadResultFileNotFound))
        outputWarning=false;
      if (outputWarning) {
        WARNING("can not download url <%s>: %s",url.c_str(),curlErrorBuffer);
      } else {
        DEBUG("can not download url <%s>: %s",url.c_str(),curlErrorBuffer);
      }
    }
  }
  return out.memory;
}

}





