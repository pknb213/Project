/****************************************************************************************
 * 				2017. 12. 14    End-Device												*						*
 * 																						*
 * 																						*
 ***************************************************************************************/
//---------------------- Header File -----------------------------
#include "app/framework/include/af.h"
#include "app/framework/plugin-soc/idle-sleep/idle-sleep.h"
#include "10_25_v6_End_Device.h"
#include "protocol/zigbee_5.8/stack/include/ember-types.h"
#include "protocol/zigbee_5.8/app/framework/include/af-types.h"
#include "platform/base/hal/micro/micro-common.h"
#include "debug-printing.h"
#include "stack/include/event.h"
#include "hal/plugin/i2c-driver/i2c-driver.h"
#include "em_chip.h"
#include "em_adc.h"
#include "math.h"
//#include <stdbool.h>, stdint
//---------------------- Macro -----------------------------------
//#define USING_LMV324  Power is high . . . . ToT
#define USING_AdcConverter
//----------------------------------------------------------------
#define false 0
#define ture 1
#define COORDINATOR_NODE_ID			0x0000
#define SI7021_I2C_ADDR				( 0x40 << 1 )
#define SI_7021_READ_TEMPERATURE	0xE3
#define GY273_I2C_ADDR				( 0x1E<<1 )
#define GY273_READ_REGISTER			0x03
#define GY45_I2C_ADDR				( 0x1C << 1)
#define GY45_REG_STATUS				0x00
#define GY45_REG_CTRL_REG1			0x2A
#define GY45_REG_CTRL_REG2			0x2B
#define GY45_REG_XYZ_DATA_CFG		0x0E
//----------------------- Function -------------------------------
static void addInt32ToMessage(int8u* msg, int32u num, int8u start, int8u msg_len);
static void addInt16ToMessage(int8u* msg, int16u num, int8u start, int8u msg_len);
static void initApsFrame();
static void dataRequestUnicast();
static void sendSensorData();
static void sensorCheckHandler();
static void readSi7021(int8u *data);
static void readGy273(int8u *data);
static void readGy45(int8u *data);
static int8u i2cSi7021Read(int8u _register_addr, int8u* _data, int8u _nBytes);
static int8u i2cGy273Read(int8u _register_addr, int8u* _data, int8u _nBytes);
static int8u i2cGy45Read(int8u _register_addr, int8u* _data, int8u _nBytes);
//----------------------- Ifdef Define ---------------------------------
#ifdef USING_LMV324
static void initLmv324();
static int32u readLmv324();
static void disableLmv324();
#endif
#ifdef USING_AdcConverter
static void initADS1015Converter();
static void readADC_SingleEnded(int8u *data);
static int8u i2cReadRegister(int8u _device_addr, int8u _register_addr, int8u* _data, int8u _nBytes);
#define ADS1015_REG_CONFIG_PGA_6_144V   (0x0000)  // +/-6.144V range = Gain 2/3
#define ADS1015_REG_CONFIG_PGA_4_096V   (0x0200)  // +/-4.096V range = Gain 1
#define ADS1015_REG_CONFIG_PGA_2_048V   (0x0400)  // +/-2.048V range = Gain 2 (default)
#define ADS1015_REG_CONFIG_PGA_1_024V   (0x0600)  // +/-1.024V range = Gain 4
#define ADS1015_REG_CONFIG_PGA_0_512V   (0x0800)  // +/-0.512V range = Gain 8
#define ADS1015_REG_CONFIG_PGA_0_256V   (0x0A00)  // +/-0.256V range = Gain 16
#define ADS1015_REG_CONFIG_CQUE_NONE    (0x0003)  // Disable the comparator and put ALERT/RDY in high state (default)
#define ADS1015_REG_CONFIG_CLAT_NONLAT  (0x0000)  // Non-latching comparator (default)
#define ADS1015_REG_CONFIG_CPOL_ACTVLOW (0x0000)  // ALERT/RDY pin is low when active (default)
#define ADS1015_REG_CONFIG_CMODE_TRAD   (0x0000)  // Traditional comparator with hysteresis (default)
#define ADS1015_REG_CONFIG_DR_1600SPS   (0x0080)  // 1600 samples per second (default)
#define ADS1015_REG_CONFIG_MODE_SINGLE  (0x0100)  // Power-down single-shot mode (default)
#define ADS1015_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
#define ADS1015_REG_CONFIG_OS_SINGLE    (0x8000)  // Write: Set to start a single-conversion
#define ADS1015_REG_POINTER_CONVERT     (0x00)
#define ADS1015_REG_POINTER_CONFIG      (0x01)
#define ADS1015_ADDR	(0x48 << 1)
#define ADS1015_REG_CONFIG_PGA_6_144V   (0x0000)  // +/-6.144V range = Gain 2/3
typedef enum
{
  GAIN_TWOTHIRDS    = 0x0000,
  GAIN_ONE          = 0x0200,
  GAIN_TWO          = 0x0400,
  GAIN_FOUR         = 0x0600,
  GAIN_EIGHT        = 0x0800,
  GAIN_SIXTEEN      = 0x0A00
} adsGain_t;
adsGain_t m_gain;
#endif
//********************** Data Value ******************************
#define NETWORK_PAN_ID 			0x1012
#define NETWORK_CHENNEL 		12
#define NETWORK_POWER 			3
#define REJOIN_DEFALUT			16
#define SLEEP_DEFALUT 			2400
#define ACK_DEFALUT				120
static EmberApsFrame unicastAf;
static int32u rejoinDuration = 16;
static int32u sleepDurtaion = 2400;
static int32u ackDuration = 120;
static int16u participationFailCount = 0;
static boolean AckFlagFA = 0;
static int32u timer,timer2,timer3 = 0;
static int32u transectionId = 0;
static int8u sendDataFailCounter = 0;
static int8u networkDownCounter = 0;
//static int32u ackTime = 0;
const static int8u activeMask = 0x01;
const static int8u range2G = 0x0;
//****************************************************************
typedef enum
{
	nwkCheckMode,
	sleepMode,
	schedulingMode,
	sendingMode,
}OperationMode;
OperationMode operationMode;
typedef enum
{
	dataNone,
	dataRequest,
	dataAck,
	dataSending,
	dataError,
}DataSchedulingMode;
DataSchedulingMode dataSchedulingMode;
typedef enum
{
	none,
	si7021,
	gy273,
	gy45,
	lmv324,
}SensorType;
SensorType sensorType;
//---------------------- Event ----------------------------------
EmberEventControl operationEvent;
EmberEventControl nwkState_ConfirmationEvent;
EmberEventControl sleep_ControlerEvent;
EmberEventControl data_SchedulingEvent;
EmberEventControl data_TransmissionEvent;
//----------------------------------------------------------------


