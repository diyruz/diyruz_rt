#ifndef ZCL_DIYRuZRT_H
#define ZCL_DIYRuZRT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "zcl.h"

// Номер эндпоинта устройства
#define DIYRuZRT_ENDPOINT            1

// События приложения
#define DIYRuZRT_EVT_BLINK                0x0001
#define DIYRuZRT_EVT_LONG                 0x0002
#define DIYRuZRT_END_DEVICE_REJOIN_EVT    0x0004
#define DIYRuZRT_REPORTING_EVT            0x0008
  
  
// NVM IDs
#define NV_DIYRuZRT_RELAY_STATE_ID        0x0402

extern SimpleDescriptionFormat_t zclDIYRuZRT_SimpleDesc;

extern CONST zclCommandRec_t zclDIYRuZRT_Cmds[];

extern CONST uint8 zclCmdsArraySize;

// Список атрибутов
extern CONST zclAttrRec_t zclDIYRuZRT_Attrs[];
extern CONST uint8 zclDIYRuZRT_NumAttributes;

// Атрибуты идентификации
extern uint16 zclDIYRuZRT_IdentifyTime;
extern uint8  zclDIYRuZRT_IdentifyCommissionState;

// TODO: Declare application specific attributes here

// Инициализация задачи
extern void zclDIYRuZRT_Init( byte task_id );

// Обработчик сообщений задачи
extern UINT16 zclDIYRuZRT_event_loop( byte task_id, UINT16 events );

// Сброс всех атрибутов в начальное состояние
extern void zclDIYRuZRT_ResetAttributesToDefaultValues(void);

// Функции работы с кнопками
extern void DIYRuZRT_HalKeyInit( void );
extern void DIYRuZRT_HalKeyPoll ( void );

// Функции команд управления
static void zclDIYRuZRT_OnOffCB(uint8);

#ifdef __cplusplus
}
#endif

#endif /* ZCL_DIYRuZRT_H */