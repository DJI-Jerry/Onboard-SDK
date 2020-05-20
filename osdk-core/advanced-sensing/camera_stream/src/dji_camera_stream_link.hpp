/** @file dji_camera_stream_link.hpp
 *  @version 3.5
 *  @date Dec 2017
 *
 *  @brief The class to read data from the camera

 *  @copyright 2017 DJI. All rights reserved.
 *
 */

#ifndef DJICAMERASTREAMLINK_HH
#define DJICAMERASTREAMLINK_HH
#include <string>
#include "pthread.h"
#include "dji_camera_image.hpp"

typedef void (*CAMCALLBACK)(void*, uint8_t*, int);
class Linker;
class DJICameraStreamLink
{
public:
  DJICameraStreamLink(CameraType c);
  virtual ~DJICameraStreamLink();
  /* Establish link to camera */
  virtual bool init();

  /* Start the data receiving thread */
  virtual bool start();

  /* Stop the data receiving thread */
  virtual void stop();

  /* do both stop and unInit */
  virtual void cleanup();

  /* start routine for the data receiving thread*/
  static void* readThreadEntry(void *);

  virtual bool isThreadRunning();

  /* register a callback function */
  virtual void registerCallback(CAMCALLBACK f, void* param);

 protected:
  CameraType  camType;
  std::string camNameStr;

  pthread_t readThread;
  int       threadStatus;

  bool isRunning;

  CAMCALLBACK cb;
  void* cbParam;

  /* disconnect link from camera */
  virtual void unInit();

  /* real function to read data from camera */
  virtual void readThreadFunc();
};

#endif // DJICAMERASTREAMLINK_HH
