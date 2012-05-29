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
void GDApp_addMessage(int severity, std::string tag, std::string message);

#ifdef __cplusplus
extern "C" {
#endif

// Reference to the virtual machine
JavaVM *virtualMachine=NULL;

// References for callbacks
jobject coreObject=NULL;
jclass appClass=NULL;
jclass coreClass=NULL;
jclass bundleClass=NULL;
jclass listClass=NULL;
jmethodID executeAppCommandMethodID=NULL;
jmethodID bundleConstructorMethodID=NULL;
jmethodID bundlePutStringMethodID=NULL;
jmethodID listAddMethodID=NULL;
jmethodID addMessageMethodID=NULL;
jmethodID setThreadPriorityMethodID=NULL;

// Called when the library is loaded
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
  JNIEnv *env;
  virtualMachine = vm;
  __android_log_write(ANDROID_LOG_INFO,"GDCore","dynamic library initialized.");
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
    std::string msg="can not find method <" + methodName + ">!";
    __android_log_write(ANDROID_LOG_FATAL,"GDCore",msg.c_str());
    exit(1);
  }
  return method;
}

// Deinits the jni
JNIEXPORT void JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI
  (JNIEnv *env, jobject thiz)
{
  // Do not use the object anymore
  //__android_log_write(ANDROID_LOG_DEBUG,"GDCore","deleting coreObject global reference.");
  if (coreObject) {
    env->DeleteGlobalRef(coreObject);
  }
  coreObject=NULL;
  //__android_log_write(ANDROID_LOG_DEBUG,"GDCore","deleting coreClass global reference.");
  if (coreClass) {
    env->DeleteGlobalRef(coreClass);
  }
  coreClass=NULL;
}

// Inits the jni
JNIEXPORT void JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_initJNI
  (JNIEnv *env, jobject thiz)
{
  // Remember the object
  coreObject=env->NewGlobalRef(thiz);
  if (!coreObject) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain global reference!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }

  // Cache all required classes
  coreClass = (jclass)env->NewGlobalRef(env->GetObjectClass(thiz));
  if(!coreClass) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find GDCore class!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }
  bundleClass = env->FindClass("android/os/Bundle");
  if (!bundleClass) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find Bundle class!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }
  listClass = env->FindClass("java/util/List");
  if (!bundleClass) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find List class!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }
  appClass = env->FindClass("com/untouchableapps/android/geodiscoverer/GDApplication");
  if (!appClass) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find GDApplication class!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }

  // Cache all required callback methods
  executeAppCommandMethodID=GDApp_findJavaMethod(env,"executeAppCommand","(Ljava/lang/String;)V");
  if (!executeAppCommandMethodID) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find executeAppCommand method!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }
  setThreadPriorityMethodID=GDApp_findJavaMethod(env,"setThreadPriority","(I)V");
  if (!setThreadPriorityMethodID) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find setThreadPriorityMethodID method!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }
  bundleConstructorMethodID = env->GetMethodID(bundleClass, "<init>","()V");
  if (!bundleConstructorMethodID) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find constructor method of Bundle class!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }
  bundlePutStringMethodID = env->GetMethodID(bundleClass, "putString","(Ljava/lang/String;Ljava/lang/String;)V");
  if (!bundlePutStringMethodID) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find putString method of Bundle class!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }
  listAddMethodID = env->GetMethodID(listClass, "add","(Ljava/lang/Object;)Z");
  if (!bundlePutStringMethodID) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find add method of List class!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }
  addMessageMethodID = env->GetStaticMethodID(appClass, "addMessage","(ILjava/lang/String;Ljava/lang/String;)V");
  if (!addMessageMethodID) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not find addMessage method of GDApplication class!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitJNI(env,thiz);
    exit(1);
  }
}

// Deinits the core
JNIEXPORT void JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore
  (JNIEnv *env, jobject thiz)
{
  // Clean up
  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","deleting core.");
  if (GEODISCOVERER::core)
    delete GEODISCOVERER::core;
  GEODISCOVERER::core=NULL;
}

