#ifndef _h_onewire
#define _h_onewire

#include <stdint.h>

/** @def ONEWIRE_SEARCH when defined, one wire search functions will be available */
#define ONEWIRE_SEARCH

/** @def DS18B20_CRC_LOOKUP_TABLE when defined, a CRC lookup table will be used for CRC calculation */
#define ONEWIRE_CRC_LOOKUP_TABLE

// Timing configuration

#define ONEWIRE_START_RESET_TIME 480
#define ONEWIRE_START_RELEASE_TIME 80
#define ONEWIRE_START_WAIT_TIME 400

#define ONEWIRE_WRITE_HIGH_LOW_TIME 10
#define ONEWIRE_WRITE_HIGH_RELEASE_TIME 55
#define ONEWIRE_WRITE_LOW_LOW_TIME 65

#define ONEWIRE_READ_BEGIN_TIME 2
#define ONEWIRE_READ_LOW_TIME 2
#define ONEWIRE_READ_WAIT_TIME 50

// Port definitions

typedef uint8_t OneWire_Id;
typedef uint16_t OneWire_Counter;
typedef uint8_t OneWire_Byte;
typedef uint8_t OneWire_Size;
typedef uint8_t OneWire_Bool;
typedef uint64_t OneWire_Address;

#define OneWire_True 1
#define OneWire_False 0

// Generic enumerations

typedef enum OneWire_PinDirection
{
	OneWire_PinDir_Input,
	OneWire_PinDir_Output
} OneWire_PinDirection;

typedef enum OneWire_PinState
{
	OneWire_PinState_High,
	OneWire_PinState_Low
} OneWire_PinState;

typedef enum OneWire_Command
{
	OneWire_Cmd_Search = 			0xF0,
	OneWire_Cmd_Search_Alarm = 		0xEC
} OneWire_Command;

// Operation result codes

typedef enum OneWire_Result
{
	OneWire_NothingToDo,	/**< OneWire is not doing anything */
	OneWire_Success,		/**< Operation was successful */
	OneWire_Failed,			/**< Operation failed */
	OneWire_Working,		/**< Operation is still being processed */
	OneWire_Undefined		/**< Invalid result state, this should never occur */
} OneWire_Result;

// State machine state

typedef enum OneWire_State
{
	OneWire_Idle,			/**< OneWire is not doing anything */
	OneWire_Starting,		/**< OneWire reset is being processed */
	OneWire_Writing,		/**< Data is being transmitted */
	OneWire_Reading,		/**< Data is being read */
	OneWire_Searching,		/**< Device search is in progress */
	OneWire_SearchingAlarm	/**< Device search in alarm mode is in progress */
} OneWire_State;

typedef enum OneWire_StartState
{
	OneWire_Start_Begin,
	OneWire_Start_Delay1,
	OneWire_Start_Delay2,
	OneWire_Start_Delay3
} OneWire_StartState;

typedef enum OneWire_WriteState
{
	OneWire_Write_Begin,
	OneWire_Write_High_1,
	OneWire_Write_High_2,
	OneWire_Write_Low_1
} OneWire_WriteState;

typedef enum OneWire_ReadState
{
	OneWire_Read_Begin,
	OneWire_Read_1,
	OneWire_Read_2,
	OneWire_Read_3
} OneWire_ReadState;

typedef enum OneWire_SearchState
{
	OneWire_Search_Begin,
	OneWire_Search_Write_Command,
	OneWire_Search_Read,
	OneWire_Search_Write_Direction
} OneWire_SearchState;

// Definitions

typedef struct OneWire OneWire;

typedef void(*OneWire_SetPinDirection)(const OneWire *ow, OneWire_PinDirection dir);
typedef void(*OneWire_SetPinState)(const OneWire *ow, OneWire_PinState state);
typedef OneWire_PinState(*OneWire_ReadPin)(const OneWire *ow);
typedef void(*OneWire_StartTimer)(const OneWire *ow);
typedef OneWire_Counter(*OneWire_ReadTimer)(const OneWire *ow);
typedef void(*OneWire_Detected)(const OneWire *ow);

#ifdef ONEWIRE_SEARCH
typedef void(*OneWire_Search_Cb)(const OneWire *ow);
#endif

// Primary struct

typedef union OneWireSubState
{
	OneWire_StartState startState;			/**< State of one wire start */
	OneWire_WriteState writeState;			/**< State of one wire write */
	OneWire_ReadState readState;			/**< State of one wire read */
} OneWireSubState;

