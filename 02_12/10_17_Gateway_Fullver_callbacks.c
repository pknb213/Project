/*********************************************************************************
*					2017_12_16 Gateway										     *
*																				 *
*********************************************************************************/
// Copyright 2016 Silicon Laboratories, Inc.                                *80*

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/util/util.h"
#include "app/framework/plugin/command-relay/command-relay.h"
#include "app/framework/plugin/device-table/device-table.h"
#include "util/plugin/plugin-common/linked-list/linked-list.h"
#include "util/plugin/plugin-common/transport-mqtt/transport-mqtt.h"
#include "util/third_party/cjson/cJSON.h"
#include "app/util/source-route-common.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"
#include "stack/include/trust-center.h"

#include <sys/time.h>
#include <stdlib.h>

// ****************************************************************************
// custom commands forward declarations
static const EmberEUI64 NULL_EUI = { 0, 0, 0, 0, 0, 0, 0, 0 };
#define isNullEui(eui) (MEMCOMPARE(eui, NULL_EUI, EUI64_SIZE) == 0)

// the number of tokens that can be written using ezspSetToken and read
// using ezspGetToken
#define MFGSAMP_NUM_EZSP_TOKENS 8
// the size of the tokens that can be written using ezspSetToken and
// read using ezspGetToken
#define MFGSAMP_EZSP_TOKEN_SIZE 8
// the number of manufacturing tokens
#define MFGSAMP_NUM_EZSP_MFG_TOKENS 11
// the size of the largest EZSP Mfg token, EZSP_MFG_CBKE_DATA
// please refer to app/util/ezsp/ezsp-enum.h
#define MFGSAMP_EZSP_TOKEN_MFG_MAXSIZE 92

EmberStatus emberAfTrustCenterStartNetworkKeyUpdate(void);

// Forward declarations of command functions
static void printSourceRouteTable(void);
static void mfgappTokenDump(void);
static void changeNwkKeyCommand(void);
static void printNextKeyCommand(void);
static void versionCommand(void);
static void setTxPowerCommand(void);

EmberCommandEntry emberAfCustomCommands[] = {
  emberCommandEntryAction("print_srt", printSourceRouteTable, "", ""),
  emberCommandEntryAction("tokdump", mfgappTokenDump, "", ""),
  emberCommandEntryAction("changeNwkKey", changeNwkKeyCommand, "", ""),
  emberCommandEntryAction("printNextKey", printNextKeyCommand, "", ""),
  emberCommandEntryAction("version", versionCommand, "", ""),
  emberCommandEntryAction("txPower", setTxPowerCommand, "s", ""),
  emberCommandEntryTerminator()
};
//// ******* test of token dump code

// the manufacturing tokens are enumerated in app/util/ezsp/ezsp-protocol.h
// the names are enumerated here to make it easier for the user
PGM_NO_CONST PGM_P ezspMfgTokenNames[] =
  {
    "EZSP_MFG_CUSTOM_VERSION...",
    "EZSP_MFG_STRING...........",
    "EZSP_MFG_BOARD_NAME.......",
    "EZSP_MFG_MANUF_ID.........",
    "EZSP_MFG_PHY_CONFIG.......",
    "EZSP_MFG_BOOTLOAD_AES_KEY.",
    "EZSP_MFG_ASH_CONFIG.......",
    "EZSP_MFG_EZSP_STORAGE.....",
    "EZSP_STACK_CAL_DATA.......",
    "EZSP_MFG_CBKE_DATA........",
    "EZSP_MFG_INSTALLATION_CODE"
  };
//Network Parameter
#define NETWORK_PANID	0x1012
#define NETWORK_CHENNEL	12
#define NETWORK_POWER	3
  
// Constants
#define HOSTVER_STRING_LENGTH 14 // 13 characters + NULL (99.99.99-9999)
#define EUI64_STRING_LENGTH 19 // "0x" + 16 characters + NULL
#define NODEID_STRING_LENGTH 7 // "0x" + 4 characters + NULL
#define CLUSTERID_STRING_LENGTH 7 // "0x" + 4 chars + NULL
#define GATEWAY_TOPIC_PREFIX_LENGTH 22 // 21 chars `gw/xxxxxxxxxxxxxxxx/` + NULL
#define HEARTBEAT_RATE_MS 5000 // milliseconds
#define PROCESS_COMMAND_RATE_MS 20 // milliseconds
#define BLOCK_SENT_THROTTLE_VALUE 25 // blocks to receive before sending updates
#define HEX_TOKEN_SIZE 2 // "0x"

// Attribute reading buffer location definitions
#define ATTRIBUTE_BUFFER_CLUSTERID_HIGH_BITS 1
#define ATTRIBUTE_BUFFER_CLUSTERID_LOW_BITS  0
#define ATTRIBUTE_BUFFER_SUCCESS_CODE        2
#define ATTRIBUTE_BUFFER_DATA_TYPE           3
#define ATTRIBUTE_BUFFER_DATA_START          4

// Attribute reporting / IAS ZONE buffer location definitions
#define ATTRIBUTE_BUFFER_REPORT_DATA_START          3
#define ATTRIBUTE_BUFFER_REPORT_CLUSTERID_HIGH_BITS 1
#define ATTRIBUTE_BUFFER_REPORT_CLUSTERID_LOW_BITS  0
#define ATTRIBUTE_BUFFER_REPORT_DATA_TYPE           2

#define COMMAND_OFFSET 2
#define ZCL_RESPONSE_TOPIC "zclresponse"
#define ZDO_RESPONSE_TOPIC "zdoresponse"
#define APS_RESPONSE_TOPIC "apsresponse"

#define ONE_BYTE_HEX_STRING_SIZE  5
#define TWO_BYTE_HEX_STRING_SIZE  7
#define FOUR_BYTE_HEX_STRING_SIZE 11

#define READ_REPORT_CONFIG_STATUS       0
#define READ_REPORT_CONFIG_DIRECTION    1
#define READ_REPORT_CONFIG_ATTRIBUTE_ID 2
#define READ_REPORT_CONFIG_DATA_TYPE    4
#define READ_REPORT_CONFIG_MIN_INTERVAL 5
#define READ_REPORT_CONFIG_MAX_INTERVAL 7
#define READ_REPORT_CONFIG_DATA         9

#define DEVICE_TABLE_BIND_RESPONSE_STATUS 1

#define BINDING_TABLE_RESPONSE_NUM_ENTRIES 4
#define BINDING_TABLE_RESPONSE_STATUS      1
#define BINDING_TABLE_RESPONSE_ENTRIES     5
#define BINDING_TABLE_RESPONSE_ENTRY_SIZE  21
#define BINDING_ENTRY_EUI                0
#define BINDING_ENTRY_SOURCE_ENDPOINT    8
#define BINDING_ENTRY_CLUSTER_ID         9
#define BINDING_ENTRY_ADDRESS_MODE       11
#define BINDING_ENTRY_DEST_EUI           12
#define BINDING_ENTRY_DEST_ENDPOINT      20

// Gateway global variables
static EmberEUI64 gatewayEui64;
static char gatewayEui64String[EUI64_STRING_LENGTH] = {0};
static char gatewayTopicUriPrefix[GATEWAY_TOPIC_PREFIX_LENGTH] = {0};
static char gwDevicesTopicUriPrefix[GATEWAY_TOPIC_PREFIX_LENGTH+8] = {0};
static bool trafficReporting = false;
static uint32_t otaBlockSent = 0; // This only supports one device OTA at a time

// Command list global variables
// We need to keep a list of commands to process as the come in from an external
// source to the gateway, these are the structs, defines and list needed for
// that list
#define COMMAND_TYPE_CLI        0x01
#define COMMAND_TYPE_POST_DELAY 0x02

typedef struct _GatewayCommand {
  uint8_t commandType;
  char* cliCommand; // used for COMMAND_TYPE_CLI
  uint32_t postDelayMs; // used for COMMAND_TYPE_POST_DELAY
  uint64_t resumeTime; // used for COMMAND_TYPE_POST_DELAY
} GatewayCommand;

static EmberAfPluginLinkedList* commandList;

static GatewayCommand* allocateGatewayCommand();
static void freeGatewayCommand(GatewayCommand* gatewayCommand);
static void addCliCommandToList(EmberAfPluginLinkedList* commandList,
                                const char* cliCommand);
static void addPostDelayMsToList(EmberAfPluginLinkedList* commandList,
                                 uint32_t postDelayMs);
static void eui64ToString(EmberEUI64, char*);
//********************** Object *******************************
EmberApsFrame unicastAf;
//EmberApsFrame routersAf;
static EmberAfIncomingMessage* deviceMessage;
//********************** Function *******************************
//static void sendDataReceivedConfirmation(uint16_t nodeId);
static void sendUnicastSettingUpReadyForZE(uint16_t nodeId);
//static void initRoutersApsFrame();
static void initApsFrame();
static uint32_t getInt32FromMessage(uint8_t* msg, uint8_t start);
static int16_t getInt16FromMessage(uint8_t* msg, uint8_t start);
//static void addInt32ToMessage(uint8_t* msg, uint32_t num, uint8_t start, uint8_t msg_len);
//static void addInt16ToMessage(uint8_t* msg, uint16_t num, uint8_t start, uint8_t msg_len);
//static int32_t getInt32From16Message(uint8_t* msg, uint8_t start);
void publishMqttBasicData(EmberEUI64 nodeEui64, uint8_t *message, uint16_t source);
void publishMqttTempData(EmberEUI64 nodeEui64, uint8_t *message, uint16_t source, int32_t temp);
void publishMqttGyroData(EmberEUI64 nodeEui64, uint8_t *message, uint16_t source, int16_t gyro_X, int16_t gyro_Y, int16_t gyro_Z);
void publishMqttSoundData(EmberEUI64 nodeEui64, uint8_t *message, uint16_t source, int32_t noise, int32_t temp);
//********************** Event *******************************
EmberEventControl nwkFormEventControl;
EmberEventControl nwkReFormEventControl;
EmberEventControl multicastRequireRouteEventControl;
EmberEventControl nwkStateCheckEventControl;

void nwkFormEventHandler()
{
	emberEventControlSetInactive(nwkFormEventControl);
	EmberStatus status;
	EmberNetworkParameters networkParams;
	MEMSET(&networkParams, 0, sizeof(EmberNetworkParameters));
	emberAfGetFormAndJoinExtendedPanIdCallback(networkParams.extendedPanId);
	networkParams.panId = NETWORK_PANID;
	networkParams.radioChannel = NETWORK_CHENNEL;
	networkParams.radioTxPower = NETWORK_POWER;

	status = emberAfFormNetwork(&networkParams);
	if(status == 0x00) {
		emberAfAppPrintln("Form network success..");
	}
}
void multicastRequireRouteEventHandler( )
{
	emberEventControlSetInactive(multicastRequireRouteEventControl);
	/*
	emberAfAppPrintln("******! [Multicasting] Router Permission !******");
	
	uint8_t msg[4];
	msg[0] = 0;
	msg[1] = 0;
	msg[2] = 0xBA;
	msg[3] = 0xff;

	uint8_t len = sizeof(msg)/sizeof(msg[0]);
	EmberApsFrame apsFrame;
	apsFrame.clusterId = 0xB100;
	apsFrame.destinationEndpoint = emberAfPrimaryEndpoint();
	apsFrame.groupId = 0x1000;
	apsFrame.options = EMBER_APS_OPTION_NONE;
	apsFrame.profileId = emberAfPrimaryProfileId();
	apsFrame.sequence = 0;
	apsFrame.sourceEndpoint = emberAfPrimaryEndpoint();

	EmberStatus status = emberAfSendMulticast(0x1000, &apsFrame, len, msg);
	if(status != 0x00)
	{
		emberAfAppPrintln("*** [Multicast Fail]");
	}
	emberEventControlSetDelayMinutes(multicastRequireRouteEventControl, 5);
	*/
}
void nwkReFormEventHandler()
{
	emberEventControlSetInactive(nwkReFormEventControl);
	EmberStatus status = emberLeaveNetwork();
	UNUSED_VAR(status);
	emberEventControlSetDelayQS(nwkFormEventControl, 240);
}
void nwkStateCheckEventHandler()
{
	emberEventControlSetInactive(nwkStateCheckEventControl);
	uint8_t nwkState = emberAfNetworkState();
    EmberNetworkParameters parameters;
    EmberNodeType nodeType;
    emberAfGetNetworkParameters(&nodeType, &parameters);

	if(nwkState == EMBER_NO_NETWORK)
	{
		emberAfAppPrintln("Network Forming Event Execution");
		emberEventControlSetActive(nwkFormEventControl);
	}
	else if(nwkState == EMBER_JOINED_NETWORK)
	{
	  if(parameters.panId != NETWORK_PANID)
	  {
		emberEventControlSetDelayMS(nwkReFormEventControl,5000);
	  }
	  else
	  {
		emberAfAppPrintln("Network Permission for End-device (255 sec)");
		//sendUnicastSettingUpReadyForZE(incomingMessage->source);
		emAfPermitJoin(255,0); 
	  }
	}
	emberEventControlSetDelayMinutes(nwkStateCheckEventControl, 4);
}
EmberEventData appEvents[] =
{
	{& nwkFormEventControl, nwkFormEventHandler},
	{& multicastRequireRouteEventControl, multicastRequireRouteEventHandler},
	{& nwkReFormEventControl, nwkReFormEventHandler},
	{& nwkStateCheckEventControl, nwkStateCheckEventHandler},
	{ NULL, NULL}
};

void initApsFrame() {
// Init APSframe for sending unicast
  unicastAf.clusterId = 0xF100;
  unicastAf.destinationEndpoint = emberAfPrimaryEndpoint();
  unicastAf.groupId = 0x0000;
  unicastAf.options = EMBER_APS_OPTION_NONE;
  unicastAf.profileId = emberAfPrimaryProfileId();
  unicastAf.sequence = 0;
  unicastAf.sourceEndpoint = emberAfPrimaryEndpoint();
}

/*void initRoutersApsFrame() {
// Multicast to Router APSframe for sending unicast
  routersAf.clusterId = 0xA100;
  routersAf.destinationEndpoint = emberAfPrimaryEndpoint();
  routersAf.groupId = 0x1000;
  routersAf.options = EMBER_APS_OPTION_NONE;
  routersAf.profileId = emberAfPrimaryProfileId();
  routersAf.sequence = 0;
  routersAf.sourceEndpoint = emberAfPrimaryEndpoint();
}*/

