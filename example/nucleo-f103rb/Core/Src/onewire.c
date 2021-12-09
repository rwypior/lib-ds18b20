#include "onewire.h"

// Private functions

static inline OneWire_Bool ow_timerPassed(const OneWire *ow, OneWire_Counter threshold)
{
	return ow->readTimer(ow) >= threshold;
}

// START

static OneWire_Result processStart(OneWire *ow)
{
	switch(ow->substate.startState)
	{
	case OneWire_Start_Begin:
		ow->setPinDir(ow, OneWire_PinDir_Output);
		ow->setPinState(ow, OneWire_PinState_Low);
		ow->startTimer(ow);
		ow->substate.startState = OneWire_Start_Delay1;
		return OneWire_Working;

	case OneWire_Start_Delay1:
		if (ow_timerPassed(ow, ONEWIRE_START_RESET_TIME))
		{
			ow->setPinDir(ow, OneWire_PinDir_Input);
			ow->startTimer(ow);
			ow->substate.startState = OneWire_Start_Delay2;
		}
		return OneWire_Working;

	case OneWire_Start_Delay2:
		if (ow_timerPassed(ow, ONEWIRE_START_RELEASE_TIME))
		{
			if (ow->detectedCallback && ow->readPin(ow) == OneWire_PinState_Low)
				ow->detectedCallback(ow);

			ow->startTimer(ow);
			ow->substate.startState = OneWire_Start_Delay3;
		}
		return OneWire_Working;

	case OneWire_Start_Delay3:
		if (ow_timerPassed(ow, ONEWIRE_START_WAIT_TIME))
		{
			ow->substate.startState = OneWire_Start_Begin;
			return OneWire_Success;
		}
		return OneWire_Working;
	}

	return OneWire_Working;
}

// WRITE

static OneWire_Result processWrite(OneWire *ow)
{
	OneWire_Size data = ow->buffer[ow->byteIndex];
	OneWire_Size bitState = data & (1 << ow->bitIndex);

	switch(ow->substate.writeState)
	{
	// Begin
	case OneWire_Write_Begin:
		ow->setPinState(ow, OneWire_PinState_Low);
		ow->setPinDir(ow, OneWire_PinDir_Output);
		ow->substate.writeState = bitState == 0 ? OneWire_Write_Low_1 : OneWire_Write_High_1;
		ow->startTimer(ow);
		return OneWire_Working;

	// High bit
	case OneWire_Write_High_1:
		if (ow_timerPassed(ow, ONEWIRE_WRITE_HIGH_LOW_TIME))
		{
			ow->setPinDir(ow, OneWire_PinDir_Input);
			ow->startTimer(ow);
			ow->substate.writeState = OneWire_Write_High_2;
		}
		return OneWire_Working;

	case OneWire_Write_High_2:
		if (ow_timerPassed(ow, ONEWIRE_WRITE_HIGH_RELEASE_TIME))
		{
			if (++ow->bitIndex >= ow->bitLength)
			{
				ow->bitIndex = 0;
				if (++ow->byteIndex >= ow->bufferLength)
				{
					ow->substate.writeState = OneWire_Write_Begin;
					ow->byteIndex = 0;
					return OneWire_Success;
				}
			}

			ow->substate.writeState = OneWire_Write_Begin;
		}
		return OneWire_Working;

	// Low bit
	case OneWire_Write_Low_1:
		if (ow_timerPassed(ow, ONEWIRE_WRITE_LOW_LOW_TIME))
		{
			ow->setPinDir(ow, OneWire_PinDir_Input);

			if (++ow->bitIndex >= ow->bitLength)
			{
				ow->bitIndex = 0;
				if (++ow->byteIndex >= ow->bufferLength)
				{
					ow->substate.writeState = OneWire_Write_Begin;
					ow->byteIndex = 0;
					return OneWire_Success;
				}
			}

			ow->substate.writeState = OneWire_Write_Begin;
		}

		return OneWire_Working;
	}

	return OneWire_Working;
}

