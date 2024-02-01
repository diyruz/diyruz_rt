#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "MT_SYS.h"

#include "nwk_util.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_ms.h"
#include "zcl_diagnostic.h"
#include "zcl_DIYRuZRT.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"

#include "onboard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_drivers.h"

#include "ds18b20.h"

// Идентификатор задачи нашего приложения
byte zclDIYRuZRT_TaskID;

// Состояние сети
devStates_t zclDIYRuZRT_NwkState = DEV_INIT;

// Состояние кнопок
static uint8 halKeySavedKeys;

// Состояние реле
uint8 RELAY_STATE = 0;

// Данные о температуре
int16 zclDIYRuZRT_MeasuredValue;

// Структура для отправки отчета
afAddrType_t zclDIYRuZRT_DstAddr;
// Номер сообщения
uint8 SeqNum = 0;

static void zclDIYRuZRT_HandleKeys( byte shift, byte keys );
static void zclDIYRuZRT_BasicResetCB( void );
static void zclDIYRuZRT_ProcessIdentifyTimeChange( uint8 endpoint );

static void zclDIYRuZRT_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);

// Функции обработки входящих сообщений ZCL Foundation команд/ответов
static void zclDIYRuZRT_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static uint8 zclDIYRuZRT_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zclDIYRuZRT_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclDIYRuZRT_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zclDIYRuZRT_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclDIYRuZRT_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclDIYRuZRT_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg );
#endif

// Изменение состояние реле
static void updateRelay( bool );
// Отображение состояния реле на пинах
static void applyRelay( void );
// Выход из сети
void zclDIYRuZRT_LeaveNetwork( void );
// Отправка отчета о состоянии реле
void zclDIYRuZRT_ReportOnOff( void );
// Отправка отчета о температуре
void zclDIYRuZRT_ReportTemp( void );

/*********************************************************************
 * Таблица обработчиков основных ZCL команд
 */
static zclGeneral_AppCallbacks_t zclDIYRuZRT_CmdCallbacks =
{
  zclDIYRuZRT_BasicResetCB,               // Basic Cluster Reset command
  NULL,                                   // Identify Trigger Effect command
  zclDIYRuZRT_OnOffCB,                    // On/Off cluster commands
  NULL,                                   // On/Off cluster enhanced command Off with Effect
  NULL,                                   // On/Off cluster enhanced command On with Recall Global Scene
  NULL,                                   // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_LEVEL_CTRL
  NULL,                                   // Level Control Move to Level command
  NULL,                                   // Level Control Move command
  NULL,                                   // Level Control Step command
  NULL,                                   // Level Control Stop command
#endif
#ifdef ZCL_GROUPS
  NULL,                                   // Group Response commands
#endif
#ifdef ZCL_SCENES
  NULL,                                  // Scene Store Request command
  NULL,                                  // Scene Recall Request command
  NULL,                                  // Scene Response command
#endif
#ifdef ZCL_ALARMS
  NULL,                                  // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
  NULL,                                  // Get Event Log command
  NULL,                                  // Publish Event Log command
#endif
  NULL,                                  // RSSI Location command
  NULL                                   // RSSI Location Response command
};

/*********************************************************************
 * TODO: Add other callback structures for any additional application specific 
 *       Clusters being used, see available callback structures below.
 *
 *       bdbTL_AppCallbacks_t 
 *       zclApplianceControl_AppCallbacks_t 
 *       zclApplianceEventsAlerts_AppCallbacks_t 
 *       zclApplianceStatistics_AppCallbacks_t 
 *       zclElectricalMeasurement_AppCallbacks_t 
 *       zclGeneral_AppCallbacks_t 
 *       zclGp_AppCallbacks_t 
 *       zclHVAC_AppCallbacks_t 
 *       zclLighting_AppCallbacks_t 
 *       zclMS_AppCallbacks_t 
 *       zclPollControl_AppCallbacks_t 
 *       zclPowerProfile_AppCallbacks_t 
 *       zclSS_AppCallbacks_t  
 *
 */