/*void addInt32ToMessage(uint8_t* msg, uint32_t num, uint8_t start, uint8_t msg_len)
{
   if(msg_len >= start+3)
   {
      for(uint8_t i = 0; i < 4; i++){
            msg[start+i] = ((uint8_t*)&num)[3-i];
      }
   }
}
void addInt16ToMessage(uint8_t* msg, uint16_t num, uint8_t start, uint8_t msg_len)
{
   if(msg_len >= start+1)
   {
      for(uint8_t i = 0; i < 2; i++){
            msg[start+i] = ((uint8_t*)&num)[1-i];
      }
   }
}
int32_t getInt32From16Message(uint8_t* msg, uint8_t start){
	int32_t num = 0;
	for(uint8_t i = 0; i < 2; i++) {
		num = (num << 8 | msg[start+i]);
	}

	return num;
}*/
uint32_t getInt32FromMessage(uint8_t* msg, uint8_t start){
	uint32_t num = 0;
	for(uint8_t i = 0; i < 4; i++) {
		num = (num << 8 | msg[start+i]);
	}

	return num;
}
int16_t getInt16FromMessage(uint8_t* msg, uint8_t start){
	int16_t num = 0;
	for(uint8_t i = 0; i < 2; i++) {
		num = (num << 8 | msg[start+i]);
	}

	return num;
}
void sendUnicastSettingUpReadyForZE(uint16_t nodeId){
	uint8_t msg[3];
	msg[0] = 0;
	msg[1] = 0;
	msg[2] = 0xFB;
	emberAfSendUnicast(EMBER_OUTGOING_DIRECT, nodeId, &unicastAf,3, msg);
}
/*void sendDataReceivedConfirmation(uint16_t nodeId){
	uint8_t msg[3];
	msg[0] = 0;
	msg[1] = 0;
	msg[2] = 0x3B;

	emberAfSendUnicast(EMBER_OUTGOING_DIRECT, nodeId, &unicastAf,3, msg);
}*/
//**************************************************************************************
// Events
EmberEventControl heartbeatEventControl;
void heartbeatEventFunction(void);

EmberEventControl processCommandEventControl;
void processCommandEventFunction(void);

EmberEventControl stateUpdateEventControl;
void stateUpdateEventFunction(void);

// String/other helpers
static void nodeIdToString(EmberNodeId nodeId, char* nodeIdString);
static void eui64ToString(EmberEUI64 eui, char* euiString);
static void printAttributeBuffer(uint16_t clusterId,
                                 uint8_t* buffer,
                                 uint16_t bufLen);

// MQTT API publishing helpers
static char* allocateAndFormMqttGatewayTopic(char* channel);
static void publishMqttHeartbeat(void);
static void publishMqttRelays(void);
static void publishMqttGatewayState(void);
static void publishMqttDeviceStateChange(EmberEUI64 eui64,
                                         uint8_t state);
static void publishMqttDeviceJoined(EmberEUI64 eui64);
static void publishMqttDeviceLeft(EmberEUI64 eui64);
static void publishMqttAttribute(EmberEUI64 eui64,
                                 EmberAfClusterId clusterId,
                                 uint8_t* buffer,
                                 uint16_t bufLen);
static void publishMqttTrafficReportEvent(char* messageType,
                                          EmberStatus* status,
                                          int8_t* lastHopRssi,
                                          uint8_t* lastHopLqi,
                                          uint8_t* zclSequenceNumber,
                                          uint64_t timeMS);
static void publishMqttOtaEvent(char* messageType,
                                EmberEUI64 eui64,
                                uint8_t* status,
                                uint32_t* blockSent,
                                uint8_t* actualLength,
                                uint16_t* manufacturerId,
                                uint16_t* imageTypeId,
                                uint32_t* firmwareVersion);
static void publishMqttCommandExecuted(char* cliCommand);
static void publishMqttDelayExecuted(uint32_t postDelayMs);
static cJSON* buildNodeJson(uint16_t deviceTableIndex);
static void publishMqttZclCommand(uint8_t commandId,
                                  boolean clusterSpecific,
                                  uint16_t clusterId,
                                  boolean mfgSpecific,
                                  uint16_t mfgCode,
                                  uint8_t* buffer,
                                  uint8_t bufLen,
                                  uint8_t payloadStartIndex);
static void publishMqttBindResponse(EmberNodeId nodeId,
                                    EmberApsFrame* apsFrame,
                                    uint8_t* message,
                                    uint16_t length);
static void publishMqttBindTableReponse(EmberNodeId nodeId,
                                        EmberApsFrame* apsFrame,
                                        uint8_t* message,
                                        uint16_t length);

// MQTT topic and handler list
typedef void (*MqttTopicHandler)(cJSON* messageJson);

typedef struct _MqttTopicHandlerMap
{
  char* topic;
  MqttTopicHandler topicHandler;
} MqttTopicHandlerMap;

EmberAfPluginLinkedList* topicHandlerList;

static MqttTopicHandlerMap* buildTopicHandler(char* topicString,
                                              MqttTopicHandler topicHandlerFunction);

// MQTT API subscription helpers
static void handleCommandsMessage(cJSON* messageJson);
static void handlePublishStateMessage(cJSON* messageJson);
static void handleUpdateSettingsMessage(cJSON* messageJson);

// Handy string creation routines.
static char* createOneByteHexString(uint8_t value)
{
  char* outputString = (char *) malloc(ONE_BYTE_HEX_STRING_SIZE);

  sprintf(outputString, "0x%02X", value);

  return outputString;
}

static char* createTwoByteHexString(uint16_t value)
{
  char* outputString = (char *) malloc(TWO_BYTE_HEX_STRING_SIZE);

  sprintf(outputString, "0x%04X", value);

  return outputString;
}

//************* Main *************************************************
void emberAfMainTickCallback(void)
{
	emberRunEvents(appEvents);
}
boolean emberAfStackStatusCallback(EmberStatus status)
{
	if(status == 0x90 && emberAfNetworkState() == 0x02)
	{
		emberAfAppPrintln("******* Network Permit join 255 sec");
		emAfPermitJoin(255,0);
	}else if(emberAfNetworkState() == 0x00)
	{
		/*emberAfAppPrintln("******* No Network Status");
		emberEventControlSetDelayQS(networkReformEventControl, 12);*/
	}
	return 0;
}
void emberAfMainInitCallback(void)
{
	// Save our EUI information
	emberAfGetEui64(gatewayEui64);
	sprintf(gatewayEui64String, "%02X%02X%02X%02X%02X%02X%02X%02X",
		  gatewayEui64[7],
		  gatewayEui64[6],
		  gatewayEui64[5],
		  gatewayEui64[4],
		  gatewayEui64[3],
		  gatewayEui64[2],
		  gatewayEui64[1],
		  gatewayEui64[0]);

	emberAfAppPrintln("HA Gateweay EUI64 = %s",
					gatewayEui64String);

	strcat(gatewayTopicUriPrefix, "gw/");
	strcat(gatewayTopicUriPrefix, gatewayEui64String);
	strcat(gatewayTopicUriPrefix, "/");

	// create uri prefix for devices topic
	// uri prefix: gw/<uei64>/devices/
	strcpy(gwDevicesTopicUriPrefix, gatewayTopicUriPrefix);
	strcat(gwDevicesTopicUriPrefix, "devices/");

	// Init our command list
	commandList = emberAfPluginLinkedListInit();

	// Init our topic handler list and the maps in the list, note that this is
	// done after the topicUriPrefix is assigned above, since it is used here
	topicHandlerList = emberAfPluginLinkedListInit();
	emberAfPluginLinkedListPushBack(topicHandlerList,
								  (void*)buildTopicHandler("commands",
														   handleCommandsMessage));
	emberAfPluginLinkedListPushBack(topicHandlerList,
								  (void*)buildTopicHandler("publishstate",
														  handlePublishStateMessage));
	emberAfPluginLinkedListPushBack(topicHandlerList,
								  (void*)buildTopicHandler("updatesettings",
														   handleUpdateSettingsMessage));
	emberEventControlSetActive(stateUpdateEventControl);

	initApsFrame();
	//initRoutersApsFrame();

	emberEventControlSetDelayQS(nwkStateCheckEventControl, 40);
	//emberEventControlSetDelayQS(multicastRequireRouteEventControl,160);
}
void publishMqttBasicData(EmberEUI64 nodeEui64, uint8_t *message, uint16_t source)
{
	char *deviceReportSuffix = "/basic";
	char euiString[EUI64_STRING_LENGTH] = {0};
	eui64ToString(nodeEui64, euiString);
	uint16_t deviceTopicLen = strlen(gwDevicesTopicUriPrefix) + strlen(euiString) + strlen(deviceReportSuffix) + 1;

	char* topicDeviceEui64 = (char*)malloc(deviceTopicLen);
	strcpy(topicDeviceEui64, gwDevicesTopicUriPrefix);
	strcat(topicDeviceEui64, euiString);
	strcat(topicDeviceEui64, deviceReportSuffix);

	char reportingNodeIdString[NODEID_STRING_LENGTH] = {0};
	char reportingCmdString[5] = {0};
	cJSON* globalReadJson;
	char* globalReadJsonString;

	uint8_t lqi = deviceMessage->lastHopLqi;
	int8_t rssi = deviceMessage->lastHopRssi;	 // need to fix  uint8 -> int8
	uint8_t cmd = message[2];
	uint8_t nodeType = message[3];
	uint8_t sensorType = message[4];
	uint16_t tranjectionId = getInt16FromMessage(message,5);
	uint32_t time = getInt32FromMessage(message,7);
	uint8_t errorCount = message[11];
	uint8_t nwkDownCount = message[12];

	sprintf(reportingNodeIdString, "0x%04X", source);
	sprintf(reportingCmdString, "0x%02X", cmd);

	globalReadJson = cJSON_CreateObject();

	cJSON_AddStringToObject(globalReadJson, "NodeId", reportingNodeIdString);
	cJSON_AddIntegerToObject(globalReadJson, "Rssi", rssi);
	cJSON_AddIntegerToObject(globalReadJson, "Lqi", lqi);
	cJSON_AddIntegerToObject(globalReadJson, "ErrorCount", errorCount);
	cJSON_AddIntegerToObject(globalReadJson, "NwkDownCount", nwkDownCount);
	
	cJSON_AddStringToObject(globalReadJson, "Cmd", reportingCmdString);
	cJSON_AddIntegerToObject(globalReadJson, "NodeType", nodeType);
	cJSON_AddIntegerToObject(globalReadJson, "SensorType", sensorType);
	cJSON_AddIntegerToObject(globalReadJson, "TranjectionId", tranjectionId);
	cJSON_AddIntegerToObject(globalReadJson, "Time", time);

	globalReadJsonString = cJSON_PrintUnformatted(globalReadJson);
	emberAfPluginTransportMqttPublish(topicDeviceEui64, globalReadJsonString);

	free(globalReadJsonString);
	free(topicDeviceEui64);
	cJSON_Delete(globalReadJson);
}
void publishMqttTempData(EmberEUI64 nodeEui64, uint8_t *message, uint16_t source, int32_t temp)
{
	char *deviceReportSuffix = "/temp";
	char euiString[EUI64_STRING_LENGTH] = {0};
	eui64ToString(nodeEui64, euiString);
	uint16_t deviceTopicLen = strlen(gwDevicesTopicUriPrefix) + strlen(euiString) + strlen(deviceReportSuffix) + 1;

	char* topicDeviceEui64 = (char*)malloc(deviceTopicLen);
	strcpy(topicDeviceEui64, gwDevicesTopicUriPrefix);
	strcat(topicDeviceEui64, euiString);
	strcat(topicDeviceEui64, deviceReportSuffix);

	char reportingNodeIdString[NODEID_STRING_LENGTH] = {0};
	char reportingCmdString[5] = {0};
	cJSON* globalReadJson;
	char* globalReadJsonString;

	uint8_t lqi = deviceMessage->lastHopLqi;
	int8_t rssi = deviceMessage->lastHopRssi;	 // need to fix  uint8 -> int8
	uint8_t cmd = message[2];
	uint8_t nodeType = message[3];
	uint8_t sensorType = message[4];
	uint16_t tranjectionId = getInt16FromMessage(message,5);
	uint32_t time = getInt32FromMessage(message,7);
	uint8_t errorCount = message[11];
	uint8_t nwkDownCount = message[12];

	sprintf(reportingNodeIdString, "0x%04X", source);
	sprintf(reportingCmdString, "0x%02X", cmd);
	
	globalReadJson = cJSON_CreateObject();

	cJSON_AddStringToObject(globalReadJson, "NodeId", reportingNodeIdString);
	cJSON_AddIntegerToObject(globalReadJson, "Rssi", rssi);
	cJSON_AddIntegerToObject(globalReadJson, "Lqi", lqi);
	cJSON_AddIntegerToObject(globalReadJson, "ErrorCount", errorCount);
	cJSON_AddIntegerToObject(globalReadJson, "NwkDownCount", nwkDownCount);

	cJSON_AddStringToObject(globalReadJson, "Cmd", reportingCmdString);
	cJSON_AddIntegerToObject(globalReadJson, "NodeType", nodeType);
	cJSON_AddIntegerToObject(globalReadJson, "SensorType", sensorType);
	cJSON_AddIntegerToObject(globalReadJson, "TranjectionId", tranjectionId);
	cJSON_AddIntegerToObject(globalReadJson, "Time", time);
	cJSON_AddIntegerToObject(globalReadJson, "Temperature", temp);

	globalReadJsonString = cJSON_PrintUnformatted(globalReadJson);
	emberAfPluginTransportMqttPublish(topicDeviceEui64, globalReadJsonString);

	free(globalReadJsonString);
	free(topicDeviceEui64);
	cJSON_Delete(globalReadJson);
}
void publishMqttGyroData(EmberEUI64 nodeEui64, uint8_t *message, uint16_t source, int16_t gyro_X, int16_t gyro_Y, int16_t gyro_Z)
{
	char *deviceReportSuffix = "/gyro";
	char euiString[EUI64_STRING_LENGTH] = {0};
	eui64ToString(nodeEui64, euiString);
	uint16_t deviceTopicLen = strlen(gwDevicesTopicUriPrefix) + strlen(euiString) + strlen(deviceReportSuffix) + 1;

	char* topicDeviceEui64 = (char*)malloc(deviceTopicLen);
	strcpy(topicDeviceEui64, gwDevicesTopicUriPrefix);
	strcat(topicDeviceEui64, euiString);
	strcat(topicDeviceEui64, deviceReportSuffix);

	char reportingNodeIdString[NODEID_STRING_LENGTH] = {0};
	char reportingCmdString[5] = {0};
	cJSON* globalReadJson;
	char* globalReadJsonString;

	// LQI RSSI from last hop

	uint8_t lqi = deviceMessage->lastHopLqi;
	int8_t rssi = deviceMessage->lastHopRssi;
	uint8_t cmd = message[2];
	uint8_t nodeType = message[3];
	uint8_t sensorType = message[4];
	uint32_t tranjectionId = getInt16FromMessage(message,5);
	int32_t time = getInt32FromMessage(message,7);
	uint8_t errorCount = message[11];
	uint8_t nwkDownCount = message[12];

	sprintf(reportingNodeIdString, "0x%04X", source);
	sprintf(reportingCmdString, "0x%02X", cmd);

	globalReadJson = cJSON_CreateObject();

	cJSON_AddStringToObject(globalReadJson, "nodeId", reportingNodeIdString);
	cJSON_AddIntegerToObject(globalReadJson, "Rssi", rssi);
	cJSON_AddIntegerToObject(globalReadJson, "Lqi", lqi);
	cJSON_AddIntegerToObject(globalReadJson, "ErrorCount", errorCount);
	cJSON_AddIntegerToObject(globalReadJson, "nwkDownCount", nwkDownCount);

	cJSON_AddStringToObject(globalReadJson, "Cmd", reportingCmdString);
	cJSON_AddIntegerToObject(globalReadJson, "NodeType", nodeType);
	cJSON_AddIntegerToObject(globalReadJson, "SensorType", sensorType);
	cJSON_AddIntegerToObject(globalReadJson, "TranjectionId", tranjectionId);
	cJSON_AddIntegerToObject(globalReadJson, "Time", time);
	cJSON_AddIntegerToObject(globalReadJson, "Gyro_X", gyro_X);
	cJSON_AddIntegerToObject(globalReadJson, "Gyro_Y", gyro_Y);
	cJSON_AddIntegerToObject(globalReadJson, "Gyro_Z", gyro_Z);

	globalReadJsonString = cJSON_PrintUnformatted(globalReadJson);
	emberAfPluginTransportMqttPublish(topicDeviceEui64, globalReadJsonString);

	free(globalReadJsonString);
	free(topicDeviceEui64);
	cJSON_Delete(globalReadJson);
}
void publishMqttSoundData(EmberEUI64 nodeEui64, uint8_t *message, uint16_t source, int32_t noise, int32_t temp)
{
	char *deviceReportSuffix = "/noise";
	char euiString[EUI64_STRING_LENGTH] = {0};
	eui64ToString(nodeEui64, euiString);
	uint16_t deviceTopicLen = strlen(gwDevicesTopicUriPrefix) + strlen(euiString) + strlen(deviceReportSuffix) + 1;

	char* topicDeviceEui64 = (char*)malloc(deviceTopicLen);
	strcpy(topicDeviceEui64, gwDevicesTopicUriPrefix);
	strcat(topicDeviceEui64, euiString);
	strcat(topicDeviceEui64, deviceReportSuffix);

	char reportingNodeIdString[NODEID_STRING_LENGTH] = {0};
	char reportingCmdString[5] = {0};
	cJSON* globalReadJson;
	char* globalReadJsonString;
	

	uint8_t lqi = deviceMessage->lastHopLqi;
	int8_t rssi = deviceMessage->lastHopRssi;	 // need to fix  uint8 -> int8
	uint8_t cmd = message[2];
	uint8_t nodeType = message[3];
	uint8_t sensorType = message[4];
	uint32_t tranjectionId = getInt16FromMessage(message,5);
	int32_t time = getInt32FromMessage(message,7);
	uint8_t errorCount = message[11];
	uint8_t nwkDownCount = message[12];

	sprintf(reportingNodeIdString, "0x%04X", source);
	sprintf(reportingCmdString, "0x%02X", cmd);

	globalReadJson = cJSON_CreateObject();

	cJSON_AddStringToObject(globalReadJson, "nodeId", reportingNodeIdString);
	cJSON_AddIntegerToObject(globalReadJson, "Rssi", rssi);
	cJSON_AddIntegerToObject(globalReadJson, "Lqi", lqi);
	cJSON_AddIntegerToObject(globalReadJson, "ErrorCount", errorCount);
	cJSON_AddIntegerToObject(globalReadJson, "nwkDownCount", nwkDownCount);

	cJSON_AddStringToObject(globalReadJson, "Cmd", reportingCmdString);
	cJSON_AddIntegerToObject(globalReadJson, "NodeType", nodeType);
	cJSON_AddIntegerToObject(globalReadJson, "SensorType", sensorType);
	cJSON_AddIntegerToObject(globalReadJson, "TranjectionId", tranjectionId);
	cJSON_AddIntegerToObject(globalReadJson, "Time", time);
	cJSON_AddIntegerToObject(globalReadJson, "NoiseValue", noise);
	cJSON_AddIntegerToObject(globalReadJson, "Temperature", temp);

	globalReadJsonString = cJSON_PrintUnformatted(globalReadJson);
	emberAfPluginTransportMqttPublish(topicDeviceEui64, globalReadJsonString);

	free(globalReadJsonString);
	free(topicDeviceEui64);
	cJSON_Delete(globalReadJson);
}
/*static void publishMqttDataTransaction(EmberEUI64 nodeEui64, uint16_t source, uint8_t mode, uint32_t transactionId, uint32_t data)
{
	char *deviceReportSuffix = "/data";
	char euiString[EUI64_STRING_LENGTH] = {0};
	eui64ToString(nodeEui64, euiString);
	uint16_t deviceTopicLen = strlen(gwDevicesTopicUriPrefix) + strlen(euiString) + strlen(deviceReportSuffix) + 1;

	char* topicDeviceEui64 = (char*)malloc(deviceTopicLen);
	strcpy(topicDeviceEui64, gwDevicesTopicUriPrefix);
	strcat(topicDeviceEui64, euiString);
	strcat(topicDeviceEui64, deviceReportSuffix);

	char reportingNodeIdString[NODEID_STRING_LENGTH] = {0};
	cJSON* globalReadJson;
	char* globalReadJsonString;

	// For traffice report

	uint8_t lqi;
	int8_t rssi;	 // need to fix  uint8 -> int8

	rssi = deviceMessage->lastHopRssi;
	lqi = deviceMessage->lastHopLqi;

	sprintf(reportingNodeIdString, "0x%04X", source);

	globalReadJson = cJSON_CreateObject();

	cJSON_AddStringToObject(globalReadJson, "nodeId", reportingNodeIdString);
	cJSON_AddIntegerToObject(globalReadJson, "rssi", rssi);
	cJSON_AddIntegerToObject(globalReadJson, "lqi", lqi);

	cJSON_AddIntegerToObject(globalReadJson, "mode", mode);
	cJSON_AddIntegerToObject(globalReadJson, "transactionId", transactionId);
	cJSON_AddIntegerToObject(globalReadJson, "data", data);

	globalReadJsonString = cJSON_PrintUnformatted(globalReadJson);
	emberAfPluginTransportMqttPublish(topicDeviceEui64, globalReadJsonString);

	free(globalReadJsonString);
	free(topicDeviceEui64);
	cJSON_Delete(globalReadJson);

	emberAfAppPrint("publishMqttDataTransaction");
}*/
MqttTopicHandlerMap* buildTopicHandler(char* topicString,
                                       MqttTopicHandler topicHandlerFunction)
{
  MqttTopicHandlerMap* topicHandlerMap =
    (MqttTopicHandlerMap*)malloc(sizeof(MqttTopicHandlerMap));
  topicHandlerMap->topic = allocateAndFormMqttGatewayTopic(topicString);
  topicHandlerMap->topicHandler = topicHandlerFunction;
  return topicHandlerMap;
}