// READ

static OneWire_Result processRead(OneWire *ow)
{
	switch(ow->substate.readState)
	{
	case OneWire_Read_Begin:
		ow->setPinDir(ow, OneWire_PinDir_Input);
		ow->startTimer(ow);
		ow->substate.readState = OneWire_Read_1;
		return OneWire_Working;

	case OneWire_Read_1:
		if (ow_timerPassed(ow, ONEWIRE_READ_BEGIN_TIME))
		{
			ow->setPinState(ow, OneWire_PinState_Low);
			ow->setPinDir(ow, OneWire_PinDir_Output);
			ow->startTimer(ow);
			ow->substate.readState = OneWire_Read_2;
		}
		return OneWire_Working;

	case OneWire_Read_2:
		if (ow_timerPassed(ow, ONEWIRE_READ_LOW_TIME))
		{
			ow->setPinDir(ow, OneWire_PinDir_Input);

			OneWire_Bool bit = ow->readPin(ow) == OneWire_PinState_High;

			ow->buffer[ow->byteIndex] |= bit << ow->bitIndex;

			if (++ow->bitIndex >= ow->bitLength)
			{
				ow->bitIndex = 0;
				if (++ow->byteIndex >= ow->bufferLength)
				{
					ow->substate.readState = OneWire_Read_Begin;
					ow->byteIndex = 0;
					//ow->state = OneWire_Idle;
					return OneWire_Success;
				}
			}

			ow->startTimer(ow);
			ow->substate.readState = OneWire_Read_3;
			return OneWire_Working;
		}
		return OneWire_Working;

	case OneWire_Read_3:
		if (ow_timerPassed(ow, ONEWIRE_READ_WAIT_TIME))
			ow->substate.readState = OneWire_Read_1;
		return OneWire_Working;
	}

	return OneWire_Working;
}

// SEARCH

static OneWire_Result processSearch(OneWire *ow)
{
	switch(ow->searchState)
	{
	case OneWire_Search_Begin:
		if (processStart(ow) == OneWire_Success)
		{
			ow->searchedAddress = 0;
			ow->searchBitIdx = 1;

			ow->searchState = OneWire_Search_Write_Command;
			ow->searchHelper = ow->state == OneWire_Searching ? OneWire_Cmd_Search : OneWire_Cmd_Search_Alarm;
			ow->buffer = &ow->searchHelper;
			ow->bufferLength = 1;
			ow->bitLength = 8;
		}
		return OneWire_Working;

	case OneWire_Search_Write_Command:
		if (processWrite(ow) == OneWire_Success)
		{
			ow->searchHelper = 0;
			ow->buffer = &ow->searchHelper;
			ow->bitLength = 2;
			ow->bufferLength = 1;
			ow->searchState = OneWire_Search_Read;
		}
		return OneWire_Working;

	case OneWire_Search_Read:
		if (processRead(ow) == OneWire_Success)
		{
			OneWire_Byte bit = ow->buffer[0];

			if (bit == 0x03)
			{
				ow->searchState = OneWire_Search_Begin;
				return OneWire_Success;
			}

			if (bit == 0x01 || bit == 0x02)
				bit &= 0x01;
			else
			{
				if (ow->searchLastDiscrepancy >= (ow->searchBitIdx << 1))
					bit = !(ow->searchLastDiscrepancy & ow->searchBitIdx);
				else
				{
					bit = (ow->searchLastDiscrepancy & ow->searchBitIdx) != 0;
					ow->searchLastDiscrepancy ^= ow->searchBitIdx;
				}
			}

			ow->searchedAddress |= bit ? ow->searchBitIdx : 0;

			ow->searchHelper = bit;
			ow->buffer = &ow->searchHelper;
			ow->bufferLength = 1;
			ow->bitLength = 1;

			ow->searchState = OneWire_Search_Write_Direction;
		}
		return OneWire_Working;

	case OneWire_Search_Write_Direction:
		if (processWrite(ow) == OneWire_Success)
		{
			ow->searchBitIdx <<= 1;

			if (ow->searchBitIdx)
			{
				ow->searchHelper = 0;
				ow->buffer = &ow->searchHelper;
				ow->bitLength = 2;
				ow->bufferLength = 1;
				ow->searchState = OneWire_Search_Read;
			}
			else
			{
				ow->onSearchDone(ow);

				ow->searchBitIdx = 1;

				ow->searchState = OneWire_Search_Begin;
				if (!ow->searchLastDiscrepancy)
					return OneWire_Success;
			}

		}
		return OneWire_Working;
	}

	return OneWire_Working;
}