// Функция инициализации задачи приложения
void zclDIYRuZRT_Init( byte task_id )
{
  zclDIYRuZRT_TaskID = task_id;
  
  // Регистрация описания профиля Home Automation Profile
  bdb_RegisterSimpleDescriptor( &zclDIYRuZRT_SimpleDesc );

  // Регистрация обработчиков ZCL команд
  zclGeneral_RegisterCmdCallbacks( DIYRuZRT_ENDPOINT, &zclDIYRuZRT_CmdCallbacks );
  
  // TODO: Register other cluster command callbacks here

  // Регистрация атрибутов кластеров приложения
  zcl_registerAttrList( DIYRuZRT_ENDPOINT, zclDIYRuZRT_NumAttributes, zclDIYRuZRT_Attrs );

  // Подписка задачи на получение сообщений о командах/ответах
  zcl_registerForMsg( zclDIYRuZRT_TaskID );

#ifdef ZCL_DISCOVER
  // Регистрация списка команд, реализуемых приложением
  zcl_registerCmdList( DIYRuZRT_ENDPOINT, zclCmdsArraySize, zclDIYRuZRT_Cmds );
#endif

  // Подписка задачи на получение всех событий для кнопок
  RegisterForKeys( zclDIYRuZRT_TaskID );

  bdb_RegisterCommissioningStatusCB( zclDIYRuZRT_ProcessCommissioningStatus );
  bdb_RegisterIdentifyTimeChangeCB( zclDIYRuZRT_ProcessIdentifyTimeChange );

#ifdef ZCL_DIAGNOSTIC
  // Register the application's callback function to read/write attribute data.
  // This is only required when the attribute data format is unknown to ZCL.
  zcl_registerReadWriteCB( DIYRuZRT_ENDPOINT, zclDiagnostic_ReadWriteAttrCB, NULL );

  if ( zclDiagnostic_InitStats() == ZSuccess )
  {
    // Here the user could start the timer to save Diagnostics to NV
  }
#endif
  
  // Установка адреса и эндпоинта для отправки отчета
  zclDIYRuZRT_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  zclDIYRuZRT_DstAddr.endPoint = 0;
  zclDIYRuZRT_DstAddr.addr.shortAddr = 0;
  
  // инициализируем NVM для хранения RELAY STATE
  if ( SUCCESS == osal_nv_item_init( NV_DIYRuZRT_RELAY_STATE_ID, 1, &RELAY_STATE ) ) {
    // читаем значение RELAY STATE из памяти
    osal_nv_read( NV_DIYRuZRT_RELAY_STATE_ID, 0, 1, &RELAY_STATE );
  }
  // применяем состояние реле
  applyRelay();

  // запускаем повторяемый таймер события HAL_KEY_EVENT через 100мс
  osal_start_reload_timer( zclDIYRuZRT_TaskID, HAL_KEY_EVENT, 100);
  
  // запускаем повторяемый таймер для информирования о температуре (60 сек)
  osal_start_reload_timer( zclDIYRuZRT_TaskID, DIYRuZRT_REPORTING_EVT,
                           60000 );
  
  // Старт процесса возвращения в сеть
  bdb_StartCommissioning(BDB_COMMISSIONING_MODE_PARENT_LOST);
}


