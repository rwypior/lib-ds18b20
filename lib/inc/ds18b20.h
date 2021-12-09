#ifndef _h_temperature
#define _h_temperature

#include "onewire.h"

#include <stdint.h>

// Porting definitions

typedef uint8_t DS18B20_Byte;
typedef uint8_t DS18B20_Size;
typedef uint8_t DS18B20_Bool;
typedef uint32_t DS18B20_Time;
typedef uint64_t DS18B20_Address;

#define DS18B20_False 0
#define DS18B20_True 1

// Properties

/** @def DS18B20_BUFFER_SIZE DS18B20 read/write buffer size in bytes */
#define DS18B20_BUFFER_SIZE 13

/** @def DS18B20_FLOAT_ENABLED If this is enabled, then float converting utility function is available */
#define DS18B20_FLOAT_ENABLED 1

/** @def DS18B20_ROM_NONE Placeholder for blank ROM */
#define DS18B20_ROM_NONE 0

#define DS18B20_FAMILY_CODE 0x28

#define DS18B20_WAIT_RES9 95000
#define DS18B20_WAIT_RES10 190000
#define DS18B20_WAIT_RES11 400000
#define DS18B20_WAIT_RES12 800000

// DS18B20 commands

#define DS18B20_SEARCH_ROM 0xF0
#define DS18B20_READ_ROM 0x33
#define DS18B20_MATCH_ROM 0x55
#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_ALARM_SEARCH 0xEC
#define DS18B20_CONVERT 0x44
#define DS18B20_WRITE_SCRATCHPAD 0x4E
#define DS18B20_READ_SCRATCHPAD 0xBE
#define DS18B20_COPY_SCRATCHPAD 0x48
#define DS18B20_RECALL_EEPROM 0xB8
#define DS18B20_READ_POWER_SUPPLY 0xB4

// Definitions

/**
 * @brief DS18B20 resolution specifiers
 */
typedef enum DS18B20Resolution
{
	DS18B20_Resolution_9 = 0x1F, 	/**< 9-bit resolution - need approx. 95 millis conversion time */
	DS18B20_Resolution_10 = 0x3F,	/**< 10-bit resolution - need approx. 190 millis conversion time */
	DS18B20_Resolution_11 = 0x5F,	/**< 11-bit resolution - need approx 400 millis conversion time */
	DS18B20_Resolution_12 = 0x7F 	/**< 12-bit resolution - need approx 800 millis conversion time */
} DS18B20Resolution;

/**
 * @brief Special callback flags used for searching operations
 */
typedef enum DS18B20CallbackFlags
{
	DS18B20_Callback_Normal,       		/**< No special flag has been set */
	DS18B20_Callback_SearchFinished,	/**< Search has been finished */
	DS18B20_Callback_NoParasitic,		/**< No parasitic-powered devices found */
	DS18B20_Callback_Parasitic			/**< At least one parasitic-powered device found */
}DS18B20CallbackFlags;

/**
 * @brief DS18B20 result enumeration
 */
typedef enum DS18B20Result
{
	DS18B20_Result_Ok,		/**< Request was accepted *//**< DS18B20_Result_Ok */
	DS18B20_Result_Busy		/**< Module is busy */     /**< DS18B20_Result_Busy */
} DS18B20Result;

/**
 * @brief Data mode specifiers
 */
typedef enum DS18b20ReadMode
{
	DS18b20_Read_Temperature	= 0x02,		/**< Read only first two bytes to get the temperature */
	DS18b20_Read_UserByte1		= 0x03, 	/**< Read up to first user byte */
	DS18b20_Read_UserByte2		= 0x04, 	/**< Read up to second user byte */
	DS18b20_Read_Config			= 0x05,   	/**< Read up to configuration register */
	DS18b20_Read_CRC			= 0x09      /**< Read whole scratchpad up to CRC */
} DS18b20ReadMode;