// Public functions

void onewireInit(
	OneWire *ow,
	OneWire_Id id,
	OneWire_SetPinDirection setPinDir,
	OneWire_SetPinState setPinState,
	OneWire_ReadPin readPin,
	OneWire_StartTimer startTimer,
	OneWire_ReadTimer readTimer
)
{
	ow->id = id;
	ow->setPinDir = setPinDir;
	ow->setPinState = setPinState;
	ow->readPin = readPin;
	ow->startTimer = startTimer;
	ow->readTimer = readTimer;
}

OneWire_Result onewireProcess(OneWire *ow)
{
	OneWire_Result res = OneWire_Undefined;

	switch(ow->state)
	{
	case OneWire_Idle: return OneWire_NothingToDo;
	case OneWire_Starting: res = processStart(ow); break;
	case OneWire_Writing: res = processWrite(ow); break;
	case OneWire_Reading: res = processRead(ow); break;
	case OneWire_Searching:
	case OneWire_SearchingAlarm:
		res = processSearch(ow);
		break;
	}

	if (res == OneWire_Success)
		ow->state = OneWire_Idle;

	return res;
}

void onewireStart(OneWire *ow)
{
	ow->state = OneWire_Starting;
	ow->substate.startState = OneWire_Start_Begin;
}

void onewireWrite(OneWire *ow, OneWire_Byte *buffer, OneWire_Size length)
{
	ow->state = OneWire_Writing;
	ow->substate.startState = OneWire_Write_Begin;
	ow->buffer = buffer;
	ow->bufferLength = length;
	ow->bitLength = 8;
}

void onewireRead(OneWire *ow, OneWire_Byte *buffer, OneWire_Size length)
{
	ow->state = OneWire_Reading;
	ow->substate.startState = OneWire_Read_Begin;
	ow->buffer = buffer;
	ow->bufferLength = length;
	ow->bitLength = 8;
}

static inline void onewireSearchReset(OneWire *ow)
{
	ow->searchLastDiscrepancy = 0;
}

void onewireSearch(OneWire *ow, OneWire_Bool alarm)
{
	onewireSearchReset(ow);
	ow->state = OneWire_Searching;
	ow->searchState = OneWire_Search_Begin;
}

void onewireSearchTarget(OneWire *ow, OneWire_Bool alarm, OneWire_Byte familyCode)
{
	onewireSearchReset(ow);
	ow->state = OneWire_SearchingAlarm;
	ow->searchState = OneWire_Search_Begin;
}

void onewireAbortSearch(OneWire *ow)
{
	if (ow->state == OneWire_Searching || ow->state == OneWire_SearchingAlarm)
		ow->state = OneWire_Idle;
}

OneWire_Byte onewireCrc(OneWire_Byte *buffer, OneWire_Size len)
{
	OneWire_Byte crc = 0;

	while (--len)
	{
		OneWire_Byte inbyte = *buffer++;
		for (OneWire_Byte i = 8; i > 0; --i)
		{
			OneWire_Byte mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix)
				crc ^= 0x8C;
			inbyte >>= 1;
		}
	}

	return crc;
}