// Inits the core
JNIEXPORT void JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_initCore
  (JNIEnv *env, jobject thiz, jstring homePath, jint screenDPI)
{
  std::stringstream out;

  __android_log_write(ANDROID_LOG_DEBUG,"GDCore","creating core.");

  // Get the home path
  const char *homePathCStr = env->GetStringUTFChars(homePath, NULL);
  if (!homePathCStr) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain home path!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }

  // Create the application
  //__android_log_write(ANDROID_LOG_DEBUG,"GDCore","before object creation.");
  if (!(GEODISCOVERER::core=new GEODISCOVERER::Core(homePathCStr,screenDPI))) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create core object!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  //__android_log_write(ANDROID_LOG_DEBUG,"GDCore",homePathCStr);
  env->ReleaseStringUTFChars(homePath,homePathCStr);

  // Init the application
  //__android_log_write(ANDROID_LOG_DEBUG,"GDCore","before object initialization.");
  if (!(GEODISCOVERER::core->init())) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not initialize core object!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }

  // Get the last known location
  //__android_log_write(ANDROID_LOG_DEBUG,"GDCore","before get last known location.");
  GDApp_executeAppCommand("getLastKnownLocation()");

  //__android_log_write(ANDROID_LOG_DEBUG,"GDCore","init finished.");
}

// Updates the screen contents
JNIEXPORT void JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_updateScreen
  (JNIEnv *env, jobject thiz, jboolean forceRedraw)
{
  if (GEODISCOVERER::core->getDebug()->getFatalOccured())
    return;
  GEODISCOVERER::core->updateScreen(forceRedraw);
}

// Sends a command to the core
JNIEXPORT jstring JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_executeCoreCommandInt
  (JNIEnv *env, jobject thiz, jstring cmd)
{
  const char *cmdCStr = env->GetStringUTFChars(cmd, NULL);
  if (!cmdCStr) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain cmd!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  std::string result=GEODISCOVERER::core->getCommander()->execute(cmdCStr);
  env->ReleaseStringUTFChars(cmd,cmdCStr);
  return env->NewStringUTF(result.c_str());
}

// Gets a string value from the config
JNIEXPORT void JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_configStoreSetStringValue
  (JNIEnv *env, jobject thiz, jstring path, jstring name, jstring value)
{
  const char *pathCStr = env->GetStringUTFChars(path, NULL);
  const char *nameCStr = env->GetStringUTFChars(name, NULL);
  const char *valueCStr = env->GetStringUTFChars(value, NULL);
  if ((!pathCStr)||(!nameCStr)||(!valueCStr)) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain strings!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  GEODISCOVERER::core->getConfigStore()->setStringValue(pathCStr,nameCStr,valueCStr);
  env->ReleaseStringUTFChars(path,pathCStr);
  env->ReleaseStringUTFChars(name,nameCStr);
  env->ReleaseStringUTFChars(value,valueCStr);
}

// Gets a string value from the config
JNIEXPORT jstring JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_configStoreGetStringValue
  (JNIEnv *env, jobject thiz, jstring path, jstring name)
{
  const char *pathCStr = env->GetStringUTFChars(path, NULL);
  const char *nameCStr = env->GetStringUTFChars(name, NULL);
  if ((!pathCStr)||(!nameCStr)) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain strings!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  std::string value=GEODISCOVERER::core->getConfigStore()->getStringValue(pathCStr,nameCStr);
  env->ReleaseStringUTFChars(path,pathCStr);
  env->ReleaseStringUTFChars(name,nameCStr);
  return env->NewStringUTF(value.c_str());
}

// Lists all elements for the given path in the config
JNIEXPORT jobject JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_configStoreGetNodeNames
  (JNIEnv *env, jobject thiz, jstring path)
{
  // Call the config method
  const char *pathCStr = env->GetStringUTFChars(path, NULL);
  if ((!pathCStr)) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain strings!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  std::list<std::string> names=GEODISCOVERER::core->getConfigStore()->getNodeNames(pathCStr);
  env->ReleaseStringUTFChars(path,pathCStr);

  // Create the resulting object array
  jobjectArray returnObj = env->NewObjectArray(names.size(),env->FindClass("java/lang/String"),env->NewStringUTF(""));
  if (!returnObj) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create return object array!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  int i=0;
  for(std::list<std::string>::iterator it=names.begin();it!=names.end();it++) {

    // Create the strings
    jstring nameString = env->NewStringUTF((*it).c_str());
    if (!nameString) {
      __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create java string!");
      exit(1);
    }

    // Add it to the array
    env->SetObjectArrayElement(returnObj,i,nameString);

    // Release the strings
    env->DeleteLocalRef(nameString);
    i++;
  }
  return returnObj;
}