// Основной цикл обрабоки событий задачи
uint16 zclDIYRuZRT_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclDIYRuZRT_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          // Обработка входящего сообщения ZCL Foundation комнды/ответа
          zclDIYRuZRT_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          zclDIYRuZRT_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          zclDIYRuZRT_NwkState = (devStates_t)(MSGpkt->hdr.status);

          // Теперь мы в сети
          if ( (zclDIYRuZRT_NwkState == DEV_ZB_COORD) ||
               (zclDIYRuZRT_NwkState == DEV_ROUTER)   ||
               (zclDIYRuZRT_NwkState == DEV_END_DEVICE) )
          {
            // отключаем мигание
            osal_stop_timerEx(zclDIYRuZRT_TaskID, HAL_LED_BLINK_EVENT);
            HalLedSet( HAL_LED_2, HAL_LED_MODE_OFF );
            
            // отправляем отчет
            zclDIYRuZRT_ReportOnOff();
          }
          break;

        default:
          break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // возврат необработаных сообщений
    return (events ^ SYS_EVENT_MSG);
  }

  /* Обработка событий приложения */
  
  // событие DIYRuZRT_EVT_BLINK
  if ( events & DIYRuZRT_EVT_BLINK )
  {
    // переключим светодиод
    HalLedSet( HAL_LED_2, HAL_LED_MODE_TOGGLE );
    return ( events ^ DIYRuZRT_EVT_BLINK );
  }
  // событие DIYRuZRT_EVT_LONG
  if ( events & DIYRuZRT_EVT_LONG )
  {
    // Проверяем текущее состояние устройства
    // В сети или не в сети?
    if ( bdbAttributes.bdbNodeIsOnANetwork )
    {
      // покидаем сеть
      zclDIYRuZRT_LeaveNetwork();
    }
    else 
    {
      // инициируем вход в сеть
      bdb_StartCommissioning(
        BDB_COMMISSIONING_MODE_NWK_FORMATION | 
        BDB_COMMISSIONING_MODE_NWK_STEERING | 
        BDB_COMMISSIONING_MODE_FINDING_BINDING | 
        BDB_COMMISSIONING_MODE_INITIATOR_TL
      );
      // будем мигать пока не подключимся
      osal_start_timerEx(zclDIYRuZRT_TaskID, DIYRuZRT_EVT_BLINK, 500);
    }
    
    return ( events ^ DIYRuZRT_EVT_LONG );
  }
  
  // событие DIYRuZRT_REPORTING_EVT
  if (events & DIYRuZRT_REPORTING_EVT) {
    
    zclDIYRuZRT_ReportTemp();
    
    return (events ^ DIYRuZRT_REPORTING_EVT);
  }
  
  // событие опроса кнопок
  if (events & HAL_KEY_EVENT)
  {
    /* Считывание кнопок */
    DIYRuZRT_HalKeyPoll();

    return events ^ HAL_KEY_EVENT;
  }
  
  // Отбросим необработаные сообщения
  return 0;
}


// Обработчик нажатий клавиш
static void zclDIYRuZRT_HandleKeys( byte shift, byte keys )
{
  if ( keys & HAL_KEY_SW_1 )
  {
    // Запускаем таймер для определения долгого нажания 5сек
    osal_start_timerEx(zclDIYRuZRT_TaskID, DIYRuZRT_EVT_LONG, 5000);
    // Переключаем реле
    updateRelay(RELAY_STATE == 0);
  }
  else
  {
    // Останавливаем таймер ожидания долгого нажатия
    osal_stop_timerEx(zclDIYRuZRT_TaskID, DIYRuZRT_EVT_LONG);
  }
}


// Обработчик изменения статусов соединения с сетью
static void zclDIYRuZRT_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg)
{
  switch(bdbCommissioningModeMsg->bdbCommissioningMode)
  {
    case BDB_COMMISSIONING_FORMATION:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //After formation, perform nwk steering again plus the remaining commissioning modes that has not been process yet
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
      }
      else
      {
        //Want to try other channels?
        //try with bdb_setChannelAttribute
      }
    break;
    case BDB_COMMISSIONING_NWK_STEERING:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //YOUR JOB:
        //We are on the nwk, what now?
      }
      else
      {
        //See the possible errors for nwk steering procedure
        //No suitable networks found
        //Want to try other channels?
        //try with bdb_setChannelAttribute
      }
    break;
    case BDB_COMMISSIONING_FINDING_BINDING:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //YOUR JOB:
      }
      else
      {
        //YOUR JOB:
        //retry?, wait for user interaction?
      }
    break;
    case BDB_COMMISSIONING_INITIALIZATION:
      //Initialization notification can only be successful. Failure on initialization
      //only happens for ZED and is notified as BDB_COMMISSIONING_PARENT_LOST notification

      //YOUR JOB:
      //We are on a network, what now?

    break;
#if ZG_BUILD_ENDDEVICE_TYPE    
    case BDB_COMMISSIONING_PARENT_LOST:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_NETWORK_RESTORED)
      {
        //We did recover from losing parent
      }
      else
      {
        //Parent not found, attempt to rejoin again after a fixed delay
        osal_start_timerEx(zclDIYRuZRT_TaskID, DIYRuZRT_END_DEVICE_REJOIN_EVT, DIYRuZRT_END_DEVICE_REJOIN_DELAY);
      }
    break;
#endif 
  }
}


// Обработчик изменения времени идентификации
static void zclDIYRuZRT_ProcessIdentifyTimeChange( uint8 endpoint )
{
  (void) endpoint;

  if ( zclDIYRuZRT_IdentifyTime > 0 )
  {
    //HalLedBlink ( HAL_LED_2, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    //HalLedSet ( HAL_LED_2, HAL_LED_MODE_OFF );
  }
}


