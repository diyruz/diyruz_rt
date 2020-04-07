#define SECURE 1
#define TC_LINKKEY_JOIN
#define NV_INIT
#define NV_RESTORE
#define xZTOOL_P1
#define MT_TASK
#define MT_APP_FUNC
#define MT_SYS_FUNC
#define MT_ZDO_FUNC
#define MT_ZDO_MGMT
#define MT_APP_CNF_FUNC
#define LEGACY_LCD_DEBUG
//#define LCD_SUPPORTED DEBUG
#define MULTICAST_ENABLED FALSE
#define ZCL_READ
#define ZCL_WRITE
#define ZCL_BASIC
#define ZCL_IDENTIFY
#define ZCL_SCENES
#define ZCL_GROUPS
#define ZCL_ON_OFF
#define ZCL_REPORTING_DEVICE

#define DISABLE_GREENPOWER_BASIC_PROXY
#define DEFAULT_CHANLIST 0x07FFF800  // Маска для работы на всех каналах

#define HAL_SONOFF // признак сборки для Sonoff Zigbee

#include "hal_board_cfg_DIYRuZRT.h"