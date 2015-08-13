#ifndef ZCL_ROUTERVERSION1_H
#define ZCL_ROUTERVERSION1_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "zcl.h"

/*********************************************************************
 * CONSTANTS
 */
#define ROUTERVERSION1_ENDPOINT            8

#define LIGHT_OFF                       0x00
#define LIGHT_ON                        0x01

// Application Events
#define ROUTERVERSION1_IDENTIFY_TIMEOUT_EVT     0x0001
#define ROUTERVERSION1_POLL_CONTROL_TIMEOUT_EVT 0x0002
#define ROUTERVERSION1_EZMODE_TIMEOUT_EVT       0x0004
#define ROUTERVERSION1_EZMODE_NEXTSTATE_EVT     0x0008
#define ROUTERVERSION1_MAIN_SCREEN_EVT          0x0010
#define ROUTERVERSION1_LEVEL_CTRL_EVT           0x0020
#define ROUTERVERSION1_START_EZMODE_EVT         0x0040  

// Application Display Modes
#define LIGHT_MAINMODE      0x00
#define LIGHT_HELPMODE      0x01

/*********************************************************************
 * MACROS
 */
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
extern SimpleDescriptionFormat_t zclRouterVersion1_SimpleDesc;

extern CONST zclCommandRec_t zclRouterVersion1_Cmds[];

extern CONST uint8 zclCmdsArraySize;

// attribute list
extern CONST zclAttrRec_t zclRouterVersion1_Attrs[];
extern CONST uint8 zclRouterVersion1_NumAttributes;

// Identify attributes
extern uint16 zclRouterVersion1_IdentifyTime;
extern uint8  zclRouterVersion1_IdentifyCommissionState;

// OnOff attributes
extern uint8  zclRouterVersion1_OnOff;

// Level Control Attributes

/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void zclRouterVersion1_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 zclRouterVersion1_event_loop( byte task_id, UINT16 events );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZCL_ROUTERVERSION1_H */