// IAS ACE Server side callbacks
bool emberAfIasAceClusterArmCallback(uint8_t armMode,
                                     uint8_t* armDisarmCode,
                                     uint8_t zoneId)
{
  uint16_t armDisarmCodeLength = emberAfStringLength(armDisarmCode);
  EmberNodeId sender = emberGetSender();
  uint16_t i;

  emberAfAppPrint("IAS ACE Arm Received %x", armMode);

  // Start i at 1 to skip over leading character in the byte array as it is the
  // length byte
  for (i = 1; i < armDisarmCodeLength; i++) {
    emberAfAppPrint("%c", armDisarmCode[i]);
  }
  emberAfAppPrintln(" %x", zoneId);

  emberAfFillCommandIasAceClusterArmResponse(armMode);
  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, sender);

  return true;
}

bool emberAfIasAceClusterBypassCallback(uint8_t numberOfZones,
                                        uint8_t* zoneIds,
                                        uint8_t* armDisarmCode)
{
  EmberNodeId sender = emberGetSender();
  uint8_t i;

  emberAfAppPrint("IAS ACE Cluster Bypass for zones ");

  for (i = 0; i < numberOfZones; i++) {
    emberAfAppPrint("%d ", zoneIds[i]);
  }
  emberAfAppPrintln("");

  emberAfFillCommandIasAceClusterBypassResponse(numberOfZones,
                                                zoneIds,
                                                numberOfZones);
  emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, sender);

  return true;
}

void heartbeatEventFunction(void)
{
  publishMqttHeartbeat();
  emberEventControlSetDelayMS(heartbeatEventControl, HEARTBEAT_RATE_MS);
}

// MQTT Helper Functions
static char* allocateAndFormMqttGatewayTopic(char* topic)
{
  // Add our string sizes + one NULL char
  uint16_t stringSize = strlen(gatewayTopicUriPrefix) + strlen(topic) + 1;
  char* fullTopicUri = (char*)malloc(stringSize);
  if (fullTopicUri == NULL) {
    emberAfAppPrintln("FATAL ERR: Unable to allocate more memory!");
    assert(false);
  }
  strcpy(fullTopicUri, gatewayTopicUriPrefix);
  strcat(fullTopicUri, topic);
  return fullTopicUri;
}

static void publishMqttHeartbeat(void)
{
  char* topic = allocateAndFormMqttGatewayTopic("heartbeat");
  EmberNetworkParameters parameters;
  EmberNodeType nodeType;
  cJSON* heartbeatJson;
  char* heartbeatJsonString;
  char* panIdString;

  EmberStatus status = emberAfGetNetworkParameters(&nodeType, &parameters);
  heartbeatJson = cJSON_CreateObject();
  if (!emberAfNcpNeedsReset() && status == EMBER_SUCCESS) {
    cJSON_AddTrueToObject(heartbeatJson, "networkUp");
    panIdString = createTwoByteHexString(parameters.panId);
    cJSON_AddStringToObject(heartbeatJson, "networkPanId", panIdString);
    free(panIdString);
    cJSON_AddIntegerToObject(heartbeatJson,
                             "radioTxPower",
                             parameters.radioTxPower + 22);
    cJSON_AddIntegerToObject(heartbeatJson,
                             "radioChannel",
                             parameters.radioChannel);
  } else {
    cJSON_AddFalseToObject(heartbeatJson, "networkUp");
  }

  heartbeatJsonString = cJSON_PrintUnformatted(heartbeatJson);
  emberAfPluginTransportMqttPublish(topic, heartbeatJsonString);
  free(heartbeatJsonString);
  cJSON_Delete(heartbeatJson);
  free(topic);
}

static void publishMqttDevices(void)
{
  char* topic = allocateAndFormMqttGatewayTopic("devices");
  uint16_t nodeIndex;
  cJSON* nodeJson;
  cJSON* devicesJson;
  cJSON* devicesJsonNodeArray;
  char* devicesJsonString;

  devicesJson = cJSON_CreateObject();
  devicesJsonNodeArray = cJSON_CreateArray();
  cJSON_AddItemToObject(devicesJson, "devices", devicesJsonNodeArray);

  for (nodeIndex = 0;
       nodeIndex < EMBER_AF_PLUGIN_DEVICE_TABLE_DEVICE_TABLE_SIZE;
       nodeIndex++) {
    if (emberAfDeviceTableGetNodeIdFromIndex(nodeIndex) != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_NODE_ID) {
      nodeJson = buildNodeJson(nodeIndex);
      cJSON_AddItemToArray(devicesJsonNodeArray, nodeJson);
    }
  }

  devicesJsonString = cJSON_PrintUnformatted(devicesJson);
  emberAfPluginTransportMqttPublish(topic, devicesJsonString);
  free(devicesJsonString);
  cJSON_Delete(devicesJson);
  free(topic);
}

static cJSON* buildDeviceEndpoint(EmberEUI64 eui64, uint8_t endpoint)
{
  cJSON* deviceEndpointObj;
  char euiString[EUI64_STRING_LENGTH] = {0};
  deviceEndpointObj = cJSON_CreateObject();

  eui64ToString(eui64, euiString);
  cJSON_AddStringToObject(deviceEndpointObj, "eui64", euiString);
  cJSON_AddIntegerToObject(deviceEndpointObj, "endpoint", endpoint);
  return deviceEndpointObj;
}

static cJSON* buildNodeJson(uint16_t nodeIndex)
{
  cJSON* nodeJson;
  cJSON* deviceEndpoint;
  char nodeIdString[NODEID_STRING_LENGTH] = {0};
  char* deviceTypeString;
  uint32_t timeSinceLastMessage = halCommonGetInt32uMillisecondTick();
  EmberAfPluginDeviceTableEntry *deviceTable = emberAfDeviceTablePointer();

  timeSinceLastMessage -= deviceTable[nodeIndex].lastMsgTimestamp;
  timeSinceLastMessage /= MILLISECOND_TICKS_PER_SECOND;

  nodeJson = cJSON_CreateObject();

  nodeIdToString(deviceTable[nodeIndex].nodeId,
                 nodeIdString);
  cJSON_AddStringToObject(nodeJson,
                          "nodeId",
                          nodeIdString);

  cJSON_AddIntegerToObject(nodeJson,
                           "deviceState",
                           deviceTable[nodeIndex].state);

  deviceTypeString = createTwoByteHexString(deviceTable[nodeIndex].deviceId);
  cJSON_AddStringToObject(nodeJson,
                          "deviceType",
                          deviceTypeString);
  free(deviceTypeString);

  cJSON_AddIntegerToObject(nodeJson,
                           "timeSinceLastMessage",
                           timeSinceLastMessage);

  deviceEndpoint = buildDeviceEndpoint(deviceTable[nodeIndex].eui64,
                                       deviceTable[nodeIndex].endpoint);
  cJSON_AddItemToObject(nodeJson,
                        "deviceEndpoint",
                        deviceEndpoint);
  return nodeJson;
}

