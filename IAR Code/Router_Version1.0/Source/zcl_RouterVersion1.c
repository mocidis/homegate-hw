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

#if ( defined (ZGP_DEVICE_TARGET) || defined (ZGP_DEVICE_TARGETPLUS) \
      || defined (ZGP_DEVICE_COMBO) || defined (ZGP_DEVICE_COMBO_MIN) )
#include "zgp_translationtable.h"
  #if (SUPPORTED_S_FEATURE(SUPP_ZGP_FEATURE_TRANSLATION_TABLE))
    #define ZGP_AUTO_TT
  #endif
#endif
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
#define CHECK_INPUT_BUTTON 0x0080
/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclRouterVersion1_TaskID;
uint8 zclRouterVersion1SeqNum;
uint8 button1 = 0;
uint8 button2 = 0;
uint8 button3 = 0;
/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
afAddrType_t zclRouterVersion1_DstAddr;

#ifdef ZCL_EZMODE
static void zclRouterVersion1_ProcessZDOMsgs( zdoIncomingMsg_t *pMsg );
static void zclRouterVersion1_EZModeCB( zlcEZMode_State_t state, zclEZMode_CBData_t *pData );


// register EZ-Mode with task information (timeout events, callback, etc...)
static const zclEZMode_RegisterData_t zclRouterVersion1_RegisterEZModeData =
{
  &zclRouterVersion1_TaskID,
  ROUTERVERSION1_EZMODE_NEXTSTATE_EVT,
  ROUTERVERSION1_EZMODE_TIMEOUT_EVT,
  &zclRouterVersion1SeqNum,
  zclRouterVersion1_EZModeCB
};

#else
uint16 bindingInClusters[] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF
};
#define ZCLROUTERVERSION1_BINDINGLIST (sizeof(bindingInClusters) / sizeof(bindingInClusters[0]))

#endif  // ZCL_EZMODE

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
static void zclRouterVersion1_ProcessIdentifyTimeChange( void );
// app display functions
static void zclRouterVersion1_LcdDisplayUpdate( void );
#ifdef LCD_SUPPORTED
static void zclRouterVersion1_LcdDisplayMainMode( void );
static void zclRouterVersion1_LcdDisplayHelpMode( void );
#endif
static void zclRouterVersion1_DisplayLight( void );

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclRouterVersion1_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static uint8 zclRouterVersion1_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zclRouterVersion1_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclRouterVersion1_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zclRouterVersion1_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclRouterVersion1_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclRouterVersion1_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg );
#endif

/*********************************************************************
 * STATUS STRINGS
 */
#ifdef LCD_SUPPORTED
const char sDeviceName[]   = "  Sample Light";
const char sClearLine[]    = " ";
const char sSwLight[]      = "SW1: ToggleLight";  // 16 chars max
const char sSwEZMode[]     = "SW2: EZ-Mode";
char sSwHelp[]             = "SW5: Help       ";  // last character is * if NWK open
const char sLightOn[]      = "    LIGHT ON ";
const char sLightOff[]     = "    LIGHT OFF";
 #if ZCL_LEVEL_CTRL
 char sLightLevel[]        = "    LEVEL ###"; // displays level 1-254
 #endif
#endif

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

#ifdef ZCL_DISCOVER
  // Register the application's command list
  zcl_registerCmdList( ROUTERVERSION1_ENDPOINT, zclCmdsArraySize, zclRouterVersion1_Cmds );
#endif

  // Register for all key events - This app will handle all key events
  RegisterForKeys( zclRouterVersion1_TaskID );

  // Register for a test endpoint
  afRegister( &sampleLight_TestEp );

#ifdef ZCL_EZMODE
  // Register EZ-Mode
  zcl_RegisterEZMode( &zclRouterVersion1_RegisterEZModeData );

  // Register with the ZDO to receive Match Descriptor Responses
  ZDO_RegisterForZDOMsg(task_id, Match_Desc_rsp);
#endif
  ZDO_RegisterForZDOMsg(task_id, Device_annce);
#ifdef ZCL_DIAGNOSTIC
  // Register the application's callback function to read/write attribute data.
  // This is only required when the attribute data format is unknown to ZCL.
  zcl_registerReadWriteCB( ROUTERVERSION1_ENDPOINT, zclDiagnostic_ReadWriteAttrCB, NULL );

  if ( zclDiagnostic_InitStats() == ZSuccess )
  {
    // Here the user could start the timer to save Diagnostics to NV
  }
#endif

#ifdef LCD_SUPPORTED
  HalLcdWriteString ( (char *)sDeviceName, HAL_LCD_LINE_3 );
#endif  // LCD_SUPPORTED

#ifdef ZGP_AUTO_TT
  zgpTranslationTable_RegisterEP ( &zclRouterVersion1_SimpleDesc );
#endif
  osal_start_timerEx(zclRouterVersion1_TaskID, CHECK_INPUT_BUTTON, 2000);
  HAL_CONFIG_IO_OUTPUT(0,4,0);
  HAL_CONFIG_IO_OUTPUT(0,5,0);
  HAL_CONFIG_IO_OUTPUT(0,6,0);
  HAL_CONFIG_IO_OUTPUT(0,7,0);
  HalLedSet(HAL_LED_1,HAL_LED_MODE_OFF);
  HalLedSet(HAL_LED_2,HAL_LED_MODE_OFF);
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
#ifdef ZCL_EZMODE
        case ZDO_CB_MSG:
          zclRouterVersion1_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;
#endif
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
            giLightScreenMode = LIGHT_MAINMODE;
            zclRouterVersion1_LcdDisplayUpdate();
#ifdef ZCL_EZMODE
            zcl_EZModeAction( EZMODE_ACTION_NETWORK_STARTED, NULL );
#endif // ZCL_EZMODE
          }
          break;

        default:
          {
            //
          
            //
          }
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & ROUTERVERSION1_IDENTIFY_TIMEOUT_EVT )
  {
    if ( zclRouterVersion1_IdentifyTime > 0 )
      zclRouterVersion1_IdentifyTime--;
    zclRouterVersion1_ProcessIdentifyTimeChange();

    return ( events ^ ROUTERVERSION1_IDENTIFY_TIMEOUT_EVT );
  }

  if ( events & ROUTERVERSION1_MAIN_SCREEN_EVT )
  {
    giLightScreenMode = LIGHT_MAINMODE;
    zclRouterVersion1_LcdDisplayUpdate();

    return ( events ^ ROUTERVERSION1_MAIN_SCREEN_EVT );
  }

#ifdef ZCL_EZMODE
  // going on to next state
  if ( events & ROUTERVERSION1_EZMODE_NEXTSTATE_EVT )
  {
    zcl_EZModeAction ( EZMODE_ACTION_PROCESS, NULL );   // going on to next state
    return ( events ^ ROUTERVERSION1_EZMODE_NEXTSTATE_EVT );
  }

  // the overall EZMode timer expired, so we timed out
  if ( events & ROUTERVERSION1_EZMODE_TIMEOUT_EVT )
  {
    zcl_EZModeAction ( EZMODE_ACTION_TIMED_OUT, NULL ); // EZ-Mode timed out
    return ( events ^ ROUTERVERSION1_EZMODE_TIMEOUT_EVT );
  }
#endif // ZLC_EZMODE
  // Discard unknown events
  if ( events & CHECK_INPUT_BUTTON ) {
    if (button1 ^ P0_4 ) {
      HalLedSet(HAL_LED_1,HAL_LED_MODE_TOGGLE);
      button1 = P0_4 ;
    }
    if (button2 ^ P0_5 ) {
      HalLedSet(HAL_LED_2,HAL_LED_MODE_TOGGLE);
      button2 = P0_5 ;
    }
    if (button3 ^ P0_6 ) {
      if(P0_7 == 0) {
        P0_7 = 1;
      } else {
        P0_7 = 0;
      }
      button3 = P0_6 ;
    }
    osal_start_reload_timer(zclRouterVersion1_TaskID, CHECK_INPUT_BUTTON, 500);
    return ( events ^ CHECK_INPUT_BUTTON );
  }
  return 0;
}

/*********************************************************************
 * @fn      zclRouterVersion1_LcdDisplayUpdate
 *
 * @brief   Called to update the LCD display.
 *
 * @param   none
 *
 * @return  none
 */
void zclRouterVersion1_LcdDisplayUpdate( void )
{
#ifdef LCD_SUPPORTED
  if ( giLightScreenMode == LIGHT_HELPMODE )
  {
    zclRouterVersion1_LcdDisplayHelpMode();
  }
  else
  {
    zclRouterVersion1_LcdDisplayMainMode();
  }
#endif

  zclRouterVersion1_DisplayLight();
}

/*********************************************************************
 * @fn      zclRouterVersion1_DisplayLight
 *
 * @brief   Displays current state of light on LED and also on main display if supported.
 *
 * @param   none
 *
 * @return  none
 */
static void zclRouterVersion1_DisplayLight( void )
{
  // set the LED1 based on light (on or off)
  if ( zclRouterVersion1_OnOff == LIGHT_ON )
  {
    HalLedSet ( HAL_LED_1, HAL_LED_MODE_ON );
  }
  else
  {
    HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
  }
}

#ifdef LCD_SUPPORTED
/*********************************************************************
 * @fn      zclRouterVersion1_LcdDisplayMainMode
 *
 * @brief   Called to display the main screen on the LCD.
 *
 * @param   none
 *
 * @return  none
 */
static void zclRouterVersion1_LcdDisplayMainMode( void )
{
  // display line 1 to indicate NWK status
  if ( zclRouterVersion1_NwkState == DEV_ZB_COORD )
  {
    zclHA_LcdStatusLine1( ZCL_HA_STATUSLINE_ZC );
  }
  else if ( zclRouterVersion1_NwkState == DEV_ROUTER )
  {
    zclHA_LcdStatusLine1( ZCL_HA_STATUSLINE_ZR );
  }
  else if ( zclRouterVersion1_NwkState == DEV_END_DEVICE )
  {
    zclHA_LcdStatusLine1( ZCL_HA_STATUSLINE_ZED );
  }

  // end of line 3 displays permit join status (*)
  if ( gPermitDuration )
  {
    sSwHelp[15] = '*';
  }
  else
  {
    sSwHelp[15] = ' ';
  }
  HalLcdWriteString( (char *)sSwHelp, HAL_LCD_LINE_3 );
}

/*********************************************************************
 * @fn      zclRouterVersion1_LcdDisplayHelpMode
 *
 * @brief   Called to display the SW options on the LCD.
 *
 * @param   none
 *
 * @return  none
 */
static void zclRouterVersion1_LcdDisplayHelpMode( void )
{
  HalLcdWriteString( (char *)sSwLight, HAL_LCD_LINE_1 );
  HalLcdWriteString( (char *)sSwEZMode, HAL_LCD_LINE_2 );
  HalLcdWriteString( (char *)sSwHelp, HAL_LCD_LINE_3 );
}
#endif  // LCD_SUPPORTED

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
#ifdef ZCL_EZMODE
    if ( zclRouterVersion1_IdentifyCommissionState & EZMODE_COMMISSION_OPERATIONAL )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_ON );
    }
    else
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    }
#endif

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
#ifdef ZCL_EZMODE
  {
    zclEZMode_ActionData_t data;
    data.pIdentifyQueryRsp = pRsp;
    zcl_EZModeAction ( EZMODE_ACTION_IDENTIFY_QUERY_RSP, &data );
  }
#endif
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
    HalLedSet(HAL_LED_1,HAL_LED_MODE_ON);
    }
    if (command_data[3] == 3) {
    HalLedSet(HAL_LED_2,HAL_LED_MODE_ON);
    }
    if (command_data[3] == 6) {
    P0_7 =1;
    }
  }
  // Turn off the light
  else if ( cmd == COMMAND_OFF )
  {
    if (command_data[3] == 1) {
    HalLedSet(HAL_LED_1,HAL_LED_MODE_OFF);
    }
    if (command_data[3] == 3) {
    HalLedSet(HAL_LED_2,HAL_LED_MODE_OFF);
    }
    if (command_data[3] == 6) {
    P0_7 =0;
    }
  }
  // Toggle the light
  else if ( cmd == COMMAND_TOGGLE )
  {
    
    if (command_data[3] == 1) {
    HalLedSet(HAL_LED_1,HAL_LED_MODE_TOGGLE);
    }
    
    if (command_data[3] == 3) {
    HalLedSet(HAL_LED_2,HAL_LED_MODE_TOGGLE);
    }
    
    if (command_data[3] == 6) {
      if(P0_7 == 0) {
        P0_7 = 1;
      } else {
        P0_7 = 0;
      }
  }
  }
  
  //osal_memcpy((void *)&Data,(const void *)MSGpkt->cmd.Data, MSGpkt->cmd.DataLength);
  /*
  if ( cmd == COMMAND_ON )
  { 
    if (Data == 0x01) {
    HalLedSet(HAL_LED_1,HAL_LED_MODE_ON);
    }
    if (Data == 0x02) {
    HalLedSet(HAL_LED_2,HAL_LED_MODE_ON);
    }
    if (Data == 0x03) {
    P0_7 =1;
    }
  }
  // Turn off the light
  if ( cmd == COMMAND_OFF)
  {
    if (Data == 0x01) {
    HalLedSet(HAL_LED_1,HAL_LED_MODE_OFF);
    }
    if (Data == 0x02) {
    HalLedSet(HAL_LED_2,HAL_LED_MODE_OFF);
    }
    if (Data == 0x03) {
    P0_7 = 0;
    }
  }
  
  
    
    if (Data == 0) {
    HalLedSet(HAL_LED_1,HAL_LED_MODE_TOGGLE);
    }
    
    if (Data == 1) {
    HalLedSet(HAL_LED_2,HAL_LED_MODE_TOGGLE);
    }
    
    if (Data == 2) {
      if(P0_7 == 0) {
        P0_7 = 1;
      } else {
        P0_7 = 0;
      }
    }
   */
  }
  

/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

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

#ifdef ZCL_DISCOVER
/*********************************************************************
 * @fn      zclRouterVersion1_ProcessInDiscCmdsRspCmd
 *
 * @brief   Process the Discover Commands Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRouterVersion1_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg )
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

/*********************************************************************
 * @fn      zclRouterVersion1_ProcessInDiscAttrsRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRouterVersion1_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg )
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

/*********************************************************************
 * @fn      zclRouterVersion1_ProcessInDiscAttrsExtRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Extended Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRouterVersion1_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg )
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

#if ZCL_EZMODE
/*********************************************************************
 * @fn      zclRouterVersion1_ProcessZDOMsgs
 *
 * @brief   Called when this node receives a ZDO/ZDP response.
 *
 * @param   none
 *
 * @return  status
 */
static void zclRouterVersion1_ProcessZDOMsgs( zdoIncomingMsg_t *pMsg )
{
  zclEZMode_ActionData_t data;
  ZDO_MatchDescRsp_t *pMatchDescRsp;

  // Let EZ-Mode know of the Simple Descriptor Response
  if ( pMsg->clusterID == Match_Desc_rsp )
  {
    pMatchDescRsp = ZDO_ParseEPListRsp( pMsg );
    data.pMatchDescRsp = pMatchDescRsp;
    zcl_EZModeAction( EZMODE_ACTION_MATCH_DESC_RSP, &data );
    osal_mem_free( pMatchDescRsp );
  }
  if (pMsg->clusterID == Device_annce) {
    ZDO_DeviceAnnce_t msg;
    
    //parse message
    ZDO_ParseDeviceAnnce(pMsg , &msg);
    
    HalLcdWriteStringValue("Child:", msg.nwkAddr,16,HAL_LCD_LINE_2);
    
  }
  
}

/*********************************************************************
 * @fn      zclRouterVersion1_EZModeCB
 *
 * @brief   The Application is informed of events. This can be used to show on the UI what is
*           going on during EZ-Mode steering/finding/binding.
 *
 * @param   state - an
 *
 * @return  none
 */
static void zclRouterVersion1_EZModeCB( zlcEZMode_State_t state, zclEZMode_CBData_t *pData )
{
#ifdef LCD_SUPPORTED
  char *pStr;
  uint8 err;
#endif

  // time to go into identify mode
  if ( state == EZMODE_STATE_IDENTIFYING )
  {
#ifdef LCD_SUPPORTED
    HalLcdWriteString( "EZMode", HAL_LCD_LINE_2 );
#endif

    zclRouterVersion1_IdentifyTime = ( EZMODE_TIME / 1000 );  // convert to seconds
    zclRouterVersion1_ProcessIdentifyTimeChange();
  }

  // autoclosing, show what happened (success, cancelled, etc...)
  if( state == EZMODE_STATE_AUTOCLOSE )
  {
#ifdef LCD_SUPPORTED
    pStr = NULL;
    err = pData->sAutoClose.err;
    if ( err == EZMODE_ERR_SUCCESS )
    {
      pStr = "EZMode: Success";
    }
    else if ( err == EZMODE_ERR_NOMATCH )
    {
      pStr = "EZMode: NoMatch"; // not a match made in heaven
    }
    if ( pStr )
    {
      if ( giLightScreenMode == LIGHT_MAINMODE )
      {
        HalLcdWriteString ( pStr, HAL_LCD_LINE_2 );
      }
    }
#endif
  }

  // finished, either show DstAddr/EP, or nothing (depending on success or not)
  if( state == EZMODE_STATE_FINISH )
  {
    // turn off identify mode
    zclRouterVersion1_IdentifyTime = 0;
    zclRouterVersion1_ProcessIdentifyTimeChange();

#ifdef LCD_SUPPORTED
    // if successful, inform user which nwkaddr/ep we bound to
    pStr = NULL;
    err = pData->sFinish.err;
    if( err == EZMODE_ERR_SUCCESS )
    {
      // already stated on autoclose
    }
    else if ( err == EZMODE_ERR_CANCELLED )
    {
      pStr = "EZMode: Cancel";
    }
    else if ( err == EZMODE_ERR_BAD_PARAMETER )
    {
      pStr = "EZMode: BadParm";
    }
    else if ( err == EZMODE_ERR_TIMEDOUT )
    {
      pStr = "EZMode: TimeOut";
    }
    if ( pStr )
    {
      if ( giLightScreenMode == LIGHT_MAINMODE )
      {
        HalLcdWriteString ( pStr, HAL_LCD_LINE_2 );
      }
    }
#endif
    // show main UI screen 3 seconds after binding
    osal_start_timerEx( zclRouterVersion1_TaskID, ROUTERVERSION1_MAIN_SCREEN_EVT, 3000 );
  }
}
#endif // ZCL_EZMODE

/****************************************************************************
****************************************************************************/


