/** @file dji_camera_stream_link.hpp
 *  @version 3.5
 *  @date Dec 2017
 *
 *  @brief The class to read data from the camera

 *  @copyright 2017 DJI. All rights reserved.
 *
 */

#ifndef DJICAMERASTREAMBULKLINK_HH
#define DJICAMERASTREAMBULKLINK_HH
#include <string>
#include "dji_camera_stream_link.hpp"
#include "dji_camera_image.hpp"
#include "dji_linker.hpp"

using namespace DJI;
using namespace DJI::OSDK;

class Linker;
class DJICameraStreamBulkLink : public DJICameraStreamLink
{
 public:
  DJICameraStreamBulkLink(CameraType c, DJI::OSDK::Linker *linker);
  ~DJICameraStreamBulkLink();
  /* Establish link to camera */
  bool init();
  bool start();
  void stop();
 private:

  /* disconnect link from camera */
  void unInit();

  DJI::OSDK::Linker *linker;
 private:
  static bool cbRegistered;

  T_OsdkTaskHandle hbTaskHandle;
  E_OsdkStat startHeartBeatTask();
  E_OsdkStat stopHeartBeatTask();

 private: //private defination for liveview

  typedef enum E_OSDKCameraType {
    OSDK_CAMERA_TYPE_FC350 = 0,
    OSDK_CAMERA_TYPE_FC550 = 1,
    OSDK_CAMERA_TYPE_FC260 = 2,
    OSDK_CAMERA_TYPE_FC300S = 3,
    OSDK_CAMERA_TYPE_FC300X = 4,
    OSDK_CAMERA_TYPE_FC550RAW = 5,
    OSDK_CAMERA_TYPE_FC330 = 6,
    OSDK_CAMERA_TYPE_TAU640 = 7,   //XT_640
    OSDK_CAMERA_TYPE_TAU336 = 8,   //XT_336
    OSDK_CAMERA_TYPE_FC220 = 9,
    OSDK_CAMERA_TYPE_FC300XW = 10,
    OSDK_CAMERA_TYPE_CV600 = 11,  //3.5X
    OSDK_CAMERA_TYPE_FC65XX = 12,
    OSDK_CAMERA_TYPE_FC6310 = 13,
    OSDK_CAMERA_TYPE_FC6510 = 14,
    OSDK_CAMERA_TYPE_FC6520 = 15,
    OSDK_CAMERA_TYPE_FC6532 = 16,
    OSDK_CAMERA_TYPE_FC6540 = 17,
    OSDK_CAMERA_TYPE_FC220L = 18,
    OSDK_CAMERA_TYPE_FC1102 = 19,
    OSDK_CAMERA_TYPE_GD600 = 20,  //30X, Z30
    OSDK_CAMERA_TYPE_FC6310A = 21,
    OSDK_CAMERA_TYPE_FC300SE = 22,
    OSDK_CAMERA_TYPE_WM230 = 23,
    OSDK_CAMERA_TYPE_FC1705 = 26,  //XT2
    OSDK_CAMERA_TYPE_PSDK = 31,
    OSDK_CAMERA_TYPE_FPV = 39,  //Matrice FPV
    OSDK_CAMERA_TYPE_TP1810 = 41,  //XTS
    OSDK_CAMERA_TYPE_GD610_DOUBLE_CAM = 42,
    OSDK_CAMERA_TYPE_GD610_TIRPLE_CAM = 43, //IR
    OSDK_CAMERA_TYPE_UNKNOWN = 0xFF
  } E_OSDKCameraType;

  enum {
    UUID_MAJOR_TYPE_CAMERA = 0,
    UUID_MAJOR_TYPE_RADAR = 1,
    UUID_MAJOR_TYPE_PSDK = 2,
    UUID_MAJOR_TYPE_UNKNOWN = 255
  };

  typedef struct {
    // 2:OSDK
    uint8_t role;
    // 0:subscribe 1:unsubscribe
    uint8_t action;
    // always 0
    uint8_t type;
  } __attribute__((packed)) T_SubscribeData;

  typedef struct {
    uint8_t role;
  } __attribute__((packed)) T_HeartBeatData;

  typedef struct {
    uint8_t role;
    uint8_t chnNum;
  } __attribute__((packed)) T_LiveViewHeader;

  typedef struct {
    uint16_t cmdSize;
    uint8_t mode;
    uint8_t action;
    uint8_t channelId;
    uint8_t priority;
  } __attribute__((packed)) T_LiveViewMiniChannelItem;

  typedef struct {
    uint16_t cmdSize;
    uint8_t mode;
    uint8_t action;
    uint8_t channelId;
    uint8_t priority;
    uint8_t fps;
    uint16_t width;
    uint16_t height;
    uint8_t codecStrategy;
    uint8_t codecFormat;
    uint8_t adaptiveResolutionEnable;
    uint8_t sourceNum;
    uint8_t size;
  } __attribute__((packed)) T_LiveViewChannelItem;

  typedef struct {
    uint8_t version;
    uint8_t major;
    uint8_t minor;
    uint8_t dataIdx : 3;
    uint8_t devicePos : 3;
    uint8_t reserved : 2;
  } __attribute__((packed)) T_LiveViewUuid;

  typedef struct {
    T_LiveViewUuid uuid;
    uint8_t cropEnable;
    float cropOffsetX;
    float cropOffsetY;
    float cropWidth;
    float cropHeight;
  } __attribute__((packed)) T_LiveViewSingleSourceItem;

  typedef struct {
    T_LiveViewUuid uuid;
    uint8_t cropEnable;
    float cropOffsetX;
    float cropOffsetY;
    float cropWidth;
    float cropHeight;
    uint8_t order;
    float blendingOffsetX;
    float blendingOffsetY;
    float blendingWidth;
    float blendingHeight;
  } __attribute__((packed)) T_LiveViewMultiSourceItem;

  typedef struct {
    T_LiveViewHeader header;
    T_LiveViewChannelItem channel;
    T_LiveViewSingleSourceItem source;
  } __attribute__((packed)) T_LiveViewSubscribeItem;

  typedef struct {
    T_LiveViewHeader header;
    T_LiveViewMiniChannelItem channel;
  } __attribute__((packed)) T_LiveViewUnsubscribeItem;

  typedef struct CameraListType
  {
    E_OSDKCameraType cameraType[3];
    bool isMounted[3];
  } CameraListType;

  typedef enum {
    OSDK_CAMERA_POSITION_NO_1 = 0,
    OSDK_CAMERA_POSITION_NO_2 = 1,
    OSDK_CAMERA_POSITION_NO_3 = 2,
    OSDK_CAMERA_POSITION_FPV = 7
  } LiveViewCameraPosition;

  static E_OsdkStat getCameraPushing(struct _CommandHandle *cmdHandle,
                                     const T_CmdInfo *cmdInfo,
                                     const uint8_t *cmdData,
                                     void *userData);

  CameraListType getCameraList();
};

#endif // DJICAMERASTREAMBULKLINK_HH