static void publishMqttRelays(void)
{
  cJSON* json;
  char* jsonStr;
  char* topic;
  cJSON* itemJson;
  cJSON* itemsJsonArray;
  cJSON* deviceEndpointJson;
  uint16_t i;

  EmberAfPluginCommandRelayEntry *relayTable = emberAfPluginCommandRelayTablePointer();

  json = cJSON_CreateObject();
  itemsJsonArray = cJSON_CreateArray();
  cJSON_AddItemToObject(json, "relays", itemsJsonArray);

  for (i = 0; i < EMBER_AF_PLUGIN_COMMAND_RELAY_RELAY_TABLE_SIZE; i++) {
    if (isNullEui(relayTable[i].inDeviceEndpoint.eui64)) {
      continue;
    }

    itemJson = cJSON_CreateObject();
    deviceEndpointJson = buildDeviceEndpoint(relayTable[i].inDeviceEndpoint.eui64,
                                             relayTable[i].inDeviceEndpoint.endpoint);
    cJSON_AddItemToObject(itemJson, "inDeviceEndpoint", deviceEndpointJson);

    deviceEndpointJson = buildDeviceEndpoint(relayTable[i].outDeviceEndpoint.eui64,
                                             relayTable[i].outDeviceEndpoint.endpoint);
    cJSON_AddItemToObject(itemJson, "outDeviceEndpoint", deviceEndpointJson);

    cJSON_AddItemToArray(itemsJsonArray, itemJson);
  }

  jsonStr = cJSON_PrintUnformatted(json);
  topic = allocateAndFormMqttGatewayTopic("relays");
  emberAfPluginTransportMqttPublish(topic, jsonStr);

  free(jsonStr);
  cJSON_Delete(json);
  free(topic);
}

static void publishMqttSettings(void)
{
  char* topic = allocateAndFormMqttGatewayTopic("settings");
  EmberNetworkParameters parameters;
  EmberNodeType nodeType;
  cJSON* settingsJson;
  char* settingsJsonString;
  char ncpStackVerString[HOSTVER_STRING_LENGTH] = {0};
  EmberVersion versionStruct;
  uint8_t ncpEzspProtocolVer;
  uint8_t ncpStackType;
  uint16_t ncpStackVer;
  char* panIdString;
  uint8_t hostEzspProtocolVer = EZSP_PROTOCOL_VERSION;

  EmberStatus status = emberAfGetNetworkParameters(&nodeType, &parameters);

  ncpEzspProtocolVer = ezspVersion(hostEzspProtocolVer,
                                   &ncpStackType,
                                   &ncpStackVer);

  if (EZSP_SUCCESS == ezspGetVersionStruct(&versionStruct)) {
    sprintf(ncpStackVerString,
            "%d.%d.%d-%d",
            versionStruct.major,
            versionStruct.minor,
            versionStruct.patch,
            versionStruct.build);
  } else {
    sprintf(ncpStackVerString, "%2x",ncpStackVer);
  }

  settingsJson = cJSON_CreateObject();
  cJSON_AddStringToObject(settingsJson, "ncpStackVersion", ncpStackVerString);
  if (!emberAfNcpNeedsReset() && status == EMBER_SUCCESS) {
    cJSON_AddTrueToObject(settingsJson, "networkUp");
    panIdString = createTwoByteHexString(parameters.panId);
    cJSON_AddStringToObject(settingsJson, "networkPanId", panIdString);
    free(panIdString);
    cJSON_AddIntegerToObject(settingsJson,
                             "radioTxPower",
                             parameters.radioTxPower + 22);
    cJSON_AddIntegerToObject(settingsJson,
                             "radioChannel",
                             parameters.radioChannel);
  } else {
    cJSON_AddFalseToObject(settingsJson, "networkUp");
  }
  settingsJsonString = cJSON_PrintUnformatted(settingsJson);
  emberAfPluginTransportMqttPublish(topic, settingsJsonString);
  free(settingsJsonString);
  cJSON_Delete(settingsJson);
  free(topic);
}

static void publishMqttGatewayState(void)
{
  // Set an event to publish all the state updates so that they will be in scope
  // of the stack
  emberEventControlSetActive(stateUpdateEventControl);
}

void stateUpdateEventFunction(void)
{
  emberEventControlSetInactive(stateUpdateEventControl);
  publishMqttSettings();
  publishMqttRelays();
  publishMqttDevices();
}

static void publishMqttDeviceStateChange(EmberEUI64 eui64,
                                         uint8_t state)
{
  char* topic = allocateAndFormMqttGatewayTopic("devicestatechange");
  char euiString[EUI64_STRING_LENGTH] = {0};
  cJSON* stateChangeJson;
  char* stateChangeJsonString;

  eui64ToString(eui64, euiString);

  stateChangeJson = cJSON_CreateObject();
  cJSON_AddStringToObject(stateChangeJson, "eui64", euiString);
  cJSON_AddIntegerToObject(stateChangeJson, "deviceState", state);
  stateChangeJsonString = cJSON_PrintUnformatted(stateChangeJson);
  emberAfPluginTransportMqttPublish(topic, stateChangeJsonString);
  free(stateChangeJsonString);
  cJSON_Delete(stateChangeJson);
  free(topic);
}

static void publishMqttDeviceJoined(EmberEUI64 eui64)
{
  char* topic = allocateAndFormMqttGatewayTopic("devicejoined");
  uint16_t deviceTableIndex;
  cJSON* nodeJson;
  char* nodeJsonString;

  deviceTableIndex = emberAfDeviceTableGetFirstIndexFromEui64(eui64);
  nodeJson = buildNodeJson(deviceTableIndex);

  nodeJsonString = cJSON_PrintUnformatted(nodeJson);
  emberAfPluginTransportMqttPublish(topic, nodeJsonString);
  free(nodeJsonString);
  cJSON_Delete(nodeJson);
  free(topic);
}

static void publishMqttDeviceLeft(EmberEUI64 eui64)
{
  char* topic = allocateAndFormMqttGatewayTopic("deviceleft");
  char euiString[EUI64_STRING_LENGTH] = {0};
  cJSON* nodeLeftJson;
  char* nodeLeftJsonString;

  eui64ToString(eui64, euiString);

  nodeLeftJson = cJSON_CreateObject();
  cJSON_AddStringToObject(nodeLeftJson, "eui64", euiString);
  nodeLeftJsonString = cJSON_PrintUnformatted(nodeLeftJson);
  emberAfPluginTransportMqttPublish(topic, nodeLeftJsonString);
  free(nodeLeftJsonString);
  cJSON_Delete(nodeLeftJson);
  free(topic);
}

static void publishMqttAttribute(EmberEUI64 eui64,
                                 EmberAfClusterId clusterId,
                                 uint8_t* buffer,
                                 uint16_t bufLen)
{
  char* topicZcl = allocateAndFormMqttGatewayTopic(ZCL_RESPONSE_TOPIC);
  uint16_t bufferIndex;
  char clusterIdString[CLUSTERID_STRING_LENGTH] = {0};
  char attribString[TWO_BYTE_HEX_STRING_SIZE] = {0};
  char dataTypeString[ONE_BYTE_HEX_STRING_SIZE] = {0};
  char* statusString;
  cJSON* globalReadJson;
  cJSON* deviceEndpointJson;
  char* globalReadJsonString;
  uint16_t bufferStringLength = ((2 * bufLen) + HEX_TOKEN_SIZE + 1); // "0x" + 2 chars per byte + null char
  char* bufferString = (char*)malloc(bufferStringLength);
  uint8_t sourceEndpoint;

  memset(bufferString, 0, bufferStringLength);

  if (bufLen > 0) {
    sprintf(&bufferString[0], "0x");
  }

  // Print buffer data as a hex string, starting at the data start byte
  for (bufferIndex = ATTRIBUTE_BUFFER_DATA_START;
       bufferIndex < bufLen;
       bufferIndex++) {
    sprintf(&bufferString[(2 * (bufferIndex - ATTRIBUTE_BUFFER_DATA_START)) + HEX_TOKEN_SIZE],
            "%02X",
            buffer[bufferIndex]);
  }

  sprintf(clusterIdString, "0x%04X", clusterId);
  sprintf(attribString,
          "0x%02X%02X",
          buffer[ATTRIBUTE_BUFFER_CLUSTERID_HIGH_BITS],
          buffer[ATTRIBUTE_BUFFER_CLUSTERID_LOW_BITS]);
  sprintf(dataTypeString, "0x%02X", buffer[ATTRIBUTE_BUFFER_DATA_TYPE]);

  globalReadJson = cJSON_CreateObject();
  cJSON_AddStringToObject(globalReadJson, "clusterId", clusterIdString);
  cJSON_AddStringToObject(globalReadJson, "attributeId", attribString);
  cJSON_AddStringToObject(globalReadJson, "attributeBuffer", bufferString);
  cJSON_AddStringToObject(globalReadJson, "attributeDataType", dataTypeString);

  sourceEndpoint = emberAfCurrentCommand()->apsFrame->sourceEndpoint;
  deviceEndpointJson = buildDeviceEndpoint(eui64, sourceEndpoint);
  cJSON_AddItemToObject(globalReadJson, "deviceEndpoint", deviceEndpointJson);

  statusString = createOneByteHexString(buffer[ATTRIBUTE_BUFFER_SUCCESS_CODE]);
  cJSON_AddStringToObject(globalReadJson, "status", statusString);
  free(statusString);

  globalReadJsonString = cJSON_PrintUnformatted(globalReadJson);
  emberAfPluginTransportMqttPublish(topicZcl, globalReadJsonString);
  free(globalReadJsonString);
  free(bufferString);
  cJSON_Delete(globalReadJson);
  free(topicZcl);
}

static void publishMqttAttributeReport(EmberEUI64 eui64,
                                       EmberAfClusterId clusterId,
                                       uint8_t* buffer,
                                       uint16_t bufLen)
{
  char* topicZcl = allocateAndFormMqttGatewayTopic(ZCL_RESPONSE_TOPIC);
  uint16_t bufferIndex;
  char euiString[EUI64_STRING_LENGTH] = {0};
  char clusterIdString[CLUSTERID_STRING_LENGTH] = {0};
  char attribString[TWO_BYTE_HEX_STRING_SIZE] = {0}; // "0x" + 4 chars + null char
  char dataTypeString[ONE_BYTE_HEX_STRING_SIZE] = {0}; // "0x" + 2 chars + null char
  cJSON* globalReadJson;
  cJSON* deviceEndpointJson;
  char* globalReadJsonString;
  uint16_t bufferStringLength = (2 * bufLen) + HEX_TOKEN_SIZE + 1; // "0x" + 2 chars per byte + null char
  char* bufferString = (char*)malloc(bufferStringLength);
  memset(bufferString, 0, bufferStringLength);
  uint8_t sourceEndpoint;

  if (bufLen > 0) {
    sprintf(&bufferString[0], "0x");
  }

  // Print buffer data as a hex string, starting at the data start byte
  for (bufferIndex = ATTRIBUTE_BUFFER_REPORT_DATA_START;
       bufferIndex < bufLen;
       bufferIndex++) {
    sprintf(&bufferString[2 * (bufferIndex - ATTRIBUTE_BUFFER_REPORT_DATA_START) + HEX_TOKEN_SIZE],
            "%02X",
            buffer[bufferIndex]);
  }

  eui64ToString(eui64, euiString);
  sprintf(clusterIdString, "0x%04X", clusterId);
  sprintf(attribString,
          "0x%02X%02X",
          buffer[ATTRIBUTE_BUFFER_CLUSTERID_HIGH_BITS],
          buffer[ATTRIBUTE_BUFFER_CLUSTERID_LOW_BITS]);
  sprintf(dataTypeString, "0x%02X", buffer[ATTRIBUTE_BUFFER_REPORT_DATA_TYPE]);

  globalReadJson = cJSON_CreateObject();
  cJSON_AddStringToObject(globalReadJson, "clusterId", clusterIdString);
  cJSON_AddStringToObject(globalReadJson, "attributeId", attribString);
  cJSON_AddStringToObject(globalReadJson, "attributeBuffer", bufferString);
  cJSON_AddStringToObject(globalReadJson, "attributeDataType", dataTypeString);

  sourceEndpoint = emberAfCurrentCommand()->apsFrame->sourceEndpoint;
  deviceEndpointJson = buildDeviceEndpoint(eui64, sourceEndpoint);
  cJSON_AddItemToObject(globalReadJson, "deviceEndpoint", deviceEndpointJson);

  globalReadJsonString = cJSON_PrintUnformatted(globalReadJson);
  emberAfPluginTransportMqttPublish(topicZcl, globalReadJsonString);
  free(globalReadJsonString);
  free(bufferString);
  cJSON_Delete(globalReadJson);
}

static void publishMqttApsStatus(EmberStatus status,
                                 EmberAfClusterId clusterId,
                                 uint8_t commandId,
                                 uint16_t indexOrDestination)
{
  char* topic = allocateAndFormMqttGatewayTopic(APS_RESPONSE_TOPIC);
  cJSON* defaultResponseJson;
  char* defaultResponseString;
  char* stringData;
  char* statusString;
  bool squelchMessage = FALSE;
  EmberEUI64 eui64;
  char euiString[EUI64_STRING_LENGTH] = {0};

  defaultResponseJson = cJSON_CreateObject();

  cJSON_AddStringToObject(defaultResponseJson, "statusType", "apsAck");

  emberAfDeviceTableGetEui64FromNodeId((EmberNodeId)indexOrDestination, eui64);
  eui64ToString(eui64, euiString);
  cJSON_AddStringToObject(defaultResponseJson, "eui64", euiString);

  statusString = createOneByteHexString(status);
  cJSON_AddStringToObject(defaultResponseJson, "status", statusString);
  free(statusString);

  stringData = createTwoByteHexString(clusterId);
  cJSON_AddStringToObject(defaultResponseJson, "clusterId", stringData);
  free(stringData);

  stringData = createOneByteHexString(commandId);
  cJSON_AddStringToObject(defaultResponseJson, "commandId", stringData);
  free(stringData);

  if (clusterId == ZCL_OTA_BOOTLOAD_CLUSTER_ID &&
      commandId == ZCL_WRITE_ATTRIBUTES_NO_RESPONSE_COMMAND_ID) {
    squelchMessage = TRUE;
  }

  defaultResponseString = cJSON_PrintUnformatted(defaultResponseJson);
  if (!squelchMessage) {
    emberAfPluginTransportMqttPublish(topic, defaultResponseString);
  }
  free(defaultResponseString);
  cJSON_Delete(defaultResponseJson);
  free(topic);
}

