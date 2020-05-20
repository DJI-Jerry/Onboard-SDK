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

#include "dji_camera_stream_bulk_link.hpp"
#include "dji_log.hpp"
#include "osdk_command.h"
#include <map>

#ifndef WIN32
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#else
#include <winsock2.h>
  #include <ws2tcpip.h>
  #include <wspiapi.h>
#endif

#define IMAGE_FILE_PATH_LEN                     (64)

#define OSDK_CMDSET_LIVEVIEW                    (0x08)
#define OSDK_CMDID_SUBSCRIBE_LIVEVIEW_STATUS    (0x65)
#define OSDK_CMDID_PUSH_LIVEVIEW_STATUS         (0x66)
#define OSDK_CMDID_LIVEVIEW_HEARTBEAT           (0x67)
#define OSDK_CMDID_REQ_SDR_LIVEVIEW             (0x68)

#define OSDK_CAMERA_INDEX_LIVEVIEW              (0)
#define OSDK_GD610_INDEX_LIVEVIEW               (1)

#define OSDK_FPV_LIVEVIEW_CHANNEL               (81)
#define OSDK_MAIN_CAMERA_LIVEVIEW_CHANNEL       (82)
#define OSDK_VICE_CAMERA_LIVEVIEW_CHANNEL       (85)
#define OSDK_TOP_CAMERA_LIVEVIEW_CHANNEL        (90)

#define LIVEVIEW_TEMP_CMD_SET                   (0x65)
#define LIVEVIEW_FPV_CAM_TEMP_CMD_ID            (0x54)
#define LIVEVIEW_MAIN_CAM_TEMP_CMD_ID           (0x55)
#define LIVEVIEW_VICE_CAM_TEMP_CMD_ID           (0x56)
#define LIVEVIEW_TOP_CAM_TEMP_CMD_ID            (0x57)

bool DJICameraStreamBulkLink::cbRegistered = false;

typedef struct H264CallbackHandler {
  CAMCALLBACK cb;
  void *userData;
} H264CallbackHandler;

std::map<CameraType, H264CallbackHandler> h264CbHandlerMap = {
    {MAIN_CAMERA, {NULL, NULL}},
    {VICE_CAMERA, {NULL, NULL}},
    {TOP_CAMERA,  {NULL, NULL}},
    {FPV_CAMERA,  {NULL, NULL}},
};

E_OsdkStat RecordStreamHandler(struct _CommandHandle *cmdHandle,
                               const T_CmdInfo *cmdInfo,
                               const uint8_t *cmdData,
                               void *userData);

T_RecvCmdItem bulkCmdList[] = {
    PROT_CMD_ITEM(0, 0, LIVEVIEW_TEMP_CMD_SET, LIVEVIEW_FPV_CAM_TEMP_CMD_ID,  MASK_HOST_DEVICE_SET_ID, (void *)&h264CbHandlerMap, RecordStreamHandler),
    PROT_CMD_ITEM(0, 0, LIVEVIEW_TEMP_CMD_SET, LIVEVIEW_MAIN_CAM_TEMP_CMD_ID, MASK_HOST_DEVICE_SET_ID, (void *)&h264CbHandlerMap, RecordStreamHandler),
    PROT_CMD_ITEM(0, 0, LIVEVIEW_TEMP_CMD_SET, LIVEVIEW_VICE_CAM_TEMP_CMD_ID, MASK_HOST_DEVICE_SET_ID, (void *)&h264CbHandlerMap, RecordStreamHandler),
    PROT_CMD_ITEM(0, 0, LIVEVIEW_TEMP_CMD_SET, LIVEVIEW_TOP_CAM_TEMP_CMD_ID,  MASK_HOST_DEVICE_SET_ID, (void *)&h264CbHandlerMap, RecordStreamHandler),
};