typedef struct OneWire
{
	OneWire_Id id;							/**< Used-specific onewire interface ID used for interface identification in case more than one onewire interfaces are used */

	OneWire_Byte *buffer;					/**< Attached data buffer */
	OneWire_Size bufferLength;				/**< Data buffer length */
	OneWire_Byte bitLength;
	OneWire_Size byteIndex;					/**< Currently processed byte */
	OneWire_Size bitIndex;					/**< Currently processed bit */

#ifdef ONEWIRE_SEARCH
	OneWire_SearchState searchState;		/**< State of one wire device search */
	OneWire_Address searchedAddress;		/**< Currently searched address */
	OneWire_Address searchBitIdx;
	OneWire_Address searchLastDiscrepancy;
	OneWire_Byte searchHelper;

	OneWire_Search_Cb onSearchDone;
#endif

	OneWire_SetPinDirection setPinDir;		/**< Set pin direction */
	OneWire_SetPinState setPinState;		/**< Write pin */
	OneWire_ReadPin readPin;				/**< Read pin state */

	OneWire_StartTimer startTimer;			/**< Restart timer counter */
	OneWire_ReadTimer readTimer;			/**< Read timer value [us] */

	OneWire_Detected detectedCallback;		/**< [Optional] Callback fired when one wire presence pulse is detected */

	OneWire_State state;					/**< Currently processed function */
	OneWireSubState substate;				/**< Current one wire sub state */
} OneWire;

// Public functions

/**
 * @brief Utility function to initialize required callbacks to OneWire module
 *
 * @param id user-assigned ID used to identify which OneWire interface is being used in callbacks
 * @param ow pointer to OneWire structure @see OneWire
 * @param setPinDir callback to set pin direction
 * @param setPinState callback to set pin state (write pin value)
 * @param readPin callback to read pin state (read pin value)
 * @param startTimer callback to restart timer counter - counter must have 1us period
 * @param readTimer callback to read timer counter [us]
 */
void onewireInit(OneWire *ow, OneWire_Id id, OneWire_SetPinDirection setPinDir, OneWire_SetPinState setPinState, OneWire_ReadPin readPin, OneWire_StartTimer startTimer, OneWire_ReadTimer readTimer);

/**
 * @brief Process OneWire state machine - this function is required to work in program's main loop
 *
 * @param ow pointer to OneWire structure @see OneWire
 * @return current operation status @see OneWire_Result
 */
OneWire_Result onewireProcess(OneWire *ow);

/**
 * @brief Begin OneWire start operation
 *
 * @param ow pointer to OneWire structure @see OneWire
 */
void onewireStart(OneWire *ow);

/**
 * @brief Begin OneWire write operation of specified buffer and given length
 *
 * @param ow pointer to OneWire structure @see OneWire
 * @param buffer buffer containing data to write
 * @param length length of given buffer
 */
void onewireWrite(OneWire *ow, OneWire_Byte *buffer, OneWire_Size length);

/**
 * @brief Begin OneWire read operation and store the result to given buffer
 *
 * @param ow pointer to OneWire structure @see OneWire
 * @param buffer buffer to store the data to
 * @param length maximum length to read
 */
void onewireRead(OneWire *ow, OneWire_Byte *buffer, OneWire_Size length);

/**
 * @brief Begin OneWire search operation
 *
 * @param ow pointer to OneWire structure @see OneWire
 * @param alarm when set to OneWire_True then only devices with alarm flag set will be found
 */
void onewireSearch(OneWire *ow, OneWire_Bool alarm);

/**
 * @brief Begin OneWire search operation for devices from specific family
 *
 * @param ow pointer to OneWire structure @see OneWire
 * @param alarm when set to OneWire_True then only devices with alarm flag set will be found
 */
void onewireSearchTarget(OneWire *ow, OneWire_Bool alarm, OneWire_Byte familyCode);

/**
 * @brief Break currently processing search
 *
 * @param ow pointer to OneWire structure @see OneWire
 */
void onewireAbortSearch(OneWire *ow);

/**
 * @brief Calculate one wire CRC
 *
 * @param buffer one wire data buffer to verify
 * @param len complete length of one wire data
 * @return
 */
OneWire_Byte onewireCrc(OneWire_Byte *buffer, OneWire_Size len);

static inline void onewireWait(OneWire *ow)
{
  do {
	  onewireProcess(ow);
  } while(ow->state != OneWire_Idle);
}

#endif
