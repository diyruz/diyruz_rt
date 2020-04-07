#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_ms.h"

/* TODO: Дополняйте нужные заголовки для соответствующих кластеров
#include "zcl_poll_control.h"
#include "zcl_electrical_measurement.h"
#include "zcl_diagnostic.h"
#include "zcl_meter_identification.h"
#include "zcl_appliance_identification.h"
#include "zcl_appliance_events_alerts.h"
#include "zcl_power_profile.h"
#include "zcl_appliance_control.h"
#include "zcl_appliance_statistics.h"
#include "zcl_hvac.h"
*/

#include "zcl_DIYRuZRT.h"

// версия устройства и флаги
#define DIYRuZRT_DEVICE_VERSION     0
#define DIYRuZRT_FLAGS              0

// версия оборудования
#define DIYRuZRT_HWVERSION          1
// версия ZCL
#define DIYRuZRT_ZCLVERSION         1

// версия кластеров
const uint16 zclDIYRuZRT_clusterRevision_all = 0x0001; 

// переменные/константы Basic кластера

// версия оборудования
const uint8 zclDIYRuZRT_HWRevision = DIYRuZRT_HWVERSION;
// версия ZCL
const uint8 zclDIYRuZRT_ZCLVersion = DIYRuZRT_ZCLVERSION;
// производитель
const uint8 zclDIYRuZRT_ManufacturerName[] = { 6, 'D','I','Y','R','u','Z' };
// модель устройства
const uint8 zclDIYRuZRT_ModelId[] = { 9, 'D','I','Y','R','u','Z', '_', 'R','T' };
// дата версии
const uint8 zclDIYRuZRT_DateCode[] = { 8, '2','0','2','0','0','4','0','5' };
// вид питания POWER_SOURCE_MAINS_1_PHASE - питание от сети с одной фазой
const uint8 zclDIYRuZRT_PowerSource = POWER_SOURCE_MAINS_1_PHASE;
// расположение устройства
uint8 zclDIYRuZRT_LocationDescription[17] = { 16, ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
uint8 zclDIYRuZRT_PhysicalEnvironment = 0;
uint8 zclDIYRuZRT_DeviceEnable = DEVICE_ENABLED;

// переменные/константы Identify кластера

// время идентификации
uint16 zclDIYRuZRT_IdentifyTime;

// Состояние реле
extern uint8 RELAY_STATE;

// Данные о температуре
#define MAX_MEASURED_VALUE  10000  // 100.00C
#define MIN_MEASURED_VALUE  -10000  // -100.00C

extern int16 zclDIYRuZRT_MeasuredValue;
const int16 zclDIYRuZRT_MinMeasuredValue = MIN_MEASURED_VALUE; 
const uint16 zclDIYRuZRT_MaxMeasuredValue = MAX_MEASURED_VALUE;

// Таблица реализуемых команд для DISCOVER запроса
#if ZCL_DISCOVER
CONST zclCommandRec_t zclDIYRuZRT_Cmds[] =
{
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    COMMAND_BASIC_RESET_FACT_DEFAULT,
    CMD_DIR_SERVER_RECEIVED
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    COMMAND_OFF,
    CMD_DIR_SERVER_RECEIVED
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    COMMAND_OFF,
    CMD_DIR_SERVER_RECEIVED
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    COMMAND_ON,
    CMD_DIR_SERVER_RECEIVED
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    COMMAND_TOGGLE,
    CMD_DIR_SERVER_RECEIVED
  },
};

CONST uint8 zclCmdsArraySize = ( sizeof(zclDIYRuZRT_Cmds) / sizeof(zclDIYRuZRT_Cmds[0]) );
#endif // ZCL_DISCOVER


// Определение атрибутов приложения
CONST zclAttrRec_t zclDIYRuZRT_Attrs[] =
{
  // *** Атрибуты Basic кластера ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // ID кластера - определен в zcl.h
    { // версия оборудования
      ATTRID_BASIC_HW_VERSION,            // ID атрибута - определен в zcl_general.h
      ZCL_DATATYPE_UINT8,                 // Тип данных  - определен zcl.h
      ACCESS_CONTROL_READ,                // Тип доступа к атрибута - определен в zcl.h
      (void *)&zclDIYRuZRT_HWRevision     // Указатель на переменную хранящую значение
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // версия ZCL
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclDIYRuZRT_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // версия приложения
      ATTRID_BASIC_APPL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclDIYRuZRT_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // версия стека
      ATTRID_BASIC_STACK_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclDIYRuZRT_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // версия прошивки
      ATTRID_BASIC_SW_BUILD_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclDIYRuZRT_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // производитель
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclDIYRuZRT_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // модель
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclDIYRuZRT_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // дата версии
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclDIYRuZRT_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // тип питания
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&zclDIYRuZRT_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // расположение
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE), // может быть изменен
      (void *)zclDIYRuZRT_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclDIYRuZRT_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclDIYRuZRT_DeviceEnable
    }
  },