E_OsdkStat RecordStreamHandler(struct _CommandHandle *cmdHandle,
                               const T_CmdInfo *cmdInfo,
                               const uint8_t *cmdData,
                               void *userData) {
  if ((!cmdInfo) || (!userData)) {
    DERROR("Recv Info is a null value");
    return OSDK_STAT_ERR;
  }

  std::map<CameraType, H264CallbackHandler> handlerMap =
      *(std::map<CameraType, H264CallbackHandler> *)userData;

  CameraType pos;
  switch (cmdInfo->cmdId) {
    case LIVEVIEW_FPV_CAM_TEMP_CMD_ID:
      pos = FPV_CAMERA;
      break;
    case LIVEVIEW_MAIN_CAM_TEMP_CMD_ID:
      pos = MAIN_CAMERA;
      break;
    case LIVEVIEW_VICE_CAM_TEMP_CMD_ID:
      pos = VICE_CAMERA;
      break;
    case LIVEVIEW_TOP_CAM_TEMP_CMD_ID:
      pos = TOP_CAMERA;
      break;
    default:
      return OSDK_STAT_ERR_OUT_OF_RANGE;
  }

  if((handlerMap.find(pos) != handlerMap.end()) && (handlerMap[pos].cb != NULL)) {
    handlerMap[pos].cb(handlerMap[pos].userData, (uint8_t *)cmdData, cmdInfo->dataLen);
  } else {
    DERROR("Can't find valid cb in handlerMap");
  }

  return OSDK_STAT_OK;
}


DJICameraStreamBulkLink::DJICameraStreamBulkLink(CameraType c, DJI::OSDK::Linker *linker) : linker(linker), DJICameraStreamLink(c)
{
  if (!cbRegistered) {
    T_RecvCmdHandle recvCmdHandle;
    recvCmdHandle.cmdList = bulkCmdList;
    recvCmdHandle.cmdCount = sizeof(bulkCmdList) / sizeof(T_RecvCmdItem);
    recvCmdHandle.protoType = PROTOCOL_USBMC;

    if (!linker->registerCmdHandler(&recvCmdHandle)) {
      DERROR("register h264 cmd callback handler failed, exiting.");
    }
    cbRegistered = true;
  }
}

DJICameraStreamBulkLink::~DJICameraStreamBulkLink()
{
  cleanup();
}


void *heartBeatTask(void *p) {
  DJI::OSDK::Linker *linker = (DJI::OSDK::Linker *) p;
  struct timeval start;
  struct timeval end;
  uint32_t timeMs;
  T_CmdInfo sendInfo;
  uint8_t data = 2;
  int result;

  sendInfo.cmdSet = OSDK_CMDSET_LIVEVIEW;
  sendInfo.cmdId = OSDK_CMDID_LIVEVIEW_HEARTBEAT;
  sendInfo.dataLen = 1;
  sendInfo.needAck = OSDK_COMMAND_NEED_ACK_FINISH_ACK;
  sendInfo.packetType = OSDK_COMMAND_PACKET_TYPE_REQUEST;
  sendInfo.addr = GEN_ADDR(0, ADDR_V1_COMMAND_INDEX);
  sendInfo.encType = 0;
  sendInfo.sender = linker->getLocalSenderId();
  sendInfo.receiver = 0x08;

  while (1) {
    OsdkOsal_TaskSleepMs(1000);
    result = linker->send(&sendInfo, &data);
    if (result != OSDK_STAT_OK) {
      DERROR("heart beat task send failed!\n");
    }
  }
}

E_OsdkStat DJICameraStreamBulkLink::startHeartBeatTask() {
  return OsdkOsal_TaskCreate(&hbTaskHandle, heartBeatTask,
                             OSDK_TASK_STACK_SIZE_DEFAULT,
                             linker);
}

E_OsdkStat DJICameraStreamBulkLink::stopHeartBeatTask() {
  return OsdkOsal_TaskDestroy(hbTaskHandle);
}


bool DJICameraStreamBulkLink::init()
{
  return true;
}


E_OsdkStat DJICameraStreamBulkLink::getCameraPushing(struct _CommandHandle *cmdHandle,
                                          const T_CmdInfo *cmdInfo,
                                          const uint8_t *cmdData,
                                          void *userData) {
  if (cmdInfo) {
    /*OSDK_LOG_ERROR("test pushing", "0x80 puhsing sender=0x%02X receiver:0x%02X len=%d", cmdInfo->sender,
                   cmdInfo->receiver, cmdInfo->dataLen);*/
    if (userData && (cmdInfo->dataLen >= 34)) {
      CameraListType *camList = (CameraListType *) userData;
      switch (cmdInfo->sender) {
        case 0x01:
        case 0x21:
          camList->isMounted[0] = true;
          camList->cameraType[0] = (E_OSDKCameraType) cmdData[33];
          break;
        case 0x41:
        case 0x61:
          camList->isMounted[1] = true;
          camList->cameraType[1] = (E_OSDKCameraType) cmdData[33];
          break;
        case 0x81:
        case 0xA1:
          camList->isMounted[2] = true;
          camList->cameraType[2] = (E_OSDKCameraType) cmdData[33];
          break;
      }
    } else {
      DERROR("cmdInfo is a null value");
    }
    return OSDK_STAT_OK;
  } else {
    return OSDK_STAT_SYS_ERR;
  }
}

DJICameraStreamBulkLink::CameraListType DJICameraStreamBulkLink::getCameraList() {
  static T_RecvCmdHandle handle = {0};
  static T_RecvCmdItem item = {0};
  static CameraListType typeList;

  for (int i = 0; i < 3; i++) {
    typeList.isMounted[i] = false;
  }

  handle.protoType = PROTOCOL_V1;
  handle.cmdCount = 1;
  handle.cmdList = &item;
  item.cmdSet = 0x02;
  item.cmdId = 0x80;
  item.mask = MASK_HOST_XXXXXX_SET_ID;
  item.host = 0;
  item.device = 0;
  item.pFunc = getCameraPushing;
  item.userData = &typeList;

  bool registerRet = linker->registerCmdHandler(&(handle));
  DSTATUS("register result of geting camera pushing : %d\n", registerRet);

  uint8_t reqStartData[] = {0x01, 0x00, 0x02, 0x80};
  T_CmdInfo cmdInfo = {0};
  T_CmdInfo ackInfo = {0};
  uint8_t ackData[1024];

  cmdInfo.cmdSet = 0x05;
  cmdInfo.cmdId = 0x0b;
  cmdInfo.dataLen = sizeof(reqStartData);
  cmdInfo.needAck = OSDK_COMMAND_NEED_ACK_FINISH_ACK;
  cmdInfo.packetType = OSDK_COMMAND_PACKET_TYPE_REQUEST;
  cmdInfo.addr = GEN_ADDR(0, ADDR_V1_COMMAND_INDEX);
  cmdInfo.receiver =
      OSDK_COMMAND_DEVICE_ID(OSDK_COMMAND_DEVICE_TYPE_CENTER, 0);
  cmdInfo.sender = linker->getLocalSenderId();
  E_OsdkStat linkAck =
      linker->sendSync(&cmdInfo, (uint8_t *) reqStartData, &ackInfo, ackData,
                                1000 / 4, 4);
  DSTATUS("Request start pushing camera info ack = %d\n", linkAck);

  OsdkOsal_TaskSleepMs(1000);

  uint8_t reqEndData[] = {0x00, 0x00, 0x02, 0x80};
  cmdInfo.cmdSet = 0x05;
  cmdInfo.cmdId = 0x0b;
  cmdInfo.dataLen = sizeof(reqEndData);
  cmdInfo.needAck = OSDK_COMMAND_NEED_ACK_FINISH_ACK;
  cmdInfo.packetType = OSDK_COMMAND_PACKET_TYPE_REQUEST;
  cmdInfo.addr = GEN_ADDR(0, ADDR_V1_COMMAND_INDEX);
  cmdInfo.receiver =
      OSDK_COMMAND_DEVICE_ID(OSDK_COMMAND_DEVICE_TYPE_CENTER, 0);
  cmdInfo.sender = linker->getLocalSenderId();
  linkAck = linker->sendSync(&cmdInfo, (uint8_t *) reqEndData, &ackInfo, ackData,
                                1000 / 4, 4);
  DSTATUS("Request end pushing camera info ack = %d\n", linkAck);
  DSTATUS("mounted\tcam0:%d cam2:%d cam3:%d", typeList.isMounted[0], typeList.isMounted[1], typeList.isMounted[2]);
  DSTATUS("type   \tcam0:%d cam2:%d cam3:%d", typeList.cameraType[0], typeList.cameraType[1], typeList.cameraType[2]);
  return typeList;
}

