/*
 * DJI Onboard SDK Advanced Sensing APIs
 *
 * Copyright (c) 2017-2018 DJI. All rights reserved.
 *
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI’s authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 *
 * @file dji_camera_stream_link.cpp
 *  @version 3.5
 *  @date Dec 2017
 *
 */

#include "dji_camera_stream_link.hpp"
#include "dji_log.hpp"
#include "dji_linker.hpp"

DJICameraStreamLink::DJICameraStreamLink(CameraType c)
  : camType(c),
    threadStatus(-1),
    isRunning(false),
    cb(NULL),
    cbParam(NULL)
{
  switch (c) {
    case FPV_CAMERA:
      camNameStr = std::string("FPV_CAMERA");
      break;
    case MAIN_CAMERA:
      camNameStr = std::string("Main_CAMERA");
      break;
    case VICE_CAMERA:
      camNameStr = std::string("Vice_CAMERA");
      break;
    case TOP_CAMERA:
      camNameStr = std::string("Top_CAMERA");
      break;

  }
}

DJICameraStreamLink::~DJICameraStreamLink()
{
  cleanup();
}

//@note should be override
bool DJICameraStreamLink::init()
{
  return true;
}

//@note should be override
void DJICameraStreamLink::unInit()
{
}

bool DJICameraStreamLink::start()
{
  if (isRunning)
  {
    DSTATUS_PRIVATE("Already reading data from %s\n", camNameStr.c_str());
    return false;
  }

  threadStatus = pthread_create(&readThread, NULL, DJICameraStreamLink::readThreadEntry, this);
  if (threadStatus != 0)
  {
    DERROR_PRIVATE("Error creating camera reading thread for %s\n", camNameStr.c_str());
    DERROR_PRIVATE("pthread_create returns %d\n", threadStatus);
    return false;
  }
  else
  {
    isRunning = true;
    return true;
  }
}

//@note should be override
void DJICameraStreamLink::stop()
{
  isRunning = false;
  if(0 == threadStatus)
  {
    pthread_join(readThread, NULL);
    threadStatus = -1;
  }
}

void DJICameraStreamLink::cleanup()
{
  stop();
  unInit();
}

void* DJICameraStreamLink::readThreadEntry(void * c)
{
  (reinterpret_cast<DJICameraStreamLink*>(c))->readThreadFunc();
  return NULL;
}

//@note should be override
void DJICameraStreamLink::readThreadFunc()
{
}

void DJICameraStreamLink::registerCallback(CAMCALLBACK f, void* param)
{
  cb = f;
  cbParam = param;
}

bool DJICameraStreamLink::isThreadRunning()
{
  return isRunning;
}