static void publishMqttTrafficReportEvent(char* messageType,
                                          EmberStatus* status,
                                          int8_t* lastHopRssi,
                                          uint8_t* lastHopLqi,
                                          uint8_t* zclSequenceNumber,
                                          uint64_t timeMS)
{
  char* topic = allocateAndFormMqttGatewayTopic("trafficreportevent");
  cJSON* trafficReportJson;
  char* trafficReportJsonString;
  char* statusString;
  char timeString[20] = {0}; // 20 character maximum for in64u, including null

  sprintf(timeString, "%llu", timeMS);

  trafficReportJson = cJSON_CreateObject();
  cJSON_AddStringToObject(trafficReportJson, "messageType", messageType);

  if (status) {
    statusString = createOneByteHexString(*status);
    cJSON_AddStringToObject(trafficReportJson, "status", statusString);
    free(statusString);
  }

  if (lastHopRssi) {
    cJSON_AddIntegerToObject(trafficReportJson, "rssi", *lastHopRssi);
  }
  if (lastHopLqi) {
    cJSON_AddIntegerToObject(trafficReportJson, "linkQuality", *lastHopLqi);
  }
  if (zclSequenceNumber) {
    cJSON_AddIntegerToObject(trafficReportJson,
                             "sequenceNumber",
                             *zclSequenceNumber);
  }
  cJSON_AddStringToObject(trafficReportJson, "currentTimeMs", timeString);
  trafficReportJsonString = cJSON_PrintUnformatted(trafficReportJson);
  emberAfPluginTransportMqttPublish(topic, trafficReportJsonString);
  free(trafficReportJsonString);
  cJSON_Delete(trafficReportJson);
  free(topic);
}

static void publishMqttOtaEvent(char* messageType,
                                EmberEUI64 eui64,
                                uint8_t* status,
                                uint32_t* blocksSent,
                                uint8_t* blockSize,
                                uint16_t* manufacturerId,
                                uint16_t* imageTypeId,
                                uint32_t* firmwareVersion)
{
  char* topic = allocateAndFormMqttGatewayTopic("otaevent");
  char euiString[EUI64_STRING_LENGTH] = {0};
  char manufacturerIdString[TWO_BYTE_HEX_STRING_SIZE] = {0};
  char imageTypeIdString[TWO_BYTE_HEX_STRING_SIZE] = {0};
  char firmwareVersionString[FOUR_BYTE_HEX_STRING_SIZE] = {0};
  cJSON* otaJson;
  char* statusString;
  char* otaJsonString;

  eui64ToString(eui64, euiString);

  otaJson = cJSON_CreateObject();
  cJSON_AddStringToObject(otaJson, "messageType", messageType);
  cJSON_AddStringToObject(otaJson, "eui64", euiString);
  if (status) {
    statusString = createOneByteHexString(*status);
    cJSON_AddStringToObject(otaJson, "status", statusString);
    free(statusString);
  }
  if (blocksSent && blockSize) {
    cJSON_AddIntegerToObject(otaJson,
                             "bytesSent",
                             (*blocksSent) * (*blockSize));
  }

  if (manufacturerId) {
    sprintf(manufacturerIdString, "0x%04X", *manufacturerId);
    cJSON_AddStringToObject(otaJson, "manufacturerId", manufacturerIdString);
  }
  if (imageTypeId) {
    sprintf(imageTypeIdString, "0x%04X", *imageTypeId);
    cJSON_AddStringToObject(otaJson, "imageTypeId", imageTypeIdString);
  }
  if (firmwareVersion) {
    sprintf(firmwareVersionString, "0x%08X", *firmwareVersion);
    cJSON_AddStringToObject(otaJson, "firmwareVersion", firmwareVersionString);
  }
  otaJsonString = cJSON_PrintUnformatted(otaJson);
  emberAfPluginTransportMqttPublish(topic, otaJsonString);
  free(otaJsonString);
  cJSON_Delete(otaJson);
  free(topic);
}

static void publishMqttCommandExecuted(char* cliCommand)
{
  char* topic = allocateAndFormMqttGatewayTopic("executed");
  cJSON* executedJson;
  char* executedJsonString;

  executedJson = cJSON_CreateObject();
  cJSON_AddStringToObject(executedJson, "command", cliCommand);
  executedJsonString = cJSON_PrintUnformatted(executedJson);
  emberAfPluginTransportMqttPublish(topic, executedJsonString);
  free(executedJsonString);
  cJSON_Delete(executedJson);
  free(topic);
}

static void publishMqttDelayExecuted(uint32_t postDelayMs)
{
  char* topic = allocateAndFormMqttGatewayTopic("executed");
  cJSON* executedJson;
  char* executedJsonString;

  executedJson = cJSON_CreateObject();
  cJSON_AddIntegerToObject(executedJson, "delayMs", postDelayMs);
  executedJsonString = cJSON_PrintUnformatted(executedJson);
  emberAfPluginTransportMqttPublish(topic, executedJsonString);
  free(executedJsonString);
  cJSON_Delete(executedJson);
  free(topic);
}

static void publishMqttBindResponse(EmberNodeId nodeId,
                                    EmberApsFrame* apsFrame,
                                    uint8_t* message,
                                    uint16_t length)
{
  char* topic = allocateAndFormMqttGatewayTopic(ZDO_RESPONSE_TOPIC);
  cJSON* objectJson;
  char* objectJsonString;
  char* dataString;
  EmberEUI64 eui64;
  char euiString[EUI64_STRING_LENGTH] = {0};

  objectJson = cJSON_CreateObject();

  cJSON_AddStringToObject(objectJson, "zdoType", "bindResponse");

  emberAfDeviceTableGetEui64FromNodeId(nodeId, eui64);
  eui64ToString(eui64, euiString);
  cJSON_AddStringToObject(objectJson, "eui64", euiString);

  dataString = createOneByteHexString(message[DEVICE_TABLE_BIND_RESPONSE_STATUS]);
  cJSON_AddStringToObject(objectJson, "status", dataString);
  free(dataString);

  objectJsonString =
    cJSON_PrintUnformatted(objectJson);
  emberAfPluginTransportMqttPublish(topic, objectJsonString);
  free(objectJsonString);
  cJSON_Delete(objectJson);
  free(topic);
}

static void publishMqttBindTableReponse(EmberNodeId nodeId,
                                        EmberApsFrame* apsFrame,
                                        uint8_t* message,
                                        uint16_t length)
{
  char* topic = allocateAndFormMqttGatewayTopic(ZDO_RESPONSE_TOPIC);
  cJSON* objectJson;
  cJSON* entryArrayJson;
  cJSON* tableEntryJson;
  cJSON* deviceEndpointJson;
  char* objectJsonString;
  uint8_t* messagePointer;
  uint8_t numEntries, entryCounter;
  char* dataString;
  char euiString[EUI64_STRING_LENGTH] = {0};
  EmberEUI64 eui64;

  numEntries = message[BINDING_TABLE_RESPONSE_NUM_ENTRIES]; // list count

  objectJson = cJSON_CreateObject();
  entryArrayJson = cJSON_CreateArray();

  cJSON_AddStringToObject(objectJson, "zdoType", "bindTableResponse");

  dataString = createOneByteHexString(message[BINDING_TABLE_RESPONSE_STATUS]);
  cJSON_AddStringToObject(objectJson, "status", dataString);
  free(dataString);

  emberAfDeviceTableGetEui64FromNodeId(nodeId, eui64);
  eui64ToString(eui64, euiString);
  cJSON_AddStringToObject(objectJson, "eui64", euiString);

  messagePointer = message + BINDING_TABLE_RESPONSE_ENTRIES;

  for (entryCounter = 0; entryCounter < numEntries; entryCounter++) {
    tableEntryJson = cJSON_CreateObject();

    deviceEndpointJson = buildDeviceEndpoint(&(messagePointer[BINDING_ENTRY_EUI]),
                                             messagePointer[BINDING_ENTRY_SOURCE_ENDPOINT]);
    cJSON_AddItemToObject(tableEntryJson,
                          "sourceDeviceEndpoint",
                          deviceEndpointJson);

    cJSON_AddIntegerToObject(tableEntryJson,
                             "addressMode",
                             messagePointer[BINDING_ENTRY_ADDRESS_MODE]);

    dataString =
      createTwoByteHexString(
        HIGH_LOW_TO_INT(messagePointer[BINDING_ENTRY_CLUSTER_ID+1],
                        messagePointer[BINDING_ENTRY_CLUSTER_ID]));
    cJSON_AddStringToObject(tableEntryJson, "clusterId", dataString);
    free(dataString);

    deviceEndpointJson = buildDeviceEndpoint(&(messagePointer[BINDING_ENTRY_DEST_EUI]),
                                             messagePointer[BINDING_ENTRY_DEST_ENDPOINT]);
    cJSON_AddItemToObject(tableEntryJson,
                          "destDeviceEndpoint",
                          deviceEndpointJson);

    cJSON_AddItemToArray(entryArrayJson, tableEntryJson);

    messagePointer += BINDING_TABLE_RESPONSE_ENTRY_SIZE;
  }

  cJSON_AddItemToObject(objectJson, "bindTable", entryArrayJson);

  objectJsonString = cJSON_PrintUnformatted(objectJson);
  emberAfPluginTransportMqttPublish(topic, objectJsonString);
  free(objectJsonString);
  cJSON_Delete(objectJson);
  free(topic);
}



// OTA Callbacks
void emberAfPluginOtaServerUpdateCompleteCallback(uint16_t manufacturerId,
                                                  uint16_t imageTypeId,
                                                  uint32_t firmwareVersion,
                                                  EmberNodeId nodeId,
                                                  uint8_t status)
{
  char* messageType = (status == EMBER_ZCL_STATUS_SUCCESS) ? "otaFinished"
                                                           : "otaFailed";

  EmberEUI64 nodeEui64;
  emberAfDeviceTableGetEui64FromNodeId(nodeId, nodeEui64);

  publishMqttOtaEvent(messageType,
                      nodeEui64,
                      &status,
                      NULL, // blockSent is unused
                      NULL, // actualLength is unused
                      &manufacturerId,
                      &imageTypeId,
                      &firmwareVersion);


  otaBlockSent = 0; // Note that this global block sent count only supports 1 OTA
}

void emberAfPluginOtaServerBlockSentCallback(uint8_t actualLength,
                                             uint16_t manufacturerId,
                                             uint16_t imageTypeId,
                                             uint32_t firmwareVersion)
{
  // Use a throttle value here to control the amount of updates being published
  if (otaBlockSent % BLOCK_SENT_THROTTLE_VALUE == 0) {
    EmberNodeId nodeId = emberAfCurrentCommand()->source;
    EmberEUI64 nodeEui64;
    emberAfDeviceTableGetEui64FromNodeId(nodeId, nodeEui64);
    publishMqttOtaEvent("otaBlockSent",
                        nodeEui64,
                        NULL, // status is unused
                        &otaBlockSent,
                        &actualLength,
                        &manufacturerId,
                        &imageTypeId,
                        &firmwareVersion);
  }

  otaBlockSent++;
}

void emberAfPluginOtaServerUpdateStartedCallback(uint16_t manufacturerId,
                                                 uint16_t imageTypeId,
                                                 uint32_t firmwareVersion,
                                                 uint8_t maxDataSize,
                                                 uint32_t offset)
{
  otaBlockSent = 0; // Note that this global block sent count only supports 1 OTA
  EmberNodeId nodeId = emberAfCurrentCommand()->source;
  EmberEUI64 nodeEui64;
  emberAfDeviceTableGetEui64FromNodeId(nodeId, nodeEui64);
  publishMqttOtaEvent("otaStarted",
                      nodeEui64,
                      NULL, // status is unused
                      NULL, // blockSent is unused
                      NULL, // actualLength is unused
                      &manufacturerId,
                      &imageTypeId,
                      &firmwareVersion);
}

// Non-cluster Callbacks
bool emberAfMessageSentCallback(EmberOutgoingMessageType type,
                                uint16_t indexOrDestination,
                                EmberApsFrame* apsFrame,
                                uint16_t msgLen,
                                uint8_t* message,
                                EmberStatus status)
{
	//ZDO Loop Problem
	if(status == 0x66 && (apsFrame->clusterId == 0x0001 || apsFrame->clusterId == 0x0000 || apsFrame-> clusterId == 0x0005))
	{
		emberAfAppPrintln("<Transmission Failed    Node: 0x%2x  Cmd: %x", indexOrDestination, message[2]);
		//return true;
		return false;
	}


	if (trafficReporting) {
	// This specifically uses the emberAfIncomingZclSequenceNumber instead
	// of the adjusted sequence number used in emberAfPreMessageReceivedCallback
	// and emberAfPreMessageSendCallback
	publishMqttTrafficReportEvent("messageSent",
								  &status,
								  NULL, // rssi unused
								  NULL, // lqi unused
								  NULL,
								  halCommonGetInt32uMillisecondTick());
	}
	publishMqttApsStatus(status,
					   apsFrame->clusterId,
					   message[COMMAND_OFFSET],
					   indexOrDestination);

	if (status != EMBER_SUCCESS) {
		emberAfAppPrintln("< Node Id: 0x%2x failed with code %x >",
					  indexOrDestination,
					  status);

		if (indexOrDestination >= EMBER_DISCOVERY_ACTIVE_NODE_ID) {
			return false;
		}
	}

	// track the state of the device, except for broadcasts
	if (emberIsZigbeeBroadcastAddress(indexOrDestination)) {
		return false;
	}

	emberAfPluginDeviceTableMessageSentStatus(indexOrDestination, status);

	return false;
}