// Returns information about the given node in the config
JNIEXPORT jobject JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_configStoreGetNodeInfo
  (JNIEnv *env, jobject thiz, jstring path)
{
  // Call the config method
  const char *pathCStr = env->GetStringUTFChars(path, NULL);
  if ((!pathCStr)) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain strings!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  std::map<std::string, std::string> nodeInfo=GEODISCOVERER::core->getConfigStore()->getNodeInfo(pathCStr);
  env->ReleaseStringUTFChars(path,pathCStr);

  // Create the resulting bundle object
  jobject returnObj = env->NewObject(bundleClass, bundleConstructorMethodID);
  if ((!returnObj)) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create return object!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  for(std::map<std::string, std::string>::iterator i=nodeInfo.begin();i!=nodeInfo.end();i++) {

    // Create the strings
    jstring keyString = env->NewStringUTF(i->first.c_str());
    jstring valueString = env->NewStringUTF(i->second.c_str());
    if ((!keyString)||(!valueString)) {
      __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create java string!");
      exit(1);
    }

    // Call the method
    env->CallVoidMethod(returnObj, bundlePutStringMethodID, keyString, valueString);

    // Release the strings
    env->DeleteLocalRef(keyString);
    env->DeleteLocalRef(valueString);
  }
  return returnObj;
}

// Lists all values for the given attribute in the config
JNIEXPORT jobject JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_configStoreGetAttributeValues
  (JNIEnv *env, jobject thiz, jstring path, jstring attributeName)
{
  // Call the config method
  const char *pathCStr = env->GetStringUTFChars(path, NULL);
  const char *attributeNameCStr = env->GetStringUTFChars(attributeName, NULL);
  if ((!pathCStr)||(!attributeNameCStr)) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain strings!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  std::list<std::string> names=GEODISCOVERER::core->getConfigStore()->getAttributeValues(pathCStr,attributeNameCStr);
  env->ReleaseStringUTFChars(path,pathCStr);
  env->ReleaseStringUTFChars(path,attributeNameCStr);

  // Create the resulting object array
  jobjectArray returnObj = env->NewObjectArray(names.size(),env->FindClass("java/lang/String"),env->NewStringUTF(""));
  if (!returnObj) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create return object array!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  int i=0;
  for(std::list<std::string>::iterator it=names.begin();it!=names.end();it++) {

    // Create the strings
    jstring valueString = env->NewStringUTF((*it).c_str());
    if (!valueString) {
      __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create java string!");
      exit(1);
    }

    // Add it to the array
    env->SetObjectArrayElement(returnObj,i,valueString);

    // Release the strings
    env->DeleteLocalRef(valueString);
    i++;
  }
  return returnObj;
}

// Checks if the path exists in the config
JNIEXPORT jboolean JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_configStorePathExists
  (JNIEnv *env, jobject thiz, jstring path)
{
  const char *pathCStr = env->GetStringUTFChars(path, NULL);
  if ((!pathCStr)) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain strings!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  bool result=GEODISCOVERER::core->getConfigStore()->pathExists(pathCStr);
  env->ReleaseStringUTFChars(path,pathCStr);
  return result;
}

// Removes the path from the config
JNIEXPORT void JNICALL Java_com_untouchableapps_android_geodiscoverer_GDCore_configStoreRemovePath
  (JNIEnv *env, jobject thiz, jstring path)
{
  const char *pathCStr = env->GetStringUTFChars(path, NULL);
  if ((!pathCStr)) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not obtain strings!");
    Java_com_untouchableapps_android_geodiscoverer_GDCore_deinitCore(env,thiz);
    exit(1);
  }
  GEODISCOVERER::core->getConfigStore()->removePath(pathCStr);
  env->ReleaseStringUTFChars(path,pathCStr);
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

// Sets the thread priority
void GDApp_setThreadPriority(int priority)
{
  JNIEnv *env;
  bool isAttached = false;

  // Get the environment pointer
  env=GDApp_obtainJNIEnv(isAttached);

  // Call the method
  env->CallVoidMethod(coreObject, setThreadPriorityMethodID, priority);

  // Release the env
  GDApp_releaseJNIEnv(env,isAttached);
}

// Adds a message to the application
void GDApp_addMessage(int severity, std::string tag, std::string message)
{
  JNIEnv *env;
  bool isAttached = false;

  // Get the environment pointer
  env=GDApp_obtainJNIEnv(isAttached);

  // Construct the java string
  jstring tagJavaString = env->NewStringUTF(tag.c_str());
  if (!tagJavaString) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create java string!");
    exit(1);
  }
  jstring messageJavaString = env->NewStringUTF(message.c_str());
  if (!messageJavaString) {
    __android_log_write(ANDROID_LOG_FATAL,"GDCore","can not create java string!");
    exit(1);
  }

  // Call the method
  env->CallStaticVoidMethod(appClass, addMessageMethodID, severity, tagJavaString, messageJavaString);

  // Release the env
  env->DeleteLocalRef(tagJavaString);
  env->DeleteLocalRef(messageJavaString);
  GDApp_releaseJNIEnv(env,isAttached);
}