void operationEventHandler()
{
	emberEventControlSetInactive(operationEvent);
	//emberAfAppPrintln("Loop event [time : %d]", emberAfGetCurrentTime());
	emberAfAppPrintln("Loop event [time : %d][Ack: %d][Data mode: %d]", emberAfGetCurrentTime(), AckFlagFA, dataSchedulingMode);
	int8u nwkStatus = emberAfNetworkState();

	switch(operationMode)
	{
		case nwkCheckMode:
		{
			emberAfAppPrintln("<Operation : NWK Check Mode>");
			emberEventControlSetActive(nwkState_ConfirmationEvent);
			break;
		}
		case sleepMode:
		{
			emberAfAppPrintln("<Operation : Sleep Mode>");
			emberEventControlSetActive(sleep_ControlerEvent);
			break;
		}
		case schedulingMode:
		{
			emberAfAppPrintln("<Operation : Scheduling Mode>");
			emberEventControlSetActive(data_SchedulingEvent);
			break;
		}
		case sendingMode:
		{
			emberAfAppPrintln("<Operation : Sending Mode>");
			emberEventControlSetActive(data_TransmissionEvent);
			break;
		}
	}
}
static void nwkState_Confirmation()
{
	emberEventControlSetInactive(nwkState_ConfirmationEvent);
	int8u nwkState = emberAfNetworkState();
	switch(nwkState)
	{
		case EMBER_NO_NETWORK :
		{
			emberSerialGuaranteedPrintf(APP_SERIAL, "Network Connection is started\n");
			EmberNetworkParameters networkParams;
			int8u extendedPanId[EXTENDED_PAN_ID_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0 };
			int8u networkIndex = emberGetCurrentNetwork();
			/* InIt NWK Parameters */
			MEMSET(&networkParams, 0, sizeof(EmberNetworkParameters));
			MEMCOPY(networkParams.extendedPanId, extendedPanId, EXTENDED_PAN_ID_SIZE);
			networkParams.joinMethod = EMBER_USE_MAC_ASSOCIATION;
			/* Artik 5 Coordinator */
			networkParams.panId = NETWORK_PAN_ID;
			networkParams.radioChannel = NETWORK_CHENNEL;
			networkParams.radioTxPower = NETWORK_POWER;
			/* Static Join Function */
			emberAfJoinNetwork(&networkParams);
			/* Dynamic Join Function */
			//EmberStatus status = emberAfStartSearchForJoinableNetwork();
			/* Next Operation Mode */
			operationMode = sleepMode;
			participationFailCount++;
			emberEventControlSetDelayMS(operationEvent,3000);
			break;
		}
		case EMBER_JOINING_NETWORK :
		{
			// Empty
			operationMode = nwkCheckMode;
			emberEventControlSetDelayMS(operationEvent,1500);
			break;
		}
		case EMBER_JOINED_NETWORK :
		{
			operationMode = schedulingMode;
			if(dataSchedulingMode == dataNone)
			{
				dataSchedulingMode = dataRequest;
			}
			emberEventControlSetActive(operationEvent);
			break;
		}
		case EMBER_JOINED_NETWORK_NO_PARENT :
		{
			emberLeaveNetwork();
			emberSerialGuaranteedPrintf(APP_SERIAL, "Network No Parent Leave\n");
			operationMode = nwkCheckMode;
			emberEventControlSetActive(operationEvent);
			break;
		}
		case EMBER_LEAVING_NETWORK :
		{
			// Empty
			operationMode = nwkCheckMode;
			emberEventControlSetDelayMS(operationEvent,1500);
			break;
		}
	}
}
static void sleep_Controler()
{
	emberEventControlSetInactive(sleep_ControlerEvent);
	switch(dataSchedulingMode)
	{
		case dataSending:
		{
			emberSerialGuaranteedPrintf(APP_SERIAL, "[SendingMode]Enter the Low Power [Duration: %d s]", sleepDurtaion/4);
			halInternalDisableWatchDog(MICRO_DISABLE_WATCH_DOG_KEY);
			/* Thread Delay */
			//halCommonDelayMilliseconds(500);
			/* Turn off the radio */
			emberStackPowerDown();
			/* Turn off board and peripherals */
			halPowerDown();
			/* Turn micro to power save mode : wake up is only 2way (external pin, timer) */
			halSleepForQuarterSeconds(&sleepDurtaion);
			halPowerUp();
			emberStackPowerUp();
			halInternalEnableWatchDog();

			sleepDurtaion = SLEEP_DEFALUT;
			operationMode = sendingMode;
			emberEventControlSetDelayMS(operationEvent,500);
			break;
		}
		case dataAck: // Not Apply
		{
			emberSerialGuaranteedPrintf(APP_SERIAL, "[AckMode]Enter the Low Power [Duration: %d s]", ackDuration/4);
			halInternalDisableWatchDog(MICRO_DISABLE_WATCH_DOG_KEY);
			/* Thread Delay */
			//halCommonDelayMilliseconds(500);
			/* Turn off the radio */
			emberStackPowerDown();
			/* Turn off board and peripherals */
			halPowerDown();
			/* Turn micro to power save mode : wake up is only 2way (external pin, timer) */
			halSleepForQuarterSeconds(&ackDuration);
			halPowerUp();
			emberStackPowerUp();
			halInternalEnableWatchDog();

			ackDuration = ACK_DEFALUT;	// Maybe go to Network Down
			operationMode = nwkCheckMode;
			emberEventControlSetDelayMS(operationEvent,500);
			break;
		}
		default:
		{
			emberSerialGuaranteedPrintf(APP_SERIAL, "[RejoinMode]Enter the Low Power [Duration: %d s]", rejoinDuration/4);
			halInternalDisableWatchDog(MICRO_DISABLE_WATCH_DOG_KEY);
			/* Thread Delay  */
			//halCommonDelayMilliseconds(500);
			/* Turn off the radio */
			emberStackPowerDown();
			/* Turn off board and peripherals */
			halPowerDown();
			/* Turn micro to power save mode : wake up is only 2way (external pin, timer) */
			halSleepForQuarterSeconds(&rejoinDuration);
			halPowerUp();
			emberStackPowerUp();
			halInternalEnableWatchDog();

			/* Calculration of Sleep Duration */
			rejoinDuration = REJOIN_DEFALUT;
			if(participationFailCount != 0)
			{
				rejoinDuration = (int32u)pow(2, participationFailCount+4);
			}
			else
			{
				rejoinDuration = REJOIN_DEFALUT;
			}
			/* Max Sleep Duration */
			if(rejoinDuration >= 14400)
				rejoinDuration = 14400;
			/* Next Operation Mode */
			operationMode = nwkCheckMode;
			emberEventControlSetDelayMS(operationEvent, 250);
			break;
		}
	}
}
static void data_Scheduling()
{
	emberEventControlSetInactive(data_SchedulingEvent);
	emberAfAppPrintln("Scheduling for data transmission");
	switch(dataSchedulingMode)
	{
		case dataNone:
		{
			emberLeaveNetwork();
			emberAfAppPrintln("Network Leave Because of No Parent ");
			operationMode = nwkCheckMode;
		//	dataSchedulingMode = dataAck;
			emberEventControlSetActive(operationEvent);
			break;
		}
		case dataRequest:
		{
			timer = halCommonGetInt32uMillisecondTick();
			dataRequestUnicast();
			dataSchedulingMode = dataAck;
			emberEventControlSetDelayMS(operationEvent, 1000);
			break;
		}
		case dataAck:
		{
			timer2 = halCommonGetInt32uMillisecondTick();
			//ackTime = timer2/1000;
			//emberAfAppPrintln("<Timer2 : %d", timer2);
			timer3 = timer2 - timer;
			//emberAfAppPrintln("<timer3 : %d> < AckFlag : %d>", timer3, AckFlagFA);
			if(AckFlagFA == 1)
			{
				dataSchedulingMode = dataSending;
			}
			else if(timer3 >= 1500)
			{
				dataSchedulingMode = dataNone;
				emberAfAppPrintln("Not Ack!!");
			}
			emberEventControlSetDelayMS(operationEvent,290);
			break;
		}
		case dataSending:
		{
			operationMode = sendingMode;
			emberEventControlSetActive(operationEvent);
			break;
		}
		case dataError:
		{
			// Empty
			break;
		}
	}
	//operationMode = nwkCheckMode;
	//emberEventControlSetDelayMS(operationEvent, 5000);
}
static void data_Transmission()
{
	emberEventControlSetInactive(data_TransmissionEvent);
	//if(ackTime <= 50000)
	//{
		emberAfAppPrintln("Ready to Data Transmission");
		sendSensorData();
		operationMode = sleepMode;
	//}
	/*else
	{
		emberAfAppPrintln("Ack reset");
		AckFlagFA = 0;
		operationMode = schedulingMode;
		dataSchedulingMode = dataRequest;
	}*/
	emberEventControlSetDelayMS(operationEvent, 1500);
}
EmberEventData appEvents[] =
{
		{ &operationEvent, 				operationEventHandler },
		{ &nwkState_ConfirmationEvent,	nwkState_Confirmation },
		{ &sleep_ControlerEvent,		sleep_Controler },
		{ &data_SchedulingEvent,		data_Scheduling },
		{ &data_TransmissionEvent, 		data_Transmission },
		{ NULL,	NULL }
};
//----------------------------------------------------------------
static void addInt32ToMessage(int8u* msg, int32u num, int8u start, int8u msg_len)
{
   if(msg_len >= start+3)
   {
      for(int8u i = 0; i < 4; i++){
            msg[start+i] = ((int8u*)&num)[3-i];
      }
   }
}
static void addInt16ToMessage(int8u* msg, int16u num, int8u start, int8u msg_len)
{
   if(msg_len >= start+1)
   {
      for(int8u i = 0; i < 2; i++){
            msg[start+i] = ((int8u*)&num)[1-i];
      }
   }
}
static void initApsFrame()
{
	//Init aps frame for sending unicast
	unicastAf.clusterId = 0xFA00;
	unicastAf.destinationEndpoint = emberAfPrimaryEndpoint();
	unicastAf.groupId = 0x0000;
	unicastAf.options = EMBER_APS_OPTION_NONE;
	unicastAf.profileId = emberAfPrimaryProfileId();
	unicastAf.sequence = 0;
	unicastAf.sourceEndpoint = emberAfPrimaryEndpoint();
	emberAfAppPrintln("ApsFrame Initialization [Cluster: %2x]", unicastAf.clusterId);
}
static void dataRequestUnicast()
{
	int8u msg[3];
	int8u len = sizeof(msg)/sizeof(msg[0]);
	msg[2] = 0xFA;

	emberAfSendUnicast(EMBER_OUTGOING_DIRECT, COORDINATOR_NODE_ID, &unicastAf, len, msg);
}
static void sendSensorData()
{
	EmberApsFrame apsFrame;
	apsFrame.clusterId = 0xFF00;
	apsFrame.destinationEndpoint = emberAfPrimaryEndpoint();
	apsFrame.groupId = 0x0000;
	apsFrame.options = EMBER_APS_OPTION_NONE;
	apsFrame.profileId = emberAfPrimaryProfileId();
	apsFrame.sequence = 0;
	apsFrame.sourceEndpoint = emberAfPrimaryEndpoint();

	switch(sensorType)
	{
		case si7021 :
		{
			int8u msg[15];

			int8u packetSize = sizeof(msg)/sizeof(msg[0]);
			msg[0] = 0;
			msg[1] = 0;
			msg[2] = 0xDA;
			msg[3] = emAfCurrentZigbeeProNetwork->nodeType;
			msg[4] = sensorType;
			addInt16ToMessage(msg, transectionId, 5, packetSize);
			addInt32ToMessage(msg, emberAfGetCurrentTime(), 7, packetSize);
			msg[11] = sendDataFailCounter;
			msg[12] = networkDownCounter;

			int8u data[2];
			readSi7021(data);
			for(int i = 0; i <= 1; i++)
			{
				msg[i+13] = data[i];
			}

			emberAfSendUnicast(EMBER_OUTGOING_DIRECT, COORDINATOR_NODE_ID, &apsFrame, packetSize, msg);
			break;
		}
		case gy273 :
		{
			int8u msg[19];

			int8u packetSize = sizeof(msg)/sizeof(msg[0]);
			msg[0] = 0;
			msg[1] = 0;
			msg[2] = 0xDA;
			msg[3] = emAfCurrentZigbeeProNetwork->nodeType;
			msg[4] = sensorType;
			addInt16ToMessage(msg, transectionId, 5, packetSize);
			addInt32ToMessage(msg, emberAfGetCurrentTime(), 7, packetSize);
			msg[11] = sendDataFailCounter;
			msg[12] = networkDownCounter;

			int8u data[6];
			readGy273(data);
			for(int i = 0; i <= 5; i++)
			{
				msg[i+13] = data[i];
			}

			emberAfSendUnicast(EMBER_OUTGOING_DIRECT, COORDINATOR_NODE_ID, &apsFrame, packetSize, msg);
			break;
		}
		case gy45 :
		{
			int8u msg[17];

			int8u packetSize = sizeof(msg)/sizeof(msg[0]);
			msg[0] = 0;
			msg[1] = 0;
			msg[2] = 0xDA;
			msg[3] = emAfCurrentZigbeeProNetwork->nodeType;
			msg[4] = sensorType;
			addInt16ToMessage(msg, transectionId, 5, packetSize);
			addInt32ToMessage(msg, emberAfGetCurrentTime(), 7, packetSize);
			msg[11] = sendDataFailCounter;
			msg[12] = networkDownCounter;

			int8u data[3];
			readGy45(data);
			for(int i = 0; i <= 2; i++)
			{
				msg[i+13] = data[i];
			}

			emberAfSendUnicast(EMBER_OUTGOING_DIRECT, COORDINATOR_NODE_ID, &apsFrame, packetSize, msg);
			break;
		}
		case lmv324 :
		{
#ifdef USING_LMV324
			initLmv324();
			int8u msg[17];

			int8u packetSize = sizeof(msg)/sizeof(msg[0]);
			msg[0] = 0;
			msg[1] = 0;
			msg[2] = 0xDA;
			msg[3] = emAfCurrentZigbeeProNetwork->nodeType;
			msg[4] = sensorType;
			addInt16ToMessage(msg, transectionId, 5, packetSize);
			addInt32ToMessage(msg, emberAfGetCurrentTime(), 7, packetSize);
			msg[11] = sendDataFailCounter;
			msg[12] = networkDownCounter;

			int32u data = readLmv324();
			addInt32ToMessage(msg, data, 13, packetSize);

			emberAfSendUnicast(EMBER_OUTGOING_DIRECT, COORDINATOR_NODE_ID, &apsFrame, packetSize, msg);
			disableLmv324();
#endif
#ifdef USING_AdcConverter
			int8u msg[17];
			initADS1015Converter();

			int8u packetSize = sizeof(msg)/sizeof(msg[0]);
			msg[0] = 0;
			msg[1] = 0;
			msg[2] = 0xDA;
			msg[3] = emAfCurrentZigbeeProNetwork->nodeType;
			msg[4] = sensorType;
			addInt16ToMessage(msg, transectionId, 5, packetSize);
			addInt32ToMessage(msg, emberAfGetCurrentTime(), 7, packetSize);
			msg[11] = sendDataFailCounter;
			msg[12] = networkDownCounter;

			int8u noiseData[2];
			readADC_SingleEnded(noiseData);
			for(int i = 0; i <= 1; i++)
			{
				msg[i+13] = noiseData[i];
			}

			int8u data[2];
			readSi7021(data);
			for(int i = 0; i <= 1; i++)
			{
				msg[i+15] = data[i];
			}
#endif
			emberAfSendUnicast(EMBER_OUTGOING_DIRECT, COORDINATOR_NODE_ID, &apsFrame, packetSize, msg);
			break;

		}
		case none :
		{
			int8u msg[13];

			int8u packetSize = sizeof(msg)/sizeof(msg[0]);
			msg[0] = 0;
			msg[1] = 0;
			msg[2] = 0xDA;
			msg[3] = emAfCurrentZigbeeProNetwork->nodeType;
			msg[4] = sensorType;
			addInt16ToMessage(msg, transectionId, 5, packetSize);
			addInt32ToMessage(msg, emberAfGetCurrentTime(), 7, packetSize);
			msg[11] = sendDataFailCounter;
			msg[12] = networkDownCounter;

			emberAfSendUnicast(EMBER_OUTGOING_DIRECT, COORDINATOR_NODE_ID, &apsFrame, packetSize, msg);
			sensorCheckHandler();
			break;
		}
	}
}
static void commandHandler(EmberAfIncomingMessage *incomingMessage)
{
	emberAfAppPrintln("Command Handling");
	int8u cmd = incomingMessage->message[2];

	if((AckFlagFA == 0) && (cmd == 0xFB) && (incomingMessage->source == 0x00))
	{
		emberAfAppPrintln("Received ACK");
		AckFlagFA = 1; // True
	}
}
static void sensorCheckHandler()
{
	halInternalDisableWatchDog(MICRO_DISABLE_WATCH_DOG_KEY);
	emberAfAppPrintln("Checking the Sensor type");
/*
	// Start with default values
	int16u config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
					  ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
					  ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
					  ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
					  ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
					  ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default
	// Set PGA/voltage range
	config |= m_gain;
	config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
	// Set 'start single-conversion' bit
	config |= ADS1015_REG_CONFIG_OS_SINGLE;
	m_gain = GAIN_TWOTHIRDS;
	int8u basic_res[3];
	basic_res[0]= ADS1015_REG_POINTER_CONFIG;
	addInt16ToMessage(basic_res, config, 1, 3);
	int8u errorCode = halI2cWriteBytes(ADS1015_ADDR, basic_res, 3);
*/
	int8u basic_res = ADS1015_REG_POINTER_CONFIG;
	int8u errorCode = halI2cWriteBytes(ADS1015_ADDR, &basic_res, 3);
	if(errorCode == 0x00)
	{
		emberAfAppPrintln("Set Noise Sensor Register OK");
		int8u res = 0xFE;
		errorCode = halI2cWriteBytes(SI7021_I2C_ADDR, &res, 1);
		if(errorCode == 0x00)
		{
			emberAfAppPrintln("Also set Temperature Sensor Register OK");
		}
		sensorType = lmv324;
	}
	else
	{
		int8u config_data[2];
		config_data[0] = 0x02;
		config_data[1] = 0x00;
		errorCode = halI2cWriteBytes(GY273_I2C_ADDR, config_data, 2);

		if(errorCode == 0x00)
		{
			emberAfAppPrintln("Set Gyro273 Sensor Register OK");
			sensorType = gy273;
		}
		else
		{
			// Reset
			int8u buffer[2];
			buffer[0] = GY45_REG_CTRL_REG2;
			buffer[1] = 0x40;
			halI2cWriteBytes(GY45_I2C_ADDR, buffer, 2);
			// Standy
			buffer[0] = GY45_REG_CTRL_REG1;
			buffer[1] = ~(activeMask);
			halI2cWriteBytes(GY45_I2C_ADDR, buffer, 1);
			// 2G
			buffer[0] = GY45_REG_XYZ_DATA_CFG;
			buffer[1] = range2G;
			halI2cWriteBytes(GY45_I2C_ADDR, buffer, 2);
			/* High Res
			buffer[0] = GY45_REG_CTRL_REG2;
			buffer[1] = 0x02; */
			halI2cWriteBytes(GY45_I2C_ADDR, buffer, 2);
			// Active
			buffer[0] = GY45_REG_CTRL_REG1;
			buffer[1] = 0x01;
			errorCode =	halI2cWriteBytes(GY45_I2C_ADDR, buffer, 2);

			if(errorCode == 0x00)
			{
				emberAfAppPrintln("Set Gyro45 Sensor Register OK");
				sensorType = gy45;
			}
			else
			{
				int8u i2cRegister = 0xFE;
				errorCode = halI2cWriteBytes(SI7021_I2C_ADDR, &i2cRegister, 1);
				if(errorCode == 0x00)
				{
					emberAfAppPrintln("Set Temperature Sensor Register OK");
					sensorType = si7021;
				}
				else
				{
					emberAfAppPrintln("*********** All Sensor Read Fail, errorCode: 0x%x", errorCode);
					sensorType = none;
				}
			}
		}
	}
	halInternalEnableWatchDog();
}
static void readSi7021(int8u *data)
{
	int8u buffer[2];
	int8u error = i2cSi7021Read(SI7021_I2C_ADDR, buffer, 2);

	if(error == 0x00)
	{
		int32s temp = HIGH_LOW_TO_INT(buffer[0],buffer[1]);
		int32s temperatureMilliC = 0;
		temp = 5624 * temp;
		temp = temp >> 13;
		temp = temp - 11994;
		temp = (temp * 1000) >> 8;
		temperatureMilliC = temp;
		emberAfAppPrintln("- Temp : %d.%d", temperatureMilliC/1000, temperatureMilliC%1000);

		data[0] = buffer[0];
		data[1]	= buffer[1];
	}
	else
	{
	   emberAfAppPrintln("< Read SI7021 Error >");
	   sensorCheckHandler();
	   data = 0;
	}
}
static void readGy273(int8u *data)
{
   int8u buffer[6];
   int8u error = i2cGy273Read(GY273_READ_REGISTER, buffer, 6);

   if(error == 0x00)
   {
		int16s Gyro_x = HIGH_LOW_TO_INT(buffer[0],buffer[1]);
		int16s Gyro_y = HIGH_LOW_TO_INT(buffer[2],buffer[3]);
		int16s Gyro_z = HIGH_LOW_TO_INT(buffer[4],buffer[5]);
		emberAfAppPrintln("- Gyro X,Y,Z: %d %d %d\n",Gyro_x,Gyro_y,Gyro_z);

		data[0] = buffer[0];
		data[1] = buffer[1];
		data[2] = buffer[2];
		data[3] = buffer[3];
		data[4] = buffer[4];
		data[5] = buffer[5];
   }
   else
   {
	   emberAfAppPrintln("< Read GY273 Error >");
	   sensorCheckHandler();
	   data = 0;
   }
}
static void readGy45(int8u *data)
{
	int8u buffer[4];
	int8u error = i2cGy45Read(GY45_REG_STATUS, buffer, sizeof(buffer)/sizeof(buffer[0]));

	if(error == 0x00)
	{
	 /* High Res
		int16s Gyro_x = (buffer[1] << 6) | (buffer[2] >> 2 & 0x3F) ;
		int16s Gyro_y = (buffer[3] << 6) | (buffer[4] >> 2 & 0x3F) ;
		int16s Gyro_z = (buffer[5] << 6) | (buffer[6] >> 2 & 0x3F) ;
	*/
	 // Low Res
		int16s Gyro_x = buffer[1] << 6;
		int16s Gyro_y = buffer[2] << 6;
		int16s Gyro_z = buffer[3] << 6;
		if(Gyro_x > 8191) Gyro_x = (Gyro_x - 16384)/4;
		if(Gyro_y > 8191) Gyro_y = (Gyro_y - 16384)/4;
		if(Gyro_z > 8191) Gyro_z = (Gyro_z - 16384)/4;
		emberAfAppPrintln("- Gyro X,Y,Z: %d %d %d",Gyro_x,Gyro_y,Gyro_z);
	 /* Buffer check
		for(int i = 0; i <= 3; i++)
		{
			emberAfAppPrintln("- buffer[%x]: 0x%x", i, buffer[i]);
		}
	*/
		emberAfAppPrintln("");
		data[0] = buffer[1];
		data[1] = buffer[2];
		data[2] = buffer[3];
	/*High Res
		data[3] = buffer[4];
		data[4] = buffer[5];
		data[5] = buffer[6];
		data[6] = buffer[7];
	*/
	}
	else
	{
	   emberAfAppPrintln("< Read GY45 Error >: Code[0x%x]", error);
	   sensorCheckHandler();
	   data = 0;
	}
}