// Обработчик команды сброса в Basic кластере
static void zclDIYRuZRT_BasicResetCB( void )
{
  /* TODO: remember to update this function with any
     application-specific cluster attribute variables */
  
  zclDIYRuZRT_ResetAttributesToDefaultValues();
}

/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

// Функция обработки входящих ZCL Foundation команд/ответов
static void zclDIYRuZRT_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zclDIYRuZRT_ProcessInReadRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_WRITE
    case ZCL_CMD_WRITE_RSP:
      zclDIYRuZRT_ProcessInWriteRspCmd( pInMsg );
      break;
#endif
    case ZCL_CMD_CONFIG_REPORT:
    case ZCL_CMD_CONFIG_REPORT_RSP:
    case ZCL_CMD_READ_REPORT_CFG:
    case ZCL_CMD_READ_REPORT_CFG_RSP:
    case ZCL_CMD_REPORT:
      //bdb_ProcessIncomingReportingMsg( pInMsg );
      break;
      
    case ZCL_CMD_DEFAULT_RSP:
      zclDIYRuZRT_ProcessInDefaultRspCmd( pInMsg );
      break;
#ifdef ZCL_DISCOVER
    case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
      zclDIYRuZRT_ProcessInDiscCmdsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
      zclDIYRuZRT_ProcessInDiscCmdsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_ATTRS_RSP:
      zclDIYRuZRT_ProcessInDiscAttrsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
      zclDIYRuZRT_ProcessInDiscAttrsExtRspCmd( pInMsg );
      break;
#endif
    default:
      break;
  }

  if ( pInMsg->attrCmd )
    osal_mem_free( pInMsg->attrCmd );
}

#ifdef ZCL_READ
// Обработка ответа команды Read
static uint8 zclDIYRuZRT_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    // Notify the originator of the results of the original read attributes
    // attempt and, for each successfull request, the value of the requested
    // attribute
  }

  return ( TRUE );
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
// Обработка ответа команды Write
static uint8 zclDIYRuZRT_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;

  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < writeRspCmd->numAttr; i++ )
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }

  return ( TRUE );
}
#endif // ZCL_WRITE

// Обработка ответа команды по-умолчанию
static uint8 zclDIYRuZRT_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  // Device is notified of the Default Response command.
  (void)pInMsg;

  return ( TRUE );
}

#ifdef ZCL_DISCOVER
// Обработка ответа команды Discover
static uint8 zclDIYRuZRT_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverCmdsCmdRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverCmdsCmdRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numCmd; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return ( TRUE );
}

// Обработка ответа команды Discover Attributes
static uint8 zclDIYRuZRT_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsRspCmd_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return ( TRUE );
}

// Обработка ответа команды Discover Attributes Ext
static uint8 zclDIYRuZRT_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsExtRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsExtRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return ( TRUE );
}
#endif // ZCL_DISCOVER


// Инициализация работы кнопок (входов)
void DIYRuZRT_HalKeyInit( void )
{
  /* Сбрасываем сохраняемое состояние кнопок в 0 */
  halKeySavedKeys = 0;

  PUSH1_SEL &= ~(PUSH1_BV); /* Выставляем функцию пина - GPIO */
  PUSH1_DIR &= ~(PUSH1_BV); /* Выставляем режим пина - Вход */
  
  PUSH1_ICTL &= ~(PUSH1_ICTLBIT); /* Не генерируем прерывания на пине */
  PUSH1_IEN &= ~(PUSH1_IENBIT);   /* Очищаем признак включения прерываний */
  
  PUSH2_SEL &= ~(PUSH2_BV); /* Set pin function to GPIO */
  PUSH2_DIR &= ~(PUSH2_BV); /* Set pin direction to Input */
  
  PUSH2_ICTL &= ~(PUSH2_ICTLBIT); /* don't generate interrupt */
  PUSH2_IEN &= ~(PUSH2_IENBIT);   /* Clear interrupt enable bit */
}

// Считывание кнопок
void DIYRuZRT_HalKeyPoll (void)
{
  uint8 keys = 0;

  // нажата кнопка 1 ?
  if (HAL_PUSH_BUTTON1())
  {
    keys |= HAL_KEY_SW_1;
  }
  
  // нажата кнопка 2 ?
  if (HAL_PUSH_BUTTON2())
  {
    keys |= HAL_KEY_SW_2;
  }
  
  if (keys == halKeySavedKeys)
  {
    // Выход - нет изменений
    return;
  }
  // Сохраним текущее состояние кнопок для сравнения в след раз
  halKeySavedKeys = keys;

  // Вызовем генерацию события изменений кнопок
  OnBoard_SendKeys(keys, HAL_KEY_STATE_NORMAL);
}

