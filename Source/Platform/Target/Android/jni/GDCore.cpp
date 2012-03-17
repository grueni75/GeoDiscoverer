//============================================================================
// Name        : Main.cpp
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

#include <jni.h>
#include <Core.h>
#include <android/log.h>

// Prototypes
void GDApp_executeAppCommand(std::string command);

#ifdef __cplusplus
extern "C" {
#endif

// Reference to the virtual machine
JavaVM *virtualMachine=NULL;

// References for callbacks
jobject coreObject=NULL;
jclass coreClass=NULL;
jmethodID executeAppCommandMethodID=NULL;

// Called when the library is loaded
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
  JNIEnv *env;
  virtualMachine = vm;
  __android_log_write(ANDROID_LOG_INFO,"GDCore","dynamic library intialized.");
  return JNI_VERSION_1_6;
}

// Gets the environment pointer
JNIEnv *GDApp_obtainJNIEnv(bool &isAttached) {
  int status;
  JNIEnv *env;
  isAttached=false;
  status = virtualMachine->GetEnv((void **) &env, JNI_VERSION_1_6);
  if (status < 0) {
    status = virtualMachine->AttachCurrentThread(&env, NULL);
    if(status < 0) {
      __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not attach current thread to virtual machine!");
      exit(1);
    }
    isAttached = true;
  }
  return env;
}

// Frees the environment pointer
void GDApp_releaseJNIEnv(JNIEnv *env, bool isAttached) {

  // Detach if required
  if (isAttached)
    virtualMachine->DetachCurrentThread();

}

// Returns the required IDs to call a static java method
jmethodID GDApp_findJavaMethod(JNIEnv *env, std::string methodName, std::string methodSignature)
{
  // Get the GDCore java class
  if (!coreObject) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","Java GDCore object not set!");
    exit(1);
  }
  if(!coreClass) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","Java GDCore class not set!");
    exit(1);
  }

  // Get the method
  jmethodID method;
  method = env->GetMethodID(coreClass, methodName.c_str(), methodSignature.c_str());
  if(!method) {
    std::string msg="can not find static method <" + methodName + ">!";
    __android_log_write(ANDROID_LOG_FATAL,"GDCore",msg.c_str());
    exit(1);
  }
  return method;
}

// Deinits the core
JNIEXPORT void JNICALL Java_com_perfectapp_android_geodiscoverer_GDCore_deinit
  (JNIEnv *env, jobject thiz)
{
  // Clean up
  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","deleting core.");
  if (GEODISCOVERER::core)
    delete GEODISCOVERER::core;
  GEODISCOVERER::core=NULL;

  // Do not use the object anymore
  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","deleting coreObject global reference.");
  if (coreObject) {
    env->DeleteGlobalRef(coreObject);
  }
  coreObject=NULL;
  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","deleting coreClass global reference.");
  if (coreClass) {
    env->DeleteGlobalRef(coreClass);
  }
  coreClass=NULL;
}

// Inits the core
JNIEXPORT void JNICALL Java_com_perfectapp_android_geodiscoverer_GDCore_init
  (JNIEnv *env, jobject thiz, jstring homePath, jint screenDPI)
{
  std::stringstream out;

  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","init called.");

  // Remember the object
  coreObject=env->NewGlobalRef(thiz);
  if (!coreObject) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain global reference!");
    Java_com_perfectapp_android_geodiscoverer_GDCore_deinit(env,thiz);
    exit(1);
  }

  // Remember the class
  coreClass = (jclass)env->NewGlobalRef(env->GetObjectClass(thiz));
  if(!coreClass) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find GDCore class!");
    exit(1);
  }

  // Cache all required callback methods
  executeAppCommandMethodID=GDApp_findJavaMethod(env,"executeAppCommand","(Ljava/lang/String;)V");

  // Get the home path
  const char *homePathCStr = env->GetStringUTFChars(homePath, NULL);
  if (!homePathCStr) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain home path!");
    Java_com_perfectapp_android_geodiscoverer_GDCore_deinit(env,thiz);
    exit(1);
  }

  // Create the application
  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","before object creation.");
  if (!(GEODISCOVERER::core=new GEODISCOVERER::Core(homePathCStr,screenDPI))) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create core object!");
    Java_com_perfectapp_android_geodiscoverer_GDCore_deinit(env,thiz);
    exit(1);
  }
  __android_log_write(ANDROID_LOG_DEBUG,"GDCore",homePathCStr);
  env->ReleaseStringUTFChars(homePath,homePathCStr);

  // Init the application
  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","before object initialization.");
  if (!(GEODISCOVERER::core->init())) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not initialize core object!");
  }

  // Get the last known location
  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","before get last known location.");
  GDApp_executeAppCommand("getLastKnownLocation()");

  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","init finished.");
}

// Updates the screen contents
JNIEXPORT void JNICALL Java_com_perfectapp_android_geodiscoverer_GDCore_updateScreen
  (JNIEnv *env, jobject thiz, jboolean forceRedraw)
{
  if (GEODISCOVERER::core->getDebug()->getFatalOccured())
    return;
  GEODISCOVERER::core->updateScreen(forceRedraw);
}

// Creates the screen context
JNIEXPORT jboolean JNICALL Java_com_perfectapp_android_geodiscoverer_GDCore_createScreen
  (JNIEnv *env, jobject thiz)
{
  // Inform the core
  if (!GEODISCOVERER::core->graphicInvalidated())
    return false;

  // That's it
  return true;
}

// Sends a command to the core
JNIEXPORT jstring JNICALL Java_com_perfectapp_android_geodiscoverer_GDCore_executeCoreCommandInt
  (JNIEnv *env, jobject thiz, jstring cmd)
{
  const char *cmdCStr = env->GetStringUTFChars(cmd, NULL);
  if (!cmdCStr) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain cmd!");
    Java_com_perfectapp_android_geodiscoverer_GDCore_deinit(env,thiz);
    exit(1);
  }
  std::string result=GEODISCOVERER::core->getCommander()->execute(cmdCStr);
  env->ReleaseStringUTFChars(cmd,cmdCStr);
  return env->NewStringUTF(result.c_str());
}

#ifdef __cplusplus
}
#endif

// Executes a command on the java side
void GDApp_executeAppCommand(std::string command)
{
  JNIEnv *env;
  bool isAttached = false;

  // Get the environment pointer
  env=GDApp_obtainJNIEnv(isAttached);

  // Construct the java string
  jstring commandJavaString = env->NewStringUTF(command.c_str());
  if (!commandJavaString) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create java string!");
    exit(1);
  }

  // Call the method
  env->CallVoidMethod(coreObject, executeAppCommandMethodID, commandJavaString);

  // Release the env
  env->DeleteLocalRef(commandJavaString);
  GDApp_releaseJNIEnv(env,isAttached);
}