static int8u i2cSi7021Read(int8u _register_addr, int8u* _data, int8u _nBytes)
{
   int8u error;
   int8u command = SI_7021_READ_TEMPERATURE;
   halI2cWriteBytes(_register_addr, &command, 1);
   error = halI2cReadBytes(_register_addr,_data, _nBytes);

   return error;
}
static int8u i2cGy273Read(int8u _register_addr, int8u* _data, int8u _nBytes)
{
   int8u error;
   halI2cWriteBytes(GY273_I2C_ADDR, &_register_addr, 1);
   error = halI2cReadBytes(GY273_I2C_ADDR,_data, _nBytes);

   return error;
}
static int8u i2cGy45Read(int8u _register_addr, int8u* _data, int8u _nBytes)
{
   int8u error;
   halI2cWriteBytes(GY45_I2C_ADDR, &_register_addr, 1);
   error = halI2cReadBytes(GY45_I2C_ADDR,_data, _nBytes);

   return error;
}
#ifdef USING_AdcConverter
static void initADS1015Converter()
{
	m_gain = GAIN_TWOTHIRDS;
}
static void readADC_SingleEnded(int8u *data)
{
	int8u channel = 0;
	if(channel > 3)
	{
		emberAfAppPrintln("Pin Connected Error");
		data = 0;
	}

	// Start with default values
	int16u config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
					  ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
					  ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
					  ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
					  ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
					  ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default
	// Set PGA/voltage range
	config |= m_gain;
	config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
	// Set 'start single-conversion' bit
	config |= ADS1015_REG_CONFIG_OS_SINGLE;

	//writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);
	int8u writeData[3];
	writeData[0] = ADS1015_REG_POINTER_CONFIG;
	addInt16ToMessage(writeData, config, 1, 3);
	int8u writeStatus = halI2cWriteBytes(ADS1015_ADDR, writeData,3 );
	emberAfAppPrintln("I2C Write Status: 0x%x", writeStatus);

	if(writeStatus == 0x00)
	{
		int8u readData[2];
		i2cReadRegister(ADS1015_ADDR, ADS1015_REG_POINTER_CONVERT, readData, 2);
		int16u readValue = ((readData[0] << 8) |  readData[1])>>4;

		emberAfAppPrintln("I2C Read value: %d", readValue);
		data[0] = readData[0];
		data[1] = readData[1];
	}
	else
	{
		emberAfAppPrintln("< Read ADS1015 Converter Error >");
		data = 0;
	}
}
static int8u i2cReadRegister(int8u _device_addr, int8u _register_addr, int8u* _data, int8u _nBytes)
{
   int8u error;
   halI2cWriteBytes(_device_addr, &_register_addr, 1);
   error = halI2cReadBytes(_device_addr,_data, _nBytes);

   return error;
}
#endif
#ifdef USING_LMV324
static void disableLmv324()
{
	ADC_Reset(ADC0);
	CMU_ClockEnable(cmuClock_GPIO, false);     // Enable GPIO peripheral clock
	CMU_ClockEnable(cmuClock_TIMER1, false);   // Enable TIMER peripheral clock
	CMU_ClockEnable(cmuClock_ADC0, false);   // Enable ADC peripheral clock
	CMU_FreezeEnable(true);
}
static void initLmv324()
{
	/* Chip errata */
	CHIP_Init();

	CMU_HFRCOBandSet(cmuHFRCOFreq_1M0Hz);      // Set High Freq. RC Osc. to 1 MHz
	CMU_ClockEnable(cmuClock_GPIO, true);     // Enable GPIO peripheral clock
	CMU_ClockEnable(cmuClock_TIMER1, true);   // Enable TIMER peripheral clock
	CMU_ClockEnable(cmuClock_ADC0, true);   // Enable ADC peripheral clock


	// $[ADC0_Init]
	ADC_Init_TypeDef ADC0_init = ADC_INIT_DEFAULT;

	ADC0_init.ovsRateSel = adcOvsRateSel2;
	ADC0_init.warmUpMode = adcWarmupNormal;
	ADC0_init.timebase = ADC_TimebaseCalc(0);
	ADC0_init.prescale = ADC_PrescaleCalc(12000000, 0);
	ADC0_init.tailgate = 0;
	ADC0_init.em2ClockConfig = adcEm2Disabled;

	// [ADC0_Init]$
	ADC_Init(ADC0, &ADC0_init);

	// $[ADC0_InputConfiguration]
	ADC_InitSingle_TypeDef ADC0_init_single = ADC_INITSINGLE_DEFAULT;

	/* PRS settings */
	ADC0_init_single.prsEnable = 0;
	ADC0_init_single.prsSel = adcPRSSELCh0;

	/* Input(s) */
	ADC0_init_single.diff = 0;
	ADC0_init_single.posSel = adcPosSelAPORT3XCH10;//adcNegSelAPORT3YCH9;
	ADC0_init_single.negSel = adcNegSelVSS;//adcPosSelAPORT3XCH8;
	ADC0_init_single.reference = adcRef1V25;//adcRef5V;

	/* Generic conversion settings */
	ADC0_init_single.acqTime = adcAcqTime8;
	ADC0_init_single.resolution = adcResOVS;//adcRes12Bit;
	ADC0_init_single.leftAdjust = 0;
	ADC0_init_single.rep = 0;
	ADC0_init_single.singleDmaEm2Wu = 0;
	ADC0_init_single.fifoOverwrite = 0;

	ADC_InitSingle(ADC0, &ADC0_init_single);
}
static int32u readLmv324()
{
	halInternalDisableWatchDog(MICRO_DISABLE_WATCH_DOG_KEY);
	ADC_Start(ADC0, adcStartSingle);
	while(ADC0->STATUS & ADC_STATUS_SINGLEACT );
	//ADC0->SINGLEDATA;
	int32u data =  ADC_DataSingleGet(ADC0);
	emberAfAppPrintln("Noise: %d", data);
	halInternalEnableWatchDog();
	return data;
}
#endif
//----------------------------------------------------------------
void emberAfMainInitCallback(void)
{
	emberAfForceEndDeviceToStayAwake(1);
	//
	initApsFrame();
	emberAfSetTime(0);
	operationMode = nwkCheckMode;
	dataSchedulingMode = dataNone;
	sensorType = none;
	//Init
	halI2cInitialize();
	sensorCheckHandler();


	//emberEventControlSetActive(operationEvent);
	emberEventControlSetDelayMS(operationEvent, 5000);
}