// Изменение состояния реле
void updateRelay ( bool value )
{
  if (value) {
    RELAY_STATE = 1;
  } else {
    RELAY_STATE = 0;
  }
  // сохраняем состояние реле
  osal_nv_write(NV_DIYRuZRT_RELAY_STATE_ID, 0, 1, &RELAY_STATE);
  // Отображаем новое состояние
  applyRelay();
  // отправляем отчет
  zclDIYRuZRT_ReportOnOff();
}
  
// Применение состояние реле
void applyRelay ( void )
{
  // если выключено
  if (RELAY_STATE == 0) {
    // то гасим светодиод 1
    HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
  } else {
    // иначе включаем светодиод 1
    HalLedSet ( HAL_LED_1, HAL_LED_MODE_ON );
  }
}


// Инициализация выхода из сети
void zclDIYRuZRT_LeaveNetwork( void )
{
  zclDIYRuZRT_ResetAttributesToDefaultValues();
  
  NLME_LeaveReq_t leaveReq;
  // Set every field to 0
  osal_memset(&leaveReq, 0, sizeof(NLME_LeaveReq_t));

  // This will enable the device to rejoin the network after reset.
  leaveReq.rejoin = FALSE;

  // Set the NV startup option to force a "new" join.
  zgWriteStartupOptions(ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE);

  // Leave the network, and reset afterwards
  if (NLME_LeaveReq(&leaveReq) != ZSuccess) {
    // Couldn't send out leave; prepare to reset anyway
    ZDApp_LeaveReset(FALSE);
  }
}

// Обработчик команд кластера OnOff
static void zclDIYRuZRT_OnOffCB(uint8 cmd)
{
  // запомним адрес откуда пришла команда
  // чтобы отправить обратно отчет
  afIncomingMSGPacket_t *pPtr = zcl_getRawAFMsg();
  zclDIYRuZRT_DstAddr.addr.shortAddr = pPtr->srcAddr.addr.shortAddr;
  
  // Включить
  if (cmd == COMMAND_ON) {
    updateRelay(TRUE);
  }
  // Выключить
  else if (cmd == COMMAND_OFF) {
    updateRelay(FALSE);
  }
  // Переключить
  else if (cmd == COMMAND_TOGGLE) {
    updateRelay(RELAY_STATE == 0);
  }
}

// Информирование о состоянии реле
void zclDIYRuZRT_ReportOnOff(void) {
  const uint8 NUM_ATTRIBUTES = 1;

  zclReportCmd_t *pReportCmd;

  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) +
                              (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;

    pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[0].attrData = (void *)(&RELAY_STATE);

    zclDIYRuZRT_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    zclDIYRuZRT_DstAddr.addr.shortAddr = 0;
    zclDIYRuZRT_DstAddr.endPoint = 1;

    zcl_SendReportCmd(DIYRuZRT_ENDPOINT, &zclDIYRuZRT_DstAddr,
                      ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd,
                      ZCL_FRAME_SERVER_CLIENT_DIR, false, SeqNum++);
  }

  osal_mem_free(pReportCmd);
}

// Информирование о температуре
void zclDIYRuZRT_ReportTemp( void )
{
  // читаем температуру
  zclDIYRuZRT_MeasuredValue = readTemperature();
  
  const uint8 NUM_ATTRIBUTES = 1;

  zclReportCmd_t *pReportCmd;

  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) +
                              (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;

    pReportCmd->attrList[0].attrID = ATTRID_MS_TEMPERATURE_MEASURED_VALUE;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_INT16;
    pReportCmd->attrList[0].attrData = (void *)(&zclDIYRuZRT_MeasuredValue);

    zclDIYRuZRT_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    zclDIYRuZRT_DstAddr.addr.shortAddr = 0;
    zclDIYRuZRT_DstAddr.endPoint = 1;

    zcl_SendReportCmd(DIYRuZRT_ENDPOINT, &zclDIYRuZRT_DstAddr,
                      ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, pReportCmd,
                      ZCL_FRAME_SERVER_CLIENT_DIR, false, SeqNum++);
  }

  osal_mem_free(pReportCmd);
}