bool DJICameraStreamBulkLink::start() {
  if (isRunning)
  {
    DSTATUS_PRIVATE("Already reading data from %s\n", camNameStr.c_str());
    return false;
  }
  isRunning = true;
  E_OSDKCameraType type;
  uint8_t deviceSite;
  if ((camType == MAIN_CAMERA) || (camType == VICE_CAMERA) || (camType == TOP_CAMERA)) {
    deviceSite = (camType == MAIN_CAMERA) ? OSDK_CAMERA_POSITION_NO_1 :
                 ((camType == VICE_CAMERA) ? OSDK_CAMERA_POSITION_NO_2 :
                 OSDK_CAMERA_POSITION_NO_3);
    CameraListType cameraList = getCameraList();
    if (cameraList.isMounted[camType]) {
      DSTATUS("camera[%d] is mounted\n", camType);
      type = cameraList.cameraType[camType];
    } else {
      DERROR("camera[%d] is not mounted\n", camType);
      return false;
    }
  } else if (camType == FPV_CAMERA){
    DSTATUS("Getting FPV Test");
    type = OSDK_CAMERA_TYPE_FPV;
    deviceSite = OSDK_CAMERA_POSITION_FPV;
  } else {
    DERROR("camera[%d] is not mounted\n", type);
    return false;
  }

  if (h264CbHandlerMap.find(camType) != h264CbHandlerMap.end()) {
    h264CbHandlerMap[camType].cb = cb;
    h264CbHandlerMap[camType].userData = cbParam;
  }

  T_CmdInfo sendInfo;
  T_CmdInfo ackInfo;
  uint8_t ackData[1024];
  int result;

  T_LiveViewSubscribeItem *subCtx;
  subCtx = (T_LiveViewSubscribeItem *)malloc(sizeof(T_LiveViewSubscribeItem));
  if(subCtx == NULL) {
    printf("malloc failed!\n");
    goto malloc_fail;
  }
  memset(subCtx, 0, sizeof(T_LiveViewSubscribeItem));
  subCtx->header.role = 2;
  subCtx->header.chnNum = 1;
  subCtx->channel.cmdSize = 0x25;//sizeof(T_LiveViewChannelItem)
      + sizeof(T_LiveViewSingleSourceItem);
  subCtx->channel.mode = 0;
  subCtx->channel.action = 0; //open

  switch (camType) {
    case FPV_CAMERA:
      subCtx->channel.channelId = OSDK_FPV_LIVEVIEW_CHANNEL - 1;
      break;
    case MAIN_CAMERA:
      subCtx->channel.channelId = OSDK_MAIN_CAMERA_LIVEVIEW_CHANNEL - 1;
      break;
    case VICE_CAMERA:
      subCtx->channel.channelId = OSDK_VICE_CAMERA_LIVEVIEW_CHANNEL - 1;
      break;
    case TOP_CAMERA:
      subCtx->channel.channelId = OSDK_TOP_CAMERA_LIVEVIEW_CHANNEL - 1;
      break;
  }

  subCtx->channel.priority = 0;
  subCtx->channel.fps = 30;
  subCtx->channel.width = 0;
  subCtx->channel.height = 0;
  subCtx->channel.codecStrategy = 0;
  subCtx->channel.codecFormat = 0;
  subCtx->channel.adaptiveResolutionEnable = 1;
  subCtx->channel.sourceNum = 1;
  subCtx->channel.size = sizeof(T_LiveViewSingleSourceItem);
  subCtx->source.uuid.version = 1;
  if(type == OSDK_CAMERA_TYPE_PSDK)   subCtx->source.uuid.major = UUID_MAJOR_TYPE_PSDK;
  else subCtx->source.uuid.major = UUID_MAJOR_TYPE_CAMERA;
  subCtx->source.uuid.minor = ((type == OSDK_CAMERA_TYPE_PSDK) ? 0 : type);
  subCtx->source.uuid.reserved = 0;
  if ((type == OSDK_CAMERA_TYPE_GD610_DOUBLE_CAM)
      || (type == OSDK_CAMERA_TYPE_GD610_TIRPLE_CAM))
    subCtx->source.uuid.dataIdx = OSDK_GD610_INDEX_LIVEVIEW;
  else
    subCtx->source.uuid.dataIdx = OSDK_CAMERA_INDEX_LIVEVIEW;

  subCtx->source.cropEnable = 1;
  subCtx->source.cropOffsetX = 0;
  subCtx->source.cropOffsetY = 0;
  subCtx->source.cropWidth = 0;
  subCtx->source.cropHeight = 0;
  subCtx->source.uuid.devicePos = deviceSite;

  sendInfo.cmdSet = OSDK_CMDSET_LIVEVIEW;
  sendInfo.cmdId = OSDK_CMDID_REQ_SDR_LIVEVIEW;
  sendInfo.dataLen = sizeof(T_LiveViewSubscribeItem);
  sendInfo.needAck = OSDK_COMMAND_NEED_ACK_FINISH_ACK;
  sendInfo.packetType = OSDK_COMMAND_PACKET_TYPE_REQUEST;
  sendInfo.addr = GEN_ADDR(0, ADDR_V1_COMMAND_INDEX);
  sendInfo.encType = 0;
  sendInfo.sender = linker->getLocalSenderId();
  sendInfo.receiver = 0x08;
  result = linker->sendSync(&sendInfo, (uint8_t *)subCtx, &ackInfo, ackData, 2000, 3);
  if(result != OSDK_STAT_OK) {
    if(result == OSDK_STAT_ERR_TIMEOUT) {
      printf("sendSync timeout, no ack receive!\n");
    }
    printf("sendSync failed!\n");
    goto send_fail;
  }
  if(ackInfo.dataLen == 1 && ackData[0] == 0) {
    printf("subsrcibe data success!\n");
  } else {
    printf("subscribe data failed!\n");
    printf("ackData:");
    for(int i = 0; i < ackInfo.dataLen; i++) {
      printf("%x", ackData[i]);
    }
    printf("\nend\n");
    goto subscribe_fail;
  }
  free(subCtx);

  startHeartBeatTask();
  isRunning = true;
  return true;

  subscribe_fail:
  send_fail:
  free(subCtx);
  malloc_fail:
  isRunning = false;
  return false;
}