#ifdef ZCL_IDENTIFY
  // *** Атрибуты Identify кластера ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // время идентификации
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclDIYRuZRT_IdentifyTime
    }
  },
#endif
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // версия Basic кластера
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclDIYRuZRT_clusterRevision_all
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // версия Identify кластера
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclDIYRuZRT_clusterRevision_all
    }
  },
  // *** Атрибуты On/Off кластера ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // состояние
      ATTRID_ON_OFF,
      ZCL_DATATYPE_BOOLEAN,
      ACCESS_CONTROL_READ,
      (void *)&RELAY_STATE
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    {  // версия On/Off кластера
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ | ACCESS_CLIENT,
      (void *)&zclDIYRuZRT_clusterRevision_all
    }
  },
  // *** Атрибуты Temperature Measurement кластера ***
  {
    ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
    { // Значение температуры
      ATTRID_MS_TEMPERATURE_MEASURED_VALUE,
      ZCL_DATATYPE_INT16,
      ACCESS_CONTROL_READ | ACCESS_REPORTABLE,
      (void *)&zclDIYRuZRT_MeasuredValue
    }
  },
  {
    ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
    { // минимальное значение температуры
      ATTRID_MS_TEMPERATURE_MIN_MEASURED_VALUE,
      ZCL_DATATYPE_INT16,
      ACCESS_CONTROL_READ,
      (void *)&zclDIYRuZRT_MinMeasuredValue
    }
  },
  {
    ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
    { // максимальное значение температуры
      ATTRID_MS_TEMPERATURE_MAX_MEASURED_VALUE,
      ZCL_DATATYPE_INT16,
      ACCESS_CONTROL_READ,
      (void *)&zclDIYRuZRT_MaxMeasuredValue
    }
  },
  {
    ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
    {  // версия кластера
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclDIYRuZRT_clusterRevision_all
    }
  },
};

uint8 CONST zclDIYRuZRT_NumAttributes = ( sizeof(zclDIYRuZRT_Attrs) / sizeof(zclDIYRuZRT_Attrs[0]) );

// Список входящих кластеров приложения
const cId_t zclDIYRuZRT_InClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_GROUPS,
  ZCL_CLUSTER_ID_GEN_ON_OFF,
  ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
  
  // TODO: Add application specific Input Clusters Here. 
  //       See zcl.h for Cluster ID definitions
  
};
#define ZCLDIYRuZRT_MAX_INCLUSTERS   (sizeof(zclDIYRuZRT_InClusterList) / sizeof(zclDIYRuZRT_InClusterList[0]))

// Список исходящих кластеров приложения
const cId_t zclDIYRuZRT_OutClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  
  // TODO: Add application specific Output Clusters Here. 
  //       See zcl.h for Cluster ID definitions
};
#define ZCLDIYRuZRT_MAX_OUTCLUSTERS  (sizeof(zclDIYRuZRT_OutClusterList) / sizeof(zclDIYRuZRT_OutClusterList[0]))

// Структура описания эндпоинта
SimpleDescriptionFormat_t zclDIYRuZRT_SimpleDesc =
{
  DIYRuZRT_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                  //  uint16 AppProfId;
  // TODO: Replace ZCL_HA_DEVICEID_ON_OFF_LIGHT with application specific device ID
  ZCL_HA_DEVICEID_ON_OFF_LIGHT,       //  uint16 AppDeviceId; 
  DIYRuZRT_DEVICE_VERSION,            //  int   AppDevVer:4;
  DIYRuZRT_FLAGS,                     //  int   AppFlags:4;
  ZCLDIYRuZRT_MAX_INCLUSTERS,         //  byte  AppNumInClusters;
  (cId_t *)zclDIYRuZRT_InClusterList, //  byte *pAppInClusterList;
  ZCLDIYRuZRT_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
  (cId_t *)zclDIYRuZRT_OutClusterList //  byte *pAppInClusterList;
};

// Сброс атрибутов в значения по-умолчанию  
void zclDIYRuZRT_ResetAttributesToDefaultValues(void)
{
  int i;
  
  zclDIYRuZRT_LocationDescription[0] = 16;
  for (i = 1; i <= 16; i++)
  {
    zclDIYRuZRT_LocationDescription[i] = ' ';
  }
  
  zclDIYRuZRT_PhysicalEnvironment = PHY_UNSPECIFIED_ENV;
  zclDIYRuZRT_DeviceEnable = DEVICE_ENABLED;
  
#ifdef ZCL_IDENTIFY
  zclDIYRuZRT_IdentifyTime = 0;
#endif
}