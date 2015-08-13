#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_ezmode.h"
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

#include "zcl_RouterVersion1.h"

/*********************************************************************
 * CONSTANTS
 */

#define ROUTERVERSION1_DEVICE_VERSION     0
#define ROUTERVERSION1_FLAGS              0

#define ROUTERVERSION1_HWVERSION          1
#define ROUTERVERSION1_ZCLVERSION         1

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Basic Cluster
const uint8 zclRouterVersion1_HWRevision = ROUTERVERSION1_HWVERSION;
const uint8 zclRouterVersion1_ZCLVersion = ROUTERVERSION1_ZCLVERSION;
const uint8 zclRouterVersion1_ManufacturerName[] = { 16, 'T','e','x','a','s','I','n','s','t','r','u','m','e','n','t','s' };
const uint8 zclRouterVersion1_ModelId[] = { 16, 'T','I','0','0','0','1',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 zclRouterVersion1_DateCode[] = { 16, '2','0','0','6','0','8','3','1',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 zclRouterVersion1_PowerSource = POWER_SOURCE_MAINS_1_PHASE;

uint8 zclRouterVersion1_LocationDescription[17] = { 16, ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
uint8 zclRouterVersion1_PhysicalEnvironment = 0;
uint8 zclRouterVersion1_DeviceEnable = DEVICE_ENABLED;

// Identify Cluster
uint16 zclRouterVersion1_IdentifyTime = 0;
#ifdef ZCL_EZMODE
uint8  zclRouterVersion1_IdentifyCommissionState;
#endif

// On/Off Cluster
uint8  zclRouterVersion1_OnOff = LIGHT_OFF;

// Level Control Cluster
#if ZCL_DISCOVER
CONST zclCommandRec_t zclRouterVersion1_Cmds[] =
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
    COMMAND_ON,
    CMD_DIR_SERVER_RECEIVED
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    COMMAND_TOGGLE,
    CMD_DIR_SERVER_RECEIVED
  }
};

CONST uint8 zclCmdsArraySize = ( sizeof(zclRouterVersion1_Cmds) / sizeof(zclRouterVersion1_Cmds[0]) );
#endif // ZCL_DISCOVER

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zclRouterVersion1_Attrs[] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zclRouterVersion1_HWRevision  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclRouterVersion1_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclRouterVersion1_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclRouterVersion1_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclRouterVersion1_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclRouterVersion1_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclRouterVersion1_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclRouterVersion1_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclRouterVersion1_DeviceEnable
    }
  },

#ifdef ZCL_IDENTIFY
  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclRouterVersion1_IdentifyTime
    }
  },
 #ifdef ZCL_EZMODE
  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_COMMISSION_STATE,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ),
      (void *)&zclRouterVersion1_IdentifyCommissionState
    }
  },
 #endif // ZCL_EZMODE
#endif

  // *** On/Off Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF,
      ZCL_DATATYPE_BOOLEAN,
      ACCESS_CONTROL_READ,
      (void *)&zclRouterVersion1_OnOff
    }
  }
 #ifdef ZCL_DIAGNOSTIC
  , {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_NUMBER_OF_RESETS,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_PERSISTENT_MEMORY_WRITES,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_MAC_RX_BCAST,
      ZCL_DATATYPE_UINT32,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_MAC_TX_BCAST,
      ZCL_DATATYPE_UINT32,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_MAC_RX_UCAST,
      ZCL_DATATYPE_UINT32,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_MAC_TX_UCAST,
      ZCL_DATATYPE_UINT32,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_MAC_TX_UCAST_RETRY,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_MAC_TX_UCAST_FAIL,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_APS_RX_BCAST,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_APS_TX_BCAST,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_APS_RX_UCAST,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_APS_TX_UCAST_SUCCESS,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_APS_TX_UCAST_RETRY,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_APS_TX_UCAST_FAIL,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_ROUTE_DISC_INITIATED,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_NEIGHBOR_ADDED,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_NEIGHBOR_REMOVED,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_NEIGHBOR_STALE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_JOIN_INDICATION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_CHILD_MOVED,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_NWK_FC_FAILURE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_APS_FC_FAILURE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_APS_UNAUTHORIZED_KEY,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_NWK_DECRYPT_FAILURES,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_APS_DECRYPT_FAILURES,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_PACKET_BUFFER_ALLOCATE_FAILURES,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_RELAYED_UCAST,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_PHY_TO_MAC_QUEUE_LIMIT_REACHED,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_PACKET_VALIDATE_DROP_COUNT,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_AVERAGE_MAC_RETRY_PER_APS_MESSAGE_SENT,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_LAST_MESSAGE_LQI,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    {  // Attribute record
      ATTRID_DIAGNOSTIC_LAST_MESSAGE_RSSI,
      ZCL_DATATYPE_INT8,
      ACCESS_CONTROL_READ,
      NULL // Use application's callback to Read this attribute
    }
  },
#endif // ZCL_DIAGNOSTIC
};

uint8 CONST zclRouterVersion1_NumAttributes = ( sizeof(zclRouterVersion1_Attrs) / sizeof(zclRouterVersion1_Attrs[0]) );

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const cId_t zclRouterVersion1_InClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_GROUPS,
  ZCL_CLUSTER_ID_GEN_SCENES,
  ZCL_CLUSTER_ID_GEN_ON_OFF
};
// work-around for compiler bug... IAR can't calculate size of array with #if options.
 #define ZCLROUTERVERSION1_MAX_INCLUSTERS   5

const cId_t zclRouterVersion1_OutClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_SCENES,
  ZCL_CLUSTER_ID_GEN_ON_OFF,
  ZCL_CLUSTER_ID_GEN_COMMISSIONING
};
#define ZCLROUTERVERSION1_MAX_OUTCLUSTERS  (sizeof(zclRouterVersion1_OutClusterList) / sizeof(zclRouterVersion1_OutClusterList[0]))

SimpleDescriptionFormat_t zclRouterVersion1_SimpleDesc =
{
  ROUTERVERSION1_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                     //  uint16 AppProfId;
  ZCL_HA_DEVICEID_ON_OFF_LIGHT,          //  uint16 AppDeviceId;
  ROUTERVERSION1_DEVICE_VERSION,            //  int   AppDevVer:4;
  ROUTERVERSION1_FLAGS,                     //  int   AppFlags:4;
  ZCLROUTERVERSION1_MAX_INCLUSTERS,         //  byte  AppNumInClusters;
  (cId_t *)zclRouterVersion1_InClusterList, //  byte *pAppInClusterList;
  ZCLROUTERVERSION1_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
  (cId_t *)zclRouterVersion1_OutClusterList //  byte *pAppInClusterList;
};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/****************************************************************************
****************************************************************************/