void emberAfMainTickCallback(void)
{
	emberRunEvents(appEvents);
}

boolean emberAfMessageSentCallback(EmberOutgoingMessageType type,
                                   int16u indexOrDestination,
                                   EmberApsFrame* apsFrame,
                                   int16u msgLen,
                                   int8u* message,
                                   EmberStatus status)
{
	emberAfAppPrintln("---- Msg Sent: Cmd(x%x) Clu(x%2x) Src(x%2x) ",
												message[2],
												apsFrame->clusterId,
												indexOrDestination);
	return false;
}

boolean emberAfPreMessageReceivedCallback(EmberAfIncomingMessage* incomingMessage)
{
	emberAfAppPrintln("----[Pre]Msg Rec: Cmd(0x%x) Clu(0x%2x) Src(0x%2x) LQI(%d) RSSI(%d)",	incomingMessage->message[2],
																					incomingMessage->apsFrame->clusterId,
																					incomingMessage->source,
																					incomingMessage->lastHopLqi,
																					incomingMessage->lastHopRssi);
	if((incomingMessage->msgLen >= 3) && (incomingMessage->source == 0x0000))
	{
		//Pass command for command handler
		commandHandler(incomingMessage);
	}
	return false;
}

boolean emberAfStackStatusCallback(EmberStatus status)
{
	if(status == 0x90)
	{
		operationMode = schedulingMode;
		participationFailCount = 0;
	}
	else if(status == 0x91)
	{
		operationMode = nwkCheckMode;
		dataSchedulingMode = dataRequest;
		AckFlagFA = 0;
	}
	return false;
}