bool emberAfPreMessageReceivedCallback(EmberAfIncomingMessage* incomingMessage)
{
	EmberEUI64 nodeEui64;
	EmberNodeId nodeId = incomingMessage->source;
	emberAfDeviceTableGetEui64FromNodeId(nodeId, nodeEui64);
	// CYJ
	deviceMessage = incomingMessage;
	uint8_t cmd = incomingMessage->message[2];
	int8_t rssi = incomingMessage->lastHopRssi;
	uint8_t lqi = incomingMessage->lastHopLqi;
	uint16_t cluId = incomingMessage->apsFrame->clusterId;
	
	if(cluId == 0xFA00 || nodeId != 0x0000)
	{
		switch(cmd)
		{
			case 0xFA :
			{
				emberAfAppPrintln("*** Welcome new device (0x%2x), Setting Done ***", nodeId);
				sendUnicastSettingUpReadyForZE(nodeId);
				return true;
				break;
			}
		}
	}
	
	if(cluId == 0xFF00 || nodeId != 0x0000)
	{
		switch(cmd)
		{
			case 0xFE :
			{
				emberAfAppPrintln("*** <Error data> Src 0x%2x Cmd %x ErrorStatus 0x%x LQI %d"
								, nodeId, cmd, incomingMessage->message[4], lqi);
				return true;
				break;
			}
			case 0xFA :
			{
				emberAfAppPrintln("*** Welcome new device (0x%2x), Setting Done ***", nodeId);
				sendUnicastSettingUpReadyForZE(nodeId);
				return true;
				break;
			}
			case 0xDF :
			{
				emberAfAppPrintln("---- Signal Test [node: 0x%x] [LQI: %d] [RSSI: %d]", nodeId, lqi, rssi);
				return true;
				break;
			}
			case 0xDA :
			{
				emberAfAppPrint("*** Received ");
				//uint8_t fc = incomingMessage->message[0];
				//uint8_t seq = incomingMessage->message[1];
				uint8_t nodeType = incomingMessage->message[3];
				uint8_t sensorType = incomingMessage->message[4];
				uint16_t transectionId = getInt16FromMessage(incomingMessage->message,5);
				uint32_t time = getInt32FromMessage(incomingMessage->message, 7);
				uint8_t errorCount = incomingMessage->message[11];
				uint8_t nwkDownCount = incomingMessage->message[12];
				
				switch(sensorType)
				{
					case 0 :	// None
					{
						emberAfAppPrintln("[No Sensor] Cmd %x NT %x ST %x TID %x Time %d EC %d NC %d LQI %d RSSI %d"
						, cmd, nodeType, sensorType, transectionId, time, errorCount, nwkDownCount, lqi, rssi);
						publishMqttBasicData(nodeEui64, incomingMessage->message, nodeId);
						return true;
						break;
					}
					case 1 :	// Si7021
					{
						int32_t temperatureMilliC = 0;
						int32_t temp = HIGH_LOW_TO_INT(incomingMessage->message[13],incomingMessage->message[14]);
						temp = 5624 * temp;
						temp = temp >> 13;
						temp = temp - 11994;
						temp = (temp * 1000) >> 8;
						temperatureMilliC = temp;
						emberAfAppPrintln("[Si7021 Data] Cmd %x NT %x ST %x TID %x Time %d EC %d NC %d Temperature %d"
						, cmd, nodeType, sensorType, transectionId, time, errorCount, nwkDownCount, temperatureMilliC);
						// MQTT Publish
						publishMqttTempData(nodeEui64, incomingMessage->message, nodeId, temperatureMilliC);
						return true;
						break;
					}
					case 2 :	// Gy273
					{
						int16_t gyro_X = HIGH_LOW_TO_INT(incomingMessage->message[13],incomingMessage->message[14]);
						int16_t gyro_Y = HIGH_LOW_TO_INT(incomingMessage->message[15],incomingMessage->message[16]);
						int16_t gyro_Z = HIGH_LOW_TO_INT(incomingMessage->message[17],incomingMessage->message[18]);
						emberAfAppPrintln("[Gy273 Data] Cmd %x NT %x ST %x TID %x Time %d EC %d NC %d XYZ (%d,%d,%d)"
						, cmd, nodeType, sensorType, transectionId, time, errorCount, nwkDownCount, gyro_X, gyro_Y, gyro_Z);
						// MQTT Publish
						publishMqttGyroData(nodeEui64, incomingMessage->message, nodeId, gyro_X, gyro_Y, gyro_Z);
						return true;
						break;
					}
					case 3 :	// Gy45
					{
						int16s gyro_X = incomingMessage->message[13] << 6;
						int16s gyro_Y = incomingMessage->message[14] << 6;
						int16s gyro_Z = incomingMessage->message[15] << 6;
						if(gyro_X > 8191) gyro_X = (gyro_X - 16384)/4;
						if(gyro_Y > 8191) gyro_Y = (gyro_Y - 16384)/4;
						if(gyro_Z > 8191) gyro_Z = (gyro_Z - 16384)/4;
						emberAfAppPrintln("[Gy45 Data] Cmd %x NT %x ST %x TID %x Time %d EC %d NC %d XYZ (%d,%d,%d)"
						, cmd, nodeType, sensorType, transectionId, time, errorCount, nwkDownCount, gyro_X, gyro_Y, gyro_Z);
						// MQTT Publish
						publishMqttGyroData(nodeEui64, incomingMessage->message, nodeId, gyro_X, gyro_Y, gyro_Z);
						return true;
						break;
					}
					case 4 :	// Lmv324 + Si7021
					{
						uint16_t readNoiseValue = ((incomingMessage->message[13] << 8) |  incomingMessage->message[14])>>4;
						int32_t temp = HIGH_LOW_TO_INT(incomingMessage->message[15],incomingMessage->message[16]);
						int32_t temperatureMilliC = 0;
						temp = 5624 * temp;
						temp = temp >> 13;
						temp = temp - 11994;
						temp = (temp * 1000) >> 8;
						temperatureMilliC = temp;
						emberAfAppPrintln("[Lmv324,Si7021 Data] Cmd %x NT %x ST %x TID %x Time %d EC %d NC %d Noise %d, Temperature %d"
						, cmd, nodeType, sensorType, transectionId, time, errorCount, nwkDownCount,readNoiseValue, temperatureMilliC);
						// MQTT Publish
						publishMqttSoundData(nodeEui64, incomingMessage->message, nodeId, readNoiseValue,temperatureMilliC);
						return true;
						break;
					}
				}
				break;
			}
		}
	}
	/* old version
	if(incomingMessage->msgLen >= 3)
	{
		uint8_t cmd = incomingMessage->message[2];
		int8_t rssi = incomingMessage->lastHopRssi;
		uint8_t lqi = incomingMessage->lastHopLqi;

		if(cmd != 0xBA && nodeId != 0x0000)
		{
			//emberAfAppPrintln("*** <All> Pre message received callback - cmd id(0x%x) - len(%d) - src(0x%2x) - LQI(%d) ***", cmd, msg_len, source, lqi);
			// Parse Command
			if(cmd == 0xFE){	// Report attribute message
				emberAfAppPrintln("*** <Error data> Src 0x%2x Cmd %x ErrorStatus 0x%x LQI %d", incomingMessage->nodeId, cmd, incomingMessage->message[4], lqi);
				return true;
			}
			else if(cmd == 0xFA && nodeId != 0x0000){	// Reset or newcomer message
				emberAfAppPrintln("*** Welcome new device (0x%2x), reset setting.. ***",incomingMessage->nodeId);
				// Add more function for reset
				emberAfAppPrintln("*********! Reset Done !*********",incomingMessage->nodeId);
				sendUnicastSettingUpReadyForZE(incomingMessage->nodeId);
				sendUnicastSettingUpReadyForZE(incomingMessage->nodeId);
				return true;
			}
			else if(cmd == 0xDF && nodeId != 0x0000){
				emberAfAppPrintln("---- Signal Test [node: 0x%x] [LQI: %d] [RSSI: %d]", nodeId, lqi, rssi);
				return true;
			}
			else if(cmd == 0xDA && nodeId != 0x0000){
				emberAfAppPrint("*** Received data");
				uint8_t sensorType = incomingMessage->message[4];
				if(sensorType == 1)
				{
					int32_t temp = HIGH_LOW_TO_INT(incomingMessage->message[13],incomingMessage->message[14]);
					int32_t temperatureMilliC = 0;
					temp = 5624 * temp;
					temp = temp >> 13;
					temp = temp - 11994;
					temp = (temp * 1000) >> 8;
					temperatureMilliC = temp;
					emberAfAppPrintln(" Cmd %x NodeType %x SensorType %x TranjectionID %x Time %d ErrorCount %d nwkDownCount %d Temp %d"
						, incomingMessage->message[2], incomingMessage->message[3], incomingMessage->message[4]
						, getInt16FromMessage(incomingMessage->message,5), getInt32FromMessage(incomingMessage->message,7)
						, incomingMessage->message[11], incomingMessage->message[12], temperatureMilliC);
					publishMqttTempData(nodeEui64, incomingMessage->message, incomingMessage->nodeId, temperatureMilliC);
				}
				else if(sensorType == 2 && nodeId != 0x0000)
				{
					//int16_t gyro_X = getInt16FromMessage(incomingMessage->message, 13);
					//int16_t gyro_Y = getInt16FromMessage(incomingMessage->message, 15);
					//int16_t gyro_Z = getInt16FromMessage(incomingMessage->message, 17);
					int16_t gyro_X = HIGH_LOW_TO_INT(incomingMessage->message[13],incomingMessage->message[14]);
					int16_t gyro_Y = HIGH_LOW_TO_INT(incomingMessage->message[15],incomingMessage->message[16]);
					int16_t gyro_Z = HIGH_LOW_TO_INT(incomingMessage->message[17],incomingMessage->message[18]);

					emberAfAppPrintln(" Cmd %x NodeType %x SensorType %x TranjectionID %x Time %d ErrorCount %d nwkDownCount %d GyroX %d GyroY %d GyroZ %d"
						, incomingMessage->message[2], incomingMessage->message[3], incomingMessage->message[4]
						, getInt16FromMessage(incomingMessage->message,5), getInt32FromMessage(incomingMessage->message,7)
						, incomingMessage->message[11], incomingMessage->message[12], gyro_X, gyro_Y, gyro_Z);

					publishMqttGyroData(nodeEui64, incomingMessage->message, incomingMessage->nodeId, gyro_X, gyro_Y, gyro_Z);

				}
				else if(sensorType == 3 && nodeId != 0x0000)
				{
					int16s gyro_X = incomingMessage->message[13] << 6;
					int16s gyro_Y = incomingMessage->message[14] << 6;
					int16s gyro_Z = incomingMessage->message[15] << 6;
					if(gyro_X > 8191) gyro_X = (gyro_X - 16384)/4;
					if(gyro_Y > 8191) gyro_Y = (gyro_Y - 16384)/4;
					if(gyro_Z > 8191) gyro_Z = (gyro_Z - 16384)/4;

					emberAfAppPrintln(" Cmd %x NodeType %x SensorType %x TranjectionID %x Time %d ErrorCount %d nwkDownCount %d GyroX %d GyroY %d GyroZ %d"
						, incomingMessage->message[2], incomingMessage->message[3], incomingMessage->message[4]
						, getInt16FromMessage(incomingMessage->message,5), getInt32FromMessage(incomingMessage->message,7)
						, incomingMessage->message[11], incomingMessage->message[12], gyro_X, gyro_Y, gyro_Z);

					publishMqttGyroData(nodeEui64, incomingMessage->message, incomingMessage->nodeId, gyro_X, gyro_Y, gyro_Z);

				}
				else if(sensorType == 4 && nodeId != 0x0000)
				{
					uint16_t readNoiseValue = ((incomingMessage->message[13] << 8) |  incomingMessage->message[14])>>4;
					int32_t temp = HIGH_LOW_TO_INT(incomingMessage->message[15],incomingMessage->message[16]);
					int32_t temperatureMilliC = 0;
					temp = 5624 * temp;
					temp = temp >> 13;
					temp = temp - 11994;
					temp = (temp * 1000) >> 8;
					temperatureMilliC = temp;
					emberAfAppPrintln(" Cmd %x NodeType %x SensorType %x TranjectionID %x Time %d ErrorCount %d nwkDownCount %d Noise %d Temp %d"
					, incomingMessage->message[2], incomingMessage->message[3], incomingMessage->message[4]
					, getInt16FromMessage(incomingMessage->message,5), getInt32FromMessage(incomingMessage->message,7)
					, incomingMessage->message[11], incomingMessage->message[12], readNoiseValue, temperatureMilliC);
					publishMqttSoundData(nodeEui64, incomingMessage->message, incomingMessage->nodeId, readNoiseValue,temperatureMilliC);
				}
				else if(sensorType == 0 && nodeId != 0x0000)
				{
					emberAfAppPrintln("- No sensor Cmd %x NT %x ST %x Trans %x Time %d EC %d nwkDC %d LQI %d RSSI %d"
					, incomingMessage->message[2], incomingMessage->message[3], incomingMessage->message[4]
					, getInt16FromMessage(incomingMessage->message,5), getInt32FromMessage(incomingMessage->message,7)
					, incomingMessage->message[11], incomingMessage->message[12], lqi, rssi);
					publishMqttBasicData(nodeEui64, incomingMessage->message, nodeId);
				}
				else if(sensorType == 5 && nodeId != 0x0000) // Illu Test
				{
					uint16_t Illu = HIGH_LOW_TO_INT(incomingMessage->message[15],incomingMessage->message[17]);
					Illu /= 1.2;
					emberAfAppPrintln(" FC %x Seq %x Cmd %x NodeType %x SensorType %x TranjectionID %x Time %d ErrorCount %d Illu %d", incomingMessage->message[0], incomingMessage->message[1], incomingMessage->message[2], incomingMessage->message[3], incomingMessage->message[4], getInt32FromMessage(incomingMessage->message,5), getInt32FromMessage(incomingMessage->message,9), getInt32FromMessage(incomingMessage->message,13), Illu);
					publishMqttTempData(nodeEui64, incomingMessage->message, incomingMessage->nodeId, Illu);
				
				}
				if(msg_len == 12)
				{
					uint8_t mode = incomingMessage->message[3];
					uint32_t transactionId = getInt32FromMessage(incomingMessage->message, 4);
					uint32_t data = getInt32FromMessage(incomingMessage->message, 8);
					emberAfAppPrintln(" [node(0x%2x) - mode(0x%x) - transactionId(%d) - data(%d)] ***", incomingMessage->nodeId, mode, transactionId, data);

					publishMqttDataTransaction(nodeEui64, incomingMessage->nodeId, mode, transactionId, data);
				}
				//sendDataReceivedConfirmation(incomingMessage->nodeId);
				return true;
			}


		}
	}*/
	return false;
}

bool emberAfPreMessageSendCallback(EmberAfMessageStruct* messageStruct,
                                   EmberStatus* status)
{
  if (trafficReporting) {
    publishMqttTrafficReportEvent("preMessageSend",
                                  NULL, // status unsused
                                  NULL, // rssi unused
                                  NULL, // lqi unused
                                  NULL,
                                  halCommonGetInt32uMillisecondTick());
  }
  return false;
}