void DJICameraStreamBulkLink::stop() {
  if (!isRunning) return;
  isRunning = false;
  T_CmdInfo sendInfo;
  T_CmdInfo ackInfo;
  uint8_t ackData[1024];
  int result;

  T_LiveViewUnsubscribeItem *unsubCtx;
  unsubCtx = (T_LiveViewUnsubscribeItem *)malloc(sizeof(T_LiveViewUnsubscribeItem));
  if(unsubCtx == NULL) {
    printf("malloc failed!\n");
    goto malloc_fail;
  }

  unsubCtx->header.role = 2;
  unsubCtx->header.chnNum = 1;
  unsubCtx->channel.cmdSize = sizeof(T_LiveViewMiniChannelItem);
  unsubCtx->channel.mode = 0;
  unsubCtx->channel.action = 1; //close

  switch (camType) {
    case FPV_CAMERA:
      unsubCtx->channel.channelId = OSDK_FPV_LIVEVIEW_CHANNEL - 1;
      break;
    case MAIN_CAMERA:
      unsubCtx->channel.channelId = OSDK_MAIN_CAMERA_LIVEVIEW_CHANNEL - 1;
      break;
    case VICE_CAMERA:
      unsubCtx->channel.channelId = OSDK_VICE_CAMERA_LIVEVIEW_CHANNEL - 1;
      break;
    case TOP_CAMERA:
      unsubCtx->channel.channelId = OSDK_TOP_CAMERA_LIVEVIEW_CHANNEL - 1;
      break;
  }
  unsubCtx->channel.priority = 0;

  sendInfo.cmdSet = OSDK_CMDSET_LIVEVIEW;
  sendInfo.cmdId = OSDK_CMDID_REQ_SDR_LIVEVIEW;
  sendInfo.dataLen = sizeof(T_LiveViewUnsubscribeItem);
  sendInfo.needAck = OSDK_COMMAND_NEED_ACK_FINISH_ACK;
  sendInfo.packetType = OSDK_COMMAND_PACKET_TYPE_REQUEST;
  sendInfo.addr = GEN_ADDR(0, ADDR_V1_COMMAND_INDEX);
  sendInfo.encType = 0;
  sendInfo.sender = linker->getLocalSenderId();
  sendInfo.receiver = 0x08;
  result = linker->sendSync(&sendInfo, (uint8_t *)unsubCtx, &ackInfo, ackData, 2000, 3);
  if(result != OSDK_STAT_OK) {
    if(result == OSDK_STAT_ERR_TIMEOUT) {
      printf("sendSync timeout, no ack receive!\n");
    }
    printf("sendSync failed!\n");
    goto send_fail;
  }
  if(ackInfo.dataLen == 1 && ackData[0] == 0) {
    printf("unsubsrcibe data success!\n");
  } else {
    printf("unsubscribe data failed!\n");
    printf("ackData:");
    for(int i = 0; i < ackInfo.dataLen; i++) {
      printf("%x", ackData[i]);
    }
    printf("\nend\n");
    goto unsubscribe_fail;
  }
  free(unsubCtx);

  stopHeartBeatTask();

  return;

  unsubscribe_fail:
  send_fail:
  free(unsubCtx);
  malloc_fail:
  DERROR("%s flow stop fail.", camNameStr.c_str());
  return;
}

void DJICameraStreamBulkLink::unInit()
{
}