typedef enum DS18b20Error
{
	DS18b20_Success,		/**< No error has been recorded */
	DS18b20_Error_CRC,		/**< Invalid CRC */
	DS18b20_Error			/**< Unspecified error encountered */
} DS18b20Error;

/**
 * @brief Primary DS18B20 status enumeration
 */
typedef enum DS18b20State
{
	DS18b20_State_Idle,           	/**< No job is currently running */
	DS18b20_State_Convert,        	/**< Temperature conversion is in progress */
	DS18b20_State_ReadScratchpad, 	/**< Scratchpad is being read */
	DS18b20_State_ReadRom,        	/**< ROM is being read */
	DS18b20_State_WriteScratchpad,	/**< Scratchpad is being written */
	DS18b20_State_CopyScratchpad, 	/**< Scratchpad is being copied to the EEPROM */
	DS18b20_State_RecallEeprom,   	/**< EEPROM is being copied to the scratchpad */
	DS18b20_State_ReadPowersupply,  /**< Powersupply test is in progress */
	DS18b20_State_Finished,        	/**< An operation has been finished and no job is currently running */
	DS18b20_State_Searching			/**< Device search is in progress */
} DS18b20State;

// Sub status enums

typedef enum DS18b20ConvertState
{
	DS18b20_Convert_Begin,
	DS18b20_Convert_Start,
	DS18b20_Convert_Write,
	DS18b20_Convert_Processing,
	DS18b20_Convert_Delay
} DS18b20ConvertState;

typedef enum DS18b20ScratchpadState
{
	DS18b20_Scratchpad_Begin,
	DS18b20_Scratchpad_Start,
	DS18b20_Scratchpad_Write,
	DS18b20_Scratchpad_Processing,
	DS18b20_Scratchpad_Reading
} DS18b20ScratchpadState;

typedef enum DS18b20ReadDataState
{
	DS18b20_ReadData_Start,
	DS18b20_ReadData_Reading
} DS18b20ReadDataState;

typedef enum DS18b20ReadROMState
{
	DS18b20_ReadROM_Start,
	DS18b20_ReadROM_Writing,
	DS18b20_ReadROM_Process,
	DS18b20_ReadROM_Reading
} DS18b20ReadROMState;

typedef enum DS18b20CopyScratchpadState
{
	DS18b20_CopyScratchpad_Start,
	DS18b20_CopyScratchpad_Writing,
	DS18b20_CopyScratchpad_Process,
	DS18b20_CopyScratchpad_Wait
} DS18b20CopyScratchpadState;

typedef enum DS18b20RecallEepromState
{
	DS18b20_RecallEeprom_Start,
	DS18b20_RecallEeprom_Writing,
	DS18b20_RecallEeprom_Process
} DS18b20RecallEepromState;

typedef enum DS18b20ReadPowerSupplyState
{
	DS18b20_ReadPowerSupply_Start,
	DS18b20_ReadPowerSupply_Writing,
	DS18b20_ReadPowerSupply_Process,
	DS18b20_ReadPowerSupply_Read
} DS18b20ReadPowerSupplyState;

// Forward declarations

typedef struct DS18B20 DS18B20;

// Function typedefs

/**
 * @brief Callback function prototype.
 *
 * @param ds Pointer to DS18B20 structure
 * @param operation Operation that has been finished
 * @param addr Device address on which the operation has been finished
 */
typedef void(*DS18B20_Callback)(DS18B20 *ds, DS18b20State operation, DS18B20_Address addr, DS18B20CallbackFlags flags);

// Primary struct

/**
 * @brief Union holding DS18B20 operation state
 */