bool emberAfReadAttributesResponseCallback(EmberAfClusterId clusterId,
                                              uint8_t* buffer,
                                              uint16_t bufLen)
{
  EmberEUI64 nodeEui64;
  EmberNodeId nodeId = emberAfCurrentCommand()->source;
  emberAfDeviceTableGetEui64FromNodeId(nodeId, nodeEui64);

  // If a zero-length attribute is reported, just leave
  if (bufLen == 0) {
    emberAfAppPrintln("Read attributes callback: zero length buffer");
    return false;
  }

  emberAfAppPrintln("Read attributes: 0x%2x", clusterId);
  publishMqttAttribute(nodeEui64,
                       clusterId,
                       buffer,
                       bufLen);
  printAttributeBuffer(clusterId, buffer, bufLen);

  return false;
}

bool emberAfReportAttributesCallback(EmberAfClusterId clusterId,
                                     uint8_t * buffer,
                                     uint16_t bufLen)
{
  EmberEUI64 nodeEui64;
  EmberNodeId nodeId = emberAfCurrentCommand()->source;
  emberAfDeviceTableGetEui64FromNodeId(nodeId, nodeEui64);
  uint8_t* bufferPtr = buffer;
  uint8_t i;

  if (bufLen == 0) {
    emberAfAppPrintln("Report attributes callback: zero length buffer");
    return false;
  }

  // Buffer [0] is low bits, Buffer [1] is high bits, Buffer [2] is dataType,
  // Buffer [3+] is data
  emberAfAppPrintln("Reporting attributes for cluster: 0x%2x", clusterId);
  for (i = 0; i < bufLen;) {
    // Get Length of Attribute Buffer
    uint8_t bufferSizeI = emberAfGetDataSize(bufferPtr[ATTRIBUTE_BUFFER_REPORT_DATA_TYPE]);
    // Add 3 bytes for header size
    bufferSizeI += 3;

    //Copy buffer to attributeBufferI
    uint8_t* bufferTemp = (uint8_t*)malloc(bufferSizeI);
    memcpy(bufferTemp, bufferPtr, bufferSizeI);

    // Set i to point to:
    // [attrLSB,attrMSB,dataT,buffer,nAttrLSB,nAttrMSB,nextDataT,nextBuffer]
    // [       ,       ,     , ..n  ,   X    ,        ,         ,  ..n     ]
    bufferPtr = bufferPtr + bufferSizeI;
    i = i + bufferSizeI;

    emberAfAppPrintln("Reported attribute: 0x%02X%02X, Type: %02X",
                      bufferTemp[ATTRIBUTE_BUFFER_CLUSTERID_HIGH_BITS],
                      bufferTemp[ATTRIBUTE_BUFFER_CLUSTERID_LOW_BITS],
                      bufferTemp[ATTRIBUTE_BUFFER_REPORT_DATA_TYPE]);

    publishMqttAttributeReport(nodeEui64,
                               clusterId,
                               bufferTemp,
                               bufferSizeI);
    free(bufferTemp);
  }

  return false;
}

// Device Table Callbacks
void emberAfPluginDeviceTableNewDeviceCallback(EmberEUI64 eui64)
{
  publishMqttDeviceJoined(eui64);
}

void emberAfPluginDeviceTableDeviceLeftCallback(EmberEUI64 nodeEui64)
{
  publishMqttDeviceLeft(nodeEui64);
}

void emberAfPluginDeviceTableRejoinDeviceCallback(EmberEUI64 nodeEui64)
{
  publishMqttDeviceJoined(nodeEui64);
}

void emberAfPluginDeviceTableStateChangeCallback(EmberNodeId nodeId,
                                                 uint8_t state)
{
  EmberEUI64 nodeEui64;
  emberAfDeviceTableGetEui64FromNodeId(nodeId, nodeEui64);
  publishMqttDeviceStateChange(nodeEui64, state);
}

void emberAfPluginDeviceTableClearedCallback(void)
{
  publishMqttGatewayState();
}

void emberAfPluginCommandRelayChangedCallback(void)
{
  publishMqttRelays();
}

// MQTT Transport Callbacks
void emberAfPluginTransportMqttStateChangedCallback(EmberAfPluginTransportMqttState state)
{
  switch (state) {
  case EMBER_AF_PLUGIN_TRANSPORT_MQTT_STATE_CONNECTED:
    {
      EmberAfPluginLinkedListElement* currentElement = NULL;
      MqttTopicHandlerMap* topicHandlerMap = NULL;

      emberAfAppPrintln(
        "MQTT connected, starting gateway heartbeat and command processing");
      emberEventControlSetActive(heartbeatEventControl);
      emberEventControlSetActive(processCommandEventControl);

      // Loop through the Topic Handler Map to subscribe to all the topics
      do {
        currentElement = emberAfPluginLinkedListNextElement(topicHandlerList,
                                                            currentElement);
        if (currentElement != NULL) {
          topicHandlerMap = (MqttTopicHandlerMap*)currentElement->content;
          emberAfPluginTransportMqttSubscribe(topicHandlerMap->topic);
        }
      } while (currentElement != NULL);

      // Since we are newly connecting, dump our complete device state and relay
      // list
      publishMqttGatewayState();
      break;
    }
  case EMBER_AF_PLUGIN_TRANSPORT_MQTT_STATE_DISCONNECTED:
    emberAfAppPrintln("MQTT disconnected, stopping gateway heartbeat");
    emberEventControlSetInactive(heartbeatEventControl);
    emberEventControlSetInactive(processCommandEventControl);
    break;
  default:
    // Unknown state
    emberAfAppPrintln("Unknown MQTT state");
    assert(false);
    break;
  }
}

bool emberAfPluginTransportMqttMessageArrivedCallback(const char* topic,
                                                      const char* payload)
{
  cJSON* incomingMessageJson;
  EmberAfPluginLinkedListElement* currentElement = NULL;
  MqttTopicHandlerMap* topicHandlerMap = NULL;

  incomingMessageJson = cJSON_Parse(payload);

  // Loop through the Topic Handler Map to determine which handler to call
  do {
    currentElement = emberAfPluginLinkedListNextElement(topicHandlerList,
                                                        currentElement);
    if (currentElement != NULL) {
      topicHandlerMap = (MqttTopicHandlerMap*)currentElement->content;

      // If the incoming topic matches a topic in the map, call it's handler
      if (strcmp(topic, topicHandlerMap->topic) == 0) {
        topicHandlerMap->topicHandler(incomingMessageJson);
        break;
      }
    }
  } while (currentElement != NULL);

  cJSON_Delete(incomingMessageJson);

  // Return true, this tells the MQTT client we have handled the incoming
  // message
  return true;
}

static void handleCommandsMessage(cJSON* messageJson)
{
  uint8_t commandIndex;
  cJSON* commandsJson;
  cJSON* commandJson;
  cJSON* commandStringJson;
  cJSON* postDelayMsJson;

  if (messageJson != NULL) {
  	char *messageString = cJSON_PrintUnformatted(messageJson);
    emberAfAppPrintln("Handling Commands Message: %s",
                      messageString);
    free(messageString);
    commandsJson = cJSON_GetObjectItem(messageJson, "commands");
    if (commandsJson != NULL) {
      for (commandIndex = 0;
           commandIndex < cJSON_GetArraySize(commandsJson);
           commandIndex++)
      {
        commandJson = cJSON_GetArrayItem(commandsJson, commandIndex);
        if (commandJson != NULL) {
          commandStringJson = cJSON_GetObjectItem(commandJson, "command");
          if (commandStringJson != NULL) {
            addCliCommandToList(commandList, commandStringJson->valuestring);
          }

          postDelayMsJson = cJSON_GetObjectItem(commandJson, "postDelayMs");
          if (postDelayMsJson != NULL) {
            addPostDelayMsToList(commandList,
                                 (uint32_t)postDelayMsJson->valueint);
          }
        }
      }
    }
  }
}

static void addCliCommandToList(EmberAfPluginLinkedList* list,
                                const char* cliCommandString)
{
  GatewayCommand* gatewayCommand = allocateGatewayCommand();
  char* cliCommandStringForList =
    (char*)malloc(strlen(cliCommandString) + 1); // Add NULL char
  strcpy(cliCommandStringForList, cliCommandString); // Copies string including NULL char

  gatewayCommand->commandType = COMMAND_TYPE_CLI;
  gatewayCommand->cliCommand = cliCommandStringForList;

  emberAfPluginLinkedListPushBack(list, (void*)gatewayCommand);
}

static void addPostDelayMsToList(EmberAfPluginLinkedList* commandList,
                                 uint32_t postDelayMs)
{
  GatewayCommand* gatewayCommand = allocateGatewayCommand();

  gatewayCommand->commandType = COMMAND_TYPE_POST_DELAY;
  gatewayCommand->postDelayMs = postDelayMs;

  emberAfPluginLinkedListPushBack(commandList, (void*)gatewayCommand);
}

void processCommandEventFunction(void)
{
  emberEventControlSetDelayMS(processCommandEventControl,
                              PROCESS_COMMAND_RATE_MS);
  EmberAfPluginLinkedListElement* commandListItem = NULL;
  GatewayCommand* gatewayCommand;

  assert(commandList != NULL);

  // Get the head of the command list
  commandListItem = emberAfPluginLinkedListNextElement(commandList,
                                                       NULL);

  // If there is nothing there, continue on
  if (commandListItem == NULL) {
    return;
  }

  gatewayCommand = commandListItem->content;
  assert(gatewayCommand != NULL);

  // CLI command processing
  if (gatewayCommand->commandType == COMMAND_TYPE_CLI) {
    // Process our command string, then pop the command from the list
    // First send the CLI, then send a /n to simulate the "return" key
    emberProcessCommandString((uint8_t*)gatewayCommand->cliCommand,
                              strlen(gatewayCommand->cliCommand));
    emberProcessCommandString((uint8_t*)"\n",
                              strlen("\n"));
    publishMqttCommandExecuted(gatewayCommand->cliCommand);
    emberAfAppPrintln("CLI command executed: %s",
                      gatewayCommand->cliCommand);
    freeGatewayCommand(gatewayCommand);
    emberAfPluginLinkedListPopFront(commandList);
  }
  // Delay processing
  else if (gatewayCommand->commandType == COMMAND_TYPE_POST_DELAY) {
    // If our resume time hasn't been initialized we are starting the delay
    if (gatewayCommand->resumeTime == 0) {
      // Make sure delay isn't 0, if so pop the list and move on
      if (gatewayCommand->postDelayMs == 0) {
        freeGatewayCommand(gatewayCommand);
        emberAfPluginLinkedListPopFront(commandList);
      }
      // Calculate the time to resume
      gatewayCommand->resumeTime = halCommonGetInt32uMillisecondTick() +
                                   gatewayCommand->postDelayMs;
    } else {
      // If we are already delaying, see if it's time to resume
      if (halCommonGetInt32uMillisecondTick() > gatewayCommand->resumeTime) {
        // Resume by popping this delay from the list
        publishMqttDelayExecuted(gatewayCommand->postDelayMs);
        emberAfAppPrintln("Delay executed for: %d ms",
                          gatewayCommand->postDelayMs);
        freeGatewayCommand(gatewayCommand);
        emberAfPluginLinkedListPopFront(commandList);
      }
    }
  }
}

static GatewayCommand* allocateGatewayCommand()
{
  GatewayCommand* gatewayCommand = (GatewayCommand*)malloc(sizeof(GatewayCommand));
  gatewayCommand->commandType = 0;
  gatewayCommand->cliCommand = NULL;
  gatewayCommand->resumeTime = 0;
  gatewayCommand->postDelayMs = 0;
  return gatewayCommand;
}

static void freeGatewayCommand(GatewayCommand* gatewayCommand)
{
  if (gatewayCommand != NULL) {
    if (gatewayCommand->cliCommand != NULL) {
      free(gatewayCommand->cliCommand);
    }
    free(gatewayCommand);
  }
}

static void handlePublishStateMessage(cJSON* messageJson)
{
  emberAfAppPrintln("Handling Publish State Message");
  publishMqttGatewayState();
}

static void handleUpdateSettingsMessage(cJSON* messageJson)
{
  if (messageJson != NULL) {
  	char *messageString = cJSON_PrintUnformatted(messageJson);
    emberAfAppPrintln("Handling Update Settings Message: %s",
                      messageString);
    free(messageString);
  }
}

// String/other helpers
static void eui64ToString(EmberEUI64 eui, char* euiString)
{
  sprintf(euiString, "0x%02X%02X%02X%02X%02X%02X%02X%02X",
          eui[7],
          eui[6],
          eui[5],
          eui[4],
          eui[3],
          eui[2],
          eui[1],
          eui[0]);
}

static void nodeIdToString(EmberNodeId nodeId, char* nodeIdString)
{
  sprintf(nodeIdString, "0x%04X", nodeId);
}

static void printAttributeBuffer(uint16_t clusterId,
                                 uint8_t* buffer,
                                 uint16_t bufLen)
{
  uint16_t bufferIndex;

  emberAfAppPrintln(" Cluster, Attribute: %04X, %02X%02X"
                    " Success Code: %02X"
                    " Data Type: %02X\n"
                    " Hex Buffer: ",
                    clusterId,
                    buffer[ATTRIBUTE_BUFFER_CLUSTERID_HIGH_BITS],
                    buffer[ATTRIBUTE_BUFFER_CLUSTERID_LOW_BITS],
                    buffer[ATTRIBUTE_BUFFER_SUCCESS_CODE],
                    buffer[ATTRIBUTE_BUFFER_DATA_TYPE]);

  // Print buffer data as a hex string, starting at the data start byte
  for (bufferIndex = ATTRIBUTE_BUFFER_DATA_START;
       bufferIndex < bufLen;
       bufferIndex++) {
    emberAfAppPrint("%02X", buffer[bufferIndex]);
  }
  emberAfAppPrintln("");
}

/** @brief Configure Reporting Response
 *
 * This function is called by the application framework when a Configure
 * Reporting Response command is received from an external device.  The
 * application should return true if the message was processed or false if it
 * was not.
 *
 * @param clusterId The cluster identifier of this response.  Ver.: always
 * @param buffer Buffer containing the list of attribute status records.  Ver.:
 * always
 * @param bufLen The length in bytes of the list.  Ver.: always
 */
