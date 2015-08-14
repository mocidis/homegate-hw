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
#include "zcl_ezmode.h"
#include "zcl_diagnostic.h"

#include "zcl_RouterVersion1.h"

#include "onboard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "NLMEDE.h"


/*********************************************************************
 * TYPEDEFS
 */
#define HAL_IO_SET(port, pin, val) HAL_IO_SET_PREP(port, pin, val);
#define HAL_IO_SET_PREP(port, pin, val) st( P##port##_##pin## = val); 

#define HAL_CONFIG_IO_OUTPUT(port, pin, val)      HAL_CONFIG_IO_OUTPUT_PREP(port, pin, val)
#define HAL_CONFIG_IO_OUTPUT_PREP(port, pin, val) st( P##port##SEL &= ~BV(pin); \
                                                      P##port##_##pin## = val; \
                                                      P##port##DIR |= BV(pin); )
#define CHECK_INPUT_BUTTON   0x0080
#define SET_PARAMETER        0x0100
/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclRouterVersion1_TaskID;
uint8 zclRouterVersion1SeqNum;
uint8 button1 = 0;
uint8 button2 = 0;
uint8 button3 = 0;
uint8 button_join = 0;
uint8 button_reset = 0;
/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
afAddrType_t zclRouterVersion1_DstAddr;
// Test Endpoint to allow SYS_APP_MSGs
static endPointDesc_t sampleLight_TestEp =
{
  ROUTERVERSION1_ENDPOINT,
  &zclRouterVersion1_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

uint8 giLightScreenMode = LIGHT_MAINMODE;   // display the main screen mode first

uint8 gPermitDuration = 0;    // permit joining default to disabled

devStates_t zclRouterVersion1_NwkState = DEV_INIT;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclRouterVersion1_BasicResetCB( void );
static void zclRouterVersion1_IdentifyCB( zclIdentify_t *pCmd );
static void zclRouterVersion1_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void zclRouterVersion1_OnOffCB( uint8 cmd );
// app display functions
// Functions to process ZCL Foundation incoming Command/Response messages
static void zclRouterVersion1_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static uint8 zclRouterVersion1_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zclRouterVersion1_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclRouterVersion1_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclRouterVersion1_CmdCallbacks =
{
  zclRouterVersion1_BasicResetCB,            // Basic Cluster Reset command
  zclRouterVersion1_IdentifyCB,              // Identify command
#ifdef ZCL_EZMODE
  NULL,                                   // Identify EZ-Mode Invoke command
  NULL,                                   // Identify Update Commission State command
#endif
  NULL,                                   // Identify Trigger Effect command
  zclRouterVersion1_IdentifyQueryRspCB,      // Identify Query Response command
  zclRouterVersion1_OnOffCB,                 // On/Off cluster commands
  NULL,                                   // On/Off cluster enhanced command Off with Effect
  NULL,                                   // On/Off cluster enhanced command On with Recall Global Scene
  NULL,                                   // On/Off cluster enhanced command On with Timed Off
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
 * @fn          zclRouterVersion1_Init
 *
 * @brief       Initialization function for the zclGeneral layer.
 *
 * @param       none
 *
 * @return      none
 */
void zclRouterVersion1_Init( byte task_id )
{
  zclRouterVersion1_TaskID = task_id;

  // Set destination address to indirect
  zclRouterVersion1_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  zclRouterVersion1_DstAddr.endPoint = 2;
  zclRouterVersion1_DstAddr.addr.shortAddr = 0x0000;

  // This app is part of the Home Automation Profile
  zclHA_Init( &zclRouterVersion1_SimpleDesc );

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( ROUTERVERSION1_ENDPOINT, &zclRouterVersion1_CmdCallbacks );
  // Register the application's attribute list
  zcl_registerAttrList( ROUTERVERSION1_ENDPOINT, zclRouterVersion1_NumAttributes, zclRouterVersion1_Attrs );

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( zclRouterVersion1_TaskID );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( zclRouterVersion1_TaskID );

  // Register for a test endpoint
  afRegister( &sampleLight_TestEp );

  ZDO_RegisterForZDOMsg(task_id, Device_annce);

  osal_start_timerEx(zclRouterVersion1_TaskID, SET_PARAMETER, 1000);
  HAL_CONFIG_IO_OUTPUT(1,2,0);
  HAL_CONFIG_IO_OUTPUT(1,3,0);
  HAL_CONFIG_IO_OUTPUT(0,0,0);
  HAL_CONFIG_IO_OUTPUT(0,1,0);
  HAL_CONFIG_IO_OUTPUT(0,2,0);
  HAL_CONFIG_IO_OUTPUT(0,3,0);
  HAL_CONFIG_IO_OUTPUT(0,4,0);
  HAL_CONFIG_IO_OUTPUT(0,7,0);
  HalLedSet(HAL_LED_1,HAL_LED_MODE_OFF);
  HalLedSet(HAL_LED_2,HAL_LED_MODE_OFF);
  //ZDOInitDevice(0);
   if (ZDApp_ReadNetworkRestoreState() == 0) {
      ZDOInitDevice(0);
    }
}

/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for zclGeneral.
 *
 * @param       none
 *
 * @return      none
 */
uint16 zclRouterVersion1_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
   while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclRouterVersion1_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          zclRouterVersion1_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
          break;      
        case KEY_CHANGE:
          //zclRouterVersion1_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          zclRouterVersion1_NwkState = (devStates_t)(MSGpkt->hdr.status);

          // now on the network
          if ( (zclRouterVersion1_NwkState == DEV_ZB_COORD) ||
               (zclRouterVersion1_NwkState == DEV_ROUTER)   ||
               (zclRouterVersion1_NwkState == DEV_END_DEVICE) )
          {
            
            HalLedSet(HAL_LED_1,HAL_LED_MODE_ON);
          }
          break;

        default:
          {
            
          }
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  
  if ( events & SET_PARAMETER ) {
    button1 = P0_2;
    button2 = P0_3;
    button3 = P0_4;
    return ( events = CHECK_INPUT_BUTTON);
  }
  
  if ( events & CHECK_INPUT_BUTTON ) {
    if (button1 ^ P0_2 ) {
      HalLedSet(HAL_LED_2,HAL_LED_MODE_TOGGLE);
      button1 = P0_2 ;
    }
    if (button2 ^ P0_3 ) {
      if(P1_2 == 0) {
        P1_2 = 1;
        //P0_1 = 1;
      } else {
        P1_2 = 0;
        //P0_1 = 0;
      }
      button2 = P0_3 ;
    }
    if (button3 ^ P0_4 ) {
      if(P1_3 == 0) {
        P1_3 = 1;
        //P0_1 = 1;
      } else {
        P1_3 = 0;
        //P0_1 = 0;
      }
      button3 = P0_4 ;
    }
    
    if (P0_1) {     
      ZDOInitDevice(0);
    }
      
    if ( P0_0) {
      zclRouterVersion1_BasicResetCB();
    }
    
    osal_start_timerEx(zclRouterVersion1_TaskID, CHECK_INPUT_BUTTON, 400);
    return ( events ^ CHECK_INPUT_BUTTON );
  }
  
  return 0;
}

/*********************************************************************
 * @fn      zclRouterVersion1_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   none
 *
 * @return  none
 */
static void zclRouterVersion1_ProcessIdentifyTimeChange( void )
{
  if ( zclRouterVersion1_IdentifyTime > 0 )
  {
    osal_start_timerEx( zclRouterVersion1_TaskID, ROUTERVERSION1_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    osal_stop_timerEx( zclRouterVersion1_TaskID, ROUTERVERSION1_IDENTIFY_TIMEOUT_EVT );
  }
}

/*********************************************************************
 * @fn      zclRouterVersion1_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zclRouterVersion1_BasicResetCB( void )
{
  NLME_LeaveReq_t leaveReq;
  // Set every field to 0
  osal_memset( &leaveReq, 0, sizeof( NLME_LeaveReq_t ) );

  // This will enable the device to rejoin the network after reset.
  leaveReq.rejoin = TRUE;

  // Set the NV startup option to force a "new" join.
  zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );

  // Leave the network, and reset afterwards
  if ( NLME_LeaveReq( &leaveReq ) != ZSuccess )
  {
    // Couldn't send out leave; prepare to reset anyway
    ZDApp_LeaveReset( FALSE );
  }
}

/*********************************************************************
 * @fn      zclRouterVersion1_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void zclRouterVersion1_IdentifyCB( zclIdentify_t *pCmd )
{
  zclRouterVersion1_IdentifyTime = pCmd->identifyTime;
  zclRouterVersion1_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      zclRouterVersion1_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   srcAddr - requestor's address
 * @param   timeout - number of seconds to identify yourself (valid for query response)
 *
 * @return  none
 */
static void zclRouterVersion1_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
{
  (void)pRsp;
}

/*********************************************************************
 * @fn      zclRouterVersion1_OnOffCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an On/Off Command for this application.
 *
 * @param   cmd - COMMAND_ON, COMMAND_OFF or COMMAND_TOGGLE
 *
 * @return  none
 */
static void zclRouterVersion1_OnOffCB( uint8 cmd )
{
  afIncomingMSGPacket_t *pPtr = zcl_getRawAFMsg();

  zclRouterVersion1_DstAddr.addr.shortAddr = pPtr->srcAddr.addr.shortAddr;
  uint8 command_data[] = {0};
  //Data = Data << 8;
  osal_memcpy( (void *)command_data, (const void *) pPtr->cmd.Data, 4);

  // Turn on the light
  if ( cmd == COMMAND_ON )
  {
    if (command_data[3] == 1) {
    HalLedSet(HAL_LED_2,HAL_LED_MODE_ON);
    }
    if (command_data[3] == 3) {
    P1_2 = 1;
    }
    if (command_data[3] == 6) {
    P1_3 = 1;
    }
  }
  // Turn off the light
  else if ( cmd == COMMAND_OFF )
  {
    if (command_data[3] == 1) {
    HalLedSet(HAL_LED_2,HAL_LED_MODE_OFF);
    }
    if (command_data[3] == 3) {
    P1_2 = 0;
    }
    if (command_data[3] == 6) {
    P1_3 =0;
    }
  }
  // Toggle the light
  else if ( cmd == COMMAND_TOGGLE )
  {
    
    if (command_data[3] == 1) {
    HalLedSet(HAL_LED_2,HAL_LED_MODE_TOGGLE);
    }
    
   if (command_data[3] == 3 ) {
      if(P1_2 == 0) {
        P1_2 = 1;
      } else {
        P1_2 = 0;
      }
    }
    
    if (command_data[3] == 6) {
      if(P1_3 == 0) {
        P1_3 = 1;
      } else {
        P1_3 = 0;
      }
  }
  }
}
  
/*********************************************************************
 * @fn      zclRouterVersion1_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zclRouterVersion1_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zclRouterVersion1_ProcessInReadRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_WRITE
    case ZCL_CMD_WRITE_RSP:
      zclRouterVersion1_ProcessInWriteRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_REPORT
    // Attribute Reporting implementation should be added here
    case ZCL_CMD_CONFIG_REPORT:
      // zclRouterVersion1_ProcessInConfigReportCmd( pInMsg );
      break;

    case ZCL_CMD_CONFIG_REPORT_RSP:
      // zclRouterVersion1_ProcessInConfigReportRspCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG:
      // zclRouterVersion1_ProcessInReadReportCfgCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG_RSP:
      // zclRouterVersion1_ProcessInReadReportCfgRspCmd( pInMsg );
      break;

    case ZCL_CMD_REPORT:
      // zclRouterVersion1_ProcessInReportCmd( pInMsg );
      break;
#endif
    case ZCL_CMD_DEFAULT_RSP:
      zclRouterVersion1_ProcessInDefaultRspCmd( pInMsg );
      break;
#ifdef ZCL_DISCOVER
    case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
      zclRouterVersion1_ProcessInDiscCmdsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
      zclRouterVersion1_ProcessInDiscCmdsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_ATTRS_RSP:
      zclRouterVersion1_ProcessInDiscAttrsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
      zclRouterVersion1_ProcessInDiscAttrsExtRspCmd( pInMsg );
      break;
#endif
    default:
      break;
  }

  if ( pInMsg->attrCmd )
    osal_mem_free( pInMsg->attrCmd );
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      zclRouterVersion1_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRouterVersion1_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
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
/*********************************************************************
 * @fn      zclRouterVersion1_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRouterVersion1_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
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

/*********************************************************************
 * @fn      zclRouterVersion1_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRouterVersion1_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  // Device is notified of the Default Response command.
  (void)pInMsg;

  return ( TRUE );
}