typedef union DS18B20SubState
{
	DS18b20ConvertState convertState;					/**< Status of conversion */
	DS18b20ScratchpadState scratchpadState;				/**< Status of scratchpad read */
	DS18b20ReadROMState readRomState;					/**< Status of ROM read */
	DS18b20ReadDataState readState;						/**< Status of data read */
	DS18b20CopyScratchpadState copyScratchpadState;		/**< Status of scratchpad copy */
	DS18b20RecallEepromState recallEepromState;			/**< Status of eeprom read */
	DS18b20ReadPowerSupplyState powerSupplyState;		/**< Status of power supply read */
} DS18B20SubState;

/**
 * @brief Primary DS18B20 structure
 */
typedef struct DS18B20
{
	OneWire *oneWire;									/**< Pointer to OneWire communication interface structure */

	DS18B20_Callback onOperationFinished;				/**< Callback called once an operation is finished */

	DS18B20_Address currentAddress;						/**< Currently used ROM address, change this value to poll different sensors, or set it to DS18B20_ROM_NONE to skip */
	DS18B20Resolution resolution;						/**< Sensor resolution @see DS18B20Resolution */
	DS18b20ReadMode readMode;							/**< Specifies what data will be read from the scratchpad */

	DS18b20State state;									/**< Currently processing function */
	DS18B20SubState substate;							/**< Current operation state */

	DS18B20_Byte datalen;								/**< Number of bytes scheduled to read */
	DS18B20_Byte buffer[DS18B20_BUFFER_SIZE];			/**< Read/write buffer */
	DS18B20_Byte temp;									/**< Temporary byte used to track eeprom recall operation */
} DS18B20;

// Public functions

/**
 * @brief Initialize DS18B20 module
 *
 * @param ds pointer to DS18B20 structure @see DS18B20
 * @param ow pointer to OneWire interface structure @see OneWire
 */
void ds18b20Init(DS18B20 *ds, OneWire *ow);

/**
 * @brief Primary DS18B20 processing structure - should be called in main loop

 * @param ds pointer to DS18B20 structure @see DS18B20*
 * @return current DS18B20 module state
 */
DS18b20State ds18b20Process(DS18B20 *ds);

/**
 * @brief Request DS18B20 conversion
 *
 * @param ds pointer to DS18B20 structure @see DS18B20
 * @param DS28B20_Address address sensor's ROM code, if set to 0 then ROM will be skipped
 * @return DS18B20_Result_Ok if request was accepted, or DS18B20_Result_Busy if conversion is already ongoing @see DS18B20Result
 */
DS18B20Result ds18b20BeginConversion(DS18B20 *ds, DS18B20_Address address);

/**
 * @brief Request DS18B20 read cycle. The address is taken from DS18B20 structure field 'currentAddress'
 *
 * @param ds pointer to DS18B20 structure @see DS18B20
 * @return DS18B20_Result_Ok if request was accepted, or DS18B20_Result_Busy if conversion is already ongoing @see DS18B20Result
 */
DS18B20Result ds18b20ReadScratchpad(DS18B20 *ds, DS18B20_Address address);

/**
 * @brief Request DS18B20 ROM code readout. The address is taken from DS18B20 structure field 'currentAddress'
 *
 * @param ds pointer to DS18B20 structure @see DS18B20
 * @return DS18B20_Result_Ok if request was accepted, or DS18B20_Result_Busy if conversion is already ongoing @see DS18B20Result
 */
DS18B20Result ds18b20RequestReadRom(DS18B20 *ds);

/**
 * @brief Request writing data to DS18B20 scratchpad. Three registers are available to write - two user bytes and configuration register
 *
 * @param ds ds pointer to DS18B20 structure @see DS18B20
 * @param bytes bytes to write
 * @param count number of bytes to write
 * @return
 */
DS18B20Result ds18b20WriteScratchpad(DS18B20 *ds, DS18B20_Byte *bytes, DS18B20_Size count, DS18B20_Address address);

/**
 * @brief Request copy of the scratchpad parameters to sensor's EEPROM
 *
 * @param ds ds pointer to DS18B20 structure @see DS18B20
 * @return
 */