boolean emberAfConfigureReportingResponseCallback(EmberAfClusterId clusterId,
                                                  uint8_t *buffer,
                                                  uint16_t bufLen)
{
  char* topic = allocateAndFormMqttGatewayTopic(ZDO_RESPONSE_TOPIC);
  cJSON* configureReportResponseJson;
  cJSON* deviceEndpointJson;
  char* configureReportResponseString;
  char* dataString;
  EmberEUI64 eui64;
  EmberNodeId nodeId;
  uint8_t sourceEndpoint;

  configureReportResponseJson = cJSON_CreateObject();

  cJSON_AddStringToObject(configureReportResponseJson,
                          "zdoType",
                          "configureReportResponse");

  nodeId = emberAfCurrentCommand()->source;
  emberAfDeviceTableGetEui64FromNodeId(nodeId, eui64);
  sourceEndpoint = emberAfCurrentCommand()->apsFrame->sourceEndpoint;
  deviceEndpointJson = buildDeviceEndpoint(eui64, sourceEndpoint);
  cJSON_AddItemToObject(configureReportResponseJson,
                        "deviceEndpoint",
                        deviceEndpointJson);

  dataString = createOneByteHexString(buffer[0]);
  cJSON_AddStringToObject(configureReportResponseJson,
                          "status",
                          dataString);
  free(dataString);

  configureReportResponseString =
  cJSON_PrintUnformatted(configureReportResponseJson);
  emberAfPluginTransportMqttPublish(topic, configureReportResponseString);
  free(configureReportResponseString);
  cJSON_Delete(configureReportResponseJson);
  free(topic);

  return false;
}

/** @brief Read Reporting Configuration Response
 *
 * This function is called by the application framework when a Read Reporting
 * Configuration Response command is received from an external device.  The
 * application should return true if the message was processed or false if it
 * was not.
 *
 * @param clusterId The cluster identifier of this response.  Ver.: always
 * @param buffer Buffer containing the list of attribute reporting configuration
 * records.  Ver.: always
 * @param bufLen The length in bytes of the list.  Ver.: always
 */
boolean emberAfReadReportingConfigurationResponseCallback(
          EmberAfClusterId clusterId,
          uint8_t *buffer,
          uint16_t bufLen)
{
  char* topic = allocateAndFormMqttGatewayTopic(ZCL_RESPONSE_TOPIC);
  cJSON* reportTableJson;
  cJSON* deviceEndpointJson;
  char* reportTableString;
  char* dataString = (char*)malloc(2*(bufLen) + HEX_TOKEN_SIZE); //"0x" + buffer
  uint8_t i;
  uint16_t maxInterval;
  uint16_t minInterval;
  char* tempString;
  EmberEUI64 eui64;
  EmberNodeId nodeId;
  uint8_t sourceEndpoint;

  for (i = 0; i < (2*bufLen); i++) {
    dataString[i] = 0;
  }

  reportTableJson = cJSON_CreateObject();

  cJSON_AddStringToObject(reportTableJson,
                          "zclType",
                          "reportTableEntry");

  nodeId = emberAfCurrentCommand()->source;
  emberAfDeviceTableGetEui64FromNodeId(nodeId, eui64);
  sourceEndpoint = emberAfCurrentCommand()->apsFrame->sourceEndpoint;
  deviceEndpointJson = buildDeviceEndpoint(eui64, sourceEndpoint);
  cJSON_AddItemToObject(reportTableJson, "deviceEndpoint", deviceEndpointJson);

  tempString = createOneByteHexString(buffer[READ_REPORT_CONFIG_STATUS]);
  cJSON_AddStringToObject(reportTableJson, "status", tempString);
  free(tempString);

  cJSON_AddIntegerToObject(reportTableJson,
                           "direction",
                           buffer[READ_REPORT_CONFIG_DIRECTION]);

  tempString = createTwoByteHexString(clusterId);
  cJSON_AddStringToObject(reportTableJson, "clusterId", tempString);
  free(tempString);

  tempString = createTwoByteHexString(
    HIGH_LOW_TO_INT(buffer[READ_REPORT_CONFIG_ATTRIBUTE_ID+1],
                    buffer[READ_REPORT_CONFIG_ATTRIBUTE_ID]));
  cJSON_AddStringToObject(reportTableJson, "attributeId", tempString);
  free(tempString);

  tempString = createOneByteHexString(buffer[READ_REPORT_CONFIG_DATA_TYPE]);
  cJSON_AddStringToObject(reportTableJson, "dataType", tempString);
  free(tempString);

  minInterval = HIGH_LOW_TO_INT(buffer[READ_REPORT_CONFIG_MIN_INTERVAL+1],
                                buffer[READ_REPORT_CONFIG_MIN_INTERVAL]);

  cJSON_AddIntegerToObject(reportTableJson, "minInterval", minInterval);

  maxInterval = HIGH_LOW_TO_INT(buffer[READ_REPORT_CONFIG_MAX_INTERVAL+1],
                                buffer[READ_REPORT_CONFIG_MAX_INTERVAL]);

  cJSON_AddIntegerToObject(reportTableJson, "maxInterval", maxInterval);

  if (bufLen > 0) {
    sprintf(&dataString[0], "0x");
  }

  for (i = READ_REPORT_CONFIG_DATA; i < bufLen; i++) {
    sprintf(&(dataString[2*(i-READ_REPORT_CONFIG_DATA) + HEX_TOKEN_SIZE]),
            "%02X", buffer[i]);
  }
  cJSON_AddStringToObject(reportTableJson, "data", dataString);

  reportTableString = cJSON_PrintUnformatted(reportTableJson);
  emberAfPluginTransportMqttPublish(topic, reportTableString);
  free(reportTableString);
  cJSON_Delete(reportTableJson);
  free(topic);
  free(dataString);

  return false;
}

bool emberAfPreCommandReceivedCallback(EmberAfClusterCommand* cmd)
{
  publishMqttZclCommand(cmd->commandId,
                        cmd->clusterSpecific,
                        cmd->apsFrame->clusterId,
                        cmd->mfgSpecific,
                        cmd->mfgCode,
                        cmd->buffer,
                        cmd->bufLen,
                        cmd->payloadStartIndex);
  return false;
};

/** @brief RulesPreCommandReceived
 *
 * Called when rules engine sees a pre command received callback
 *
 * @param commandId   Ver.: always
 * @param clusterSpecific   Ver.: always
 * @param clusterId   Ver.: always
 * @param mfgSpecific   Ver.: always
 * @param mfgCode   Ver.: always
 * @param buffer   Ver.: always
 * @param bufLen   Ver.: always
 * @param payloadStartIndex   Ver.: always
 */
static void publishMqttZclCommand(uint8_t commandId,
                                  boolean clusterSpecific,
                                  uint16_t clusterId,
                                  boolean mfgSpecific,
                                  uint16_t mfgCode,
                                  uint8_t* buffer,
                                  uint8_t bufLen,
                                  uint8_t payloadStartIndex)
{
  char* topic = allocateAndFormMqttGatewayTopic(ZCL_RESPONSE_TOPIC);
  cJSON* cmdResponseJson;
  cJSON* deviceEndpointJson;
  char* cmdResponseString;
  char* tempString;
  char* dataString = (char*)malloc(2*(bufLen) + HEX_TOKEN_SIZE); // "0x" + buffer
  uint8_t i, *bufPtr;
  bool squelchMessage = FALSE;
  EmberEUI64 eui64;
  EmberNodeId nodeId;
  uint8_t sourceEndpoint;

  for (i = 0; i < (2*bufLen); i++) {
    dataString[i] = 0;
  }

  cmdResponseJson = cJSON_CreateObject();

  tempString = createTwoByteHexString(clusterId);
  cJSON_AddStringToObject(cmdResponseJson, "clusterId", tempString);
  free(tempString);

  tempString = createOneByteHexString(commandId);
  cJSON_AddStringToObject(cmdResponseJson, "commandId", tempString);
  free(tempString);

  if (clusterId == ZCL_OTA_BOOTLOAD_CLUSTER_ID &&
      commandId == ZCL_WRITE_ATTRIBUTES_UNDIVIDED_COMMAND_ID) {
    squelchMessage = TRUE;
  }

  if ((bufLen - payloadStartIndex) > 0) {
    sprintf(&dataString[0], "0x");
  }

  bufPtr = buffer + payloadStartIndex;
  for (i = 0; i < (bufLen - payloadStartIndex); i++) {
    sprintf(& (dataString[2*i + HEX_TOKEN_SIZE]), "%02X", bufPtr[i]);
  }

  cJSON_AddStringToObject(cmdResponseJson, "commandData", dataString);
  free(dataString);

  if (clusterSpecific) {
    cJSON_AddTrueToObject(cmdResponseJson, "clusterSpecific");
  } else {
    cJSON_AddFalseToObject(cmdResponseJson, "clusterSpecific");
  }

  if (mfgSpecific) {
    tempString = createTwoByteHexString(mfgCode);
    cJSON_AddStringToObject(cmdResponseJson, "mfgCode", tempString);
    free(tempString);
  }

  deviceEndpointJson = cJSON_CreateObject();
  sourceEndpoint = emberAfCurrentCommand()->apsFrame->sourceEndpoint;
  nodeId = emberAfCurrentCommand()->source;
  emberAfDeviceTableGetEui64FromNodeId(nodeId, eui64);
  deviceEndpointJson = buildDeviceEndpoint(eui64, sourceEndpoint);
  cJSON_AddItemToObject(cmdResponseJson, "deviceEndpoint", deviceEndpointJson);

  cmdResponseString = cJSON_PrintUnformatted(cmdResponseJson);
  if (!squelchMessage) {
    emberAfPluginTransportMqttPublish(topic, cmdResponseString);
  }
  free(cmdResponseString);
  cJSON_Delete(cmdResponseJson);
  free(topic);
}

// code to print out the source route table
static void printSourceRouteTable(void) {
  uint8_t i;
  for (i = 0; i < sourceRouteTableSize; i++) {
    if (sourceRouteTable[i].destination != 0x0000) {
      emberAfCorePrintln("[ind]%x[dest]%2x[closer]%x[older]%x",
                         i,
                         sourceRouteTable[i].destination,
                         sourceRouteTable[i].closerIndex,
                         sourceRouteTable[i].olderIndex);
    }
    emberSerialWaitSend(APP_SERIAL);
  }
  emberAfCorePrintln("<print srt>");
  emberSerialWaitSend(APP_SERIAL);
}

// Called to dump all of the tokens. This dumps the indices, the names,
// and the values using ezspGetToken and ezspGetMfgToken. The indices
// are used for read and write functions below.
static void mfgappTokenDump(void)
{

  EmberStatus status;
  uint8_t tokenData[MFGSAMP_EZSP_TOKEN_MFG_MAXSIZE];
  uint8_t index, i, tokenLength;

  // first go through the tokens accessed using ezspGetToken
  emberAfCorePrintln("(data shown little endian)");
  emberAfCorePrintln("Tokens:");
  emberAfCorePrintln("idx  value:");
  for (index = 0; index < MFGSAMP_NUM_EZSP_TOKENS; index++) {

    // get the token data here
    status = ezspGetToken(index, tokenData);
    emberAfCorePrint("[%d]", index);
    if (status == EMBER_SUCCESS) {

      // Print out the token data
      for (i = 0; i < MFGSAMP_EZSP_TOKEN_SIZE; i++) {
        emberAfCorePrint( " %X", tokenData[i]);
      }

      emberSerialWaitSend(APP_SERIAL);
      emberAfCorePrintln("");
    }
    else {
    // handle when ezspGetToken returns an error
      emberAfCorePrintln(" ... error 0x%x ...",
                         status);
    }
  }

  // now go through the tokens accessed using ezspGetMfgToken
  // the manufacturing tokens are enumerated in app/util/ezsp/ezsp-protocol.h
  // this file contains an array (ezspMfgTokenNames) representing the names.
  emberAfCorePrintln("Manufacturing Tokens:");
  emberAfCorePrintln("idx  token name                 len   value");
  for (index = 0; index < MFGSAMP_NUM_EZSP_MFG_TOKENS; index++) {

    // ezspGetMfgToken returns a length, be careful to only access
    // valid token indices.
    tokenLength = ezspGetMfgToken(index, tokenData);
    emberAfCorePrintln("[%x] %p: 0x%x:", index,
                      ezspMfgTokenNames[index], tokenLength);

    // Print out the token data
    for (i = 0; i < tokenLength; i++) {
      if ((i != 0) && ((i % 8) == 0)) {
        emberAfCorePrintln("");
        emberAfCorePrint("                                    :");
        emberSerialWaitSend(APP_SERIAL);
      }
      emberAfCorePrint( " %X", tokenData[i]);
    }
    emberSerialWaitSend(APP_SERIAL);
    emberAfCorePrintln("");
  }
  emberAfCorePrintln("");
}

static void changeNwkKeyCommand(void)
{
  EmberStatus status = emberAfTrustCenterStartNetworkKeyUpdate();

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("Change Key Error %x", status);
  } else {
    emberAfCorePrintln("Change Key Success");
  }
}

static void dcPrintKey(uint8_t label, uint8_t *key)
{
  uint8_t i;
  emberAfCorePrintln("key %x: ", label);
  for (i = 0; i < EMBER_ENCRYPTION_KEY_SIZE; i++) {
    emberAfCorePrint( "%x", key[i]);
  }
  emberAfCorePrintln("");
}

static void printNextKeyCommand(void)
{
  EmberKeyStruct nextNwkKey;
  EmberStatus status;

  status = emberGetKey(EMBER_NEXT_NETWORK_KEY,
                       &nextNwkKey);

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("Error getting key");
  } else {
    dcPrintKey(1, nextNwkKey.key.contents);
  }
}

static void versionCommand(void)
{
  emberAfCorePrintln("Version:  0.1 Alpha");
  emberAfCorePrintln(" %s", __DATE__);
  emberAfCorePrintln(" %s", __TIME__);
  emberAfCorePrintln("");
}

static void setTxPowerCommand(void)
{
  int8_t dBm = (int8_t)emberSignedCommandArgument(0);

  emberSetRadioPower(dBm);
}

bool emberAfPreZDOMessageReceivedCallback(EmberNodeId emberNodeId,
                                          EmberApsFrame* apsFrame,
                                          uint8_t* message,
                                          uint16_t length)
{
  switch (apsFrame->clusterId)
  {
  case ACTIVE_ENDPOINTS_RESPONSE:
    break;
  case SIMPLE_DESCRIPTOR_RESPONSE:
    break;
  case END_DEVICE_ANNOUNCE:
    break;
  case PERMIT_JOINING_RESPONSE:
    break;
  case LEAVE_RESPONSE:
    break;
  case BIND_RESPONSE:
    publishMqttBindResponse(emberNodeId, apsFrame, message, length);
    break;
  case BINDING_TABLE_RESPONSE:
    publishMqttBindTableReponse(emberNodeId, apsFrame, message, length);
    break;
  case NETWORK_ADDRESS_RESPONSE:
    break;
  case IEEE_ADDRESS_RESPONSE:
    break;
  default:
    break;
  }

  return false;
}
