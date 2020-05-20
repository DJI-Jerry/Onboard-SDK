/** @file dji_camera_stream_link.hpp
 *  @version 3.5
 *  @date Dec 2017
 *
 *  @brief The class to read data from the camera

 *  @copyright 2017 DJI. All rights reserved.
 *
 */

#ifndef DJICAMERASTREAMUDTLINK_HH
#define DJICAMERASTREAMUDTLINK_HH
#include "netdb.h"
#include <string>
#include "dji_camera_stream_link.hpp"
#include "dji_camera_image.hpp"

class DJICameraStreamUdtLink : public DJICameraStreamLink
{
public:
  DJICameraStreamUdtLink(CameraType c);
  ~DJICameraStreamUdtLink();
  /* Establish link to camera */
  bool init();

private:
  std::string ip;
  std::string port;
  int fHandle;

  /* disconnect link from camera */
  void unInit();

  /* real function to read data from camera */
  void readThreadFunc();
};

#endif // DJICAMERASTREAMUDTLINK_HH