DS18B20Result ds18b20CopyScratchpad(DS18B20 *ds, DS18B20_Address address);

/**
 * @brief Request loading of the data from EEPROM to scratchpad
 *
 * @param ds ds pointer to DS18B20 structure @see DS18B20
 * @return
 */
DS18B20Result ds18b20RecallEeprom(DS18B20 *ds, DS18B20_Address address);

/**
 * @brief Request DS18B20 resolution setting
 *
 * @param ds pointer to DS18B20 structure @see DS18B20
 * @param resolution resolution to set @see DS18B20Resolution
 * @return DS18B20_Result_Ok if request was accepted, or DS18B20_Result_Busy if conversion is already ongoing @see DS18B20Result
 */
DS18B20Result ds18b20SetResolution(DS18B20 *ds, DS18B20Resolution resolution, DS18B20_Byte *userbytes, DS18B20_Address address);

/**
 * @brief Request test if any device on the bus is using parasite power. When the test is complete, a callback will be called
 * with either DS18B20_Callback_Parasitic parameter if any parasitic-powered device is found, or DS18B20_Callback_NoParasitic otherwise
 *
 * @param ds ds pointer to DS18B20 structure @see DS18B20
 * @return
 */
DS18B20Result ds18b20ReadPowerSupply(DS18B20 *ds);

/**
 * @brief Verify CRC of DS18B20 data. This only works when module's `readMode` is set to DS18b20_Read_CRC.
 *
 * @param buffer data buffer to verify
 * @param len complete length of data (including CRC byte)
 * @return DS18B20_True on successful verification or DS18B20_False otherwise
 */
static inline DS18B20_Bool ds18b20VerifyCrc(DS18B20 *ds)
{
	return ds->readMode == DS18b20_Read_CRC ? onewireCrc(ds->buffer, DS18b20_Read_CRC) == ds->buffer[DS18b20_Read_CRC - 1] : DS18B20_True;
}

/**
 * @brief Check if sensor is geniune by its ROM code address.
 *
 * This functions has been based on info from <a href="https://github.com/cpetrich/counterfeit_DS18B20">this github repository</a>.
 *
 * @param DS18B20_Address address device address
 * @return DS18B20_True if test has passed or DS18B20_False otherwise
 */
static inline DS18B20_Bool ds18b20CheckAuthentic(DS18B20_Address address)
{
	return
			( (address >> 0)  & 0xFF ) == DS18B20_FAMILY_CODE &&
			( (address >> 40) & 0xFF ) == 0x00 &&
			( (address >> 48) & 0xFF ) == 0x00;
}

/**
 * @brief Wait for DS operation to finish. This function is blocking.
 *
 * @param ds pointer to DS18B20 structure @see DS18B20
 */
static inline void ds18b20Wait(DS18B20 *ds)
{
  do {
	  ds18b20Process(ds);
  } while(ds->state != DS18b20_State_Finished);
}

#if DS18B20_FLOAT_ENABLED

/**
 * @brief Convert DS18B20 buffer into real temperature. This function is only valid after ds18b20Process function returns DS18b20_State_Finished status code.
 *
 * @param ds pointer to DS18B20 structure @see DS18B20
 * @return read temperature in Celsius degrees
 */
static inline float ds18b20GetTemperatureFloat(const DS18B20 *ds)
{
	return
			( ( ( ds->buffer[0] | (ds->buffer[1] << 8) ) & 0x07FF ) >> 4 ) +
			0.5    * ((ds->buffer[0] & 0x08) >> 3) +
			0.25   * ((ds->buffer[1] & 0x04) >> 2) +
			0.125  * ((ds->buffer[2] & 0x02) >> 1) +
			0.0625 * ((ds->buffer[3] & 0x01) >> 0);
	//return ((float)(ds->buffer[0] | (ds->buffer[1] << 8) )) * ds18b20GetResolutionDivider(ds);
}
#endif

#endif
