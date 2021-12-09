#include "ds18b20.h"

// Private functions

static inline void clearbuffer(DS18B20 *ds)
{
	for (DS18B20_Size i = 0; i < sizeof(ds->buffer); ++i)
		ds->buffer[i] = 0;
}

static inline DS18B20_Size cpy(DS18B20_Byte *target, const DS18B20_Byte *source, DS18B20_Size length)
{
	for (DS18B20_Size i = 0; i < length; ++i)
		target[i] = source[i];
	return length;
}

static inline DS18B20_Bool ds_timerPassed(DS18B20 *ds, unsigned int threshold)
{
	static unsigned int passedMillis;
	unsigned int t = ds->oneWire->readTimer(ds->oneWire);

	if (t >= 1000)
	{
		ds->oneWire->startTimer(ds->oneWire);
		++passedMillis;
	}

	unsigned int mil = threshold / 1000;
	unsigned int mic = threshold - mil * 1000;
	if (passedMillis >= mil && t >= mic)
	{
		passedMillis = 0;
		return DS18B20_True;
	}

	return DS18B20_False;
}

static inline void ds18b20write(DS18B20 *ds, const DS18B20_Byte *data, DS18B20_Size length)
{
	DS18B20_Size len = length > sizeof(ds->buffer) ? sizeof(ds->buffer) : length;
	for (DS18B20_Size i = 0; i < len; ++i)
		ds->buffer[i] = data[i];
	onewireWrite(ds->oneWire, ds->buffer, len);
}

static inline DS18B20_Size ds18b20prepareBuffer(DS18B20 *ds, DS18B20_Byte dsCmd, DS18B20_Address romAddress, DS18B20_Size paramCount, DS18B20_Byte *params)
{
	DS18B20_Size i = 0;
	ds->buffer[i++] = romAddress ? DS18B20_MATCH_ROM : DS18B20_SKIP_ROM;
	if (romAddress)
		i += cpy(&ds->buffer[i], (DS18B20_Byte*)&romAddress, sizeof(romAddress));
	ds->buffer[i++] = dsCmd;
	if (paramCount)
		i += cpy(&ds->buffer[i], params, paramCount);

	return i;
}

static inline void ds18b20writeCommand(DS18B20 *ds, DS18B20_Byte dsCmd, DS18B20_Address romAddress, DS18B20_Size paramCount, DS18B20_Byte *params)
{
	DS18B20_Size size = ds18b20prepareBuffer(ds, dsCmd, romAddress, paramCount, params);
	onewireWrite(ds->oneWire, ds->buffer, size);
}

static inline DS18B20_Time ds18b20conversionTime(const DS18B20 *ds)
{
	switch(ds->resolution)
	{
	case DS18B20_Resolution_9: return DS18B20_WAIT_RES9;
	case DS18B20_Resolution_10: return DS18B20_WAIT_RES10;
	case DS18B20_Resolution_11: return DS18B20_WAIT_RES11;
	case DS18B20_Resolution_12: return DS18B20_WAIT_RES12;
	}

	return 0;
}

// CONVERT

static void processConvert(DS18B20 *ds)
{
	switch(ds->substate.convertState)
	{
	case DS18b20_Convert_Begin:
		onewireStart(ds->oneWire);
		ds->substate.convertState = DS18b20_Convert_Start;
		break;

	case DS18b20_Convert_Start:
	{
		OneWire_Result res = onewireProcess(ds->oneWire);
		if (res == OneWire_Success)
		{
			ds->substate.convertState = DS18b20_Convert_Write;
			ds->oneWire->startTimer(ds->oneWire);
		}
		else if (res == OneWire_Failed)
		{
			ds->substate.convertState = DS18b20_Convert_Begin;
		}
	}
		break;

	case DS18b20_Convert_Write:
		if (ds_timerPassed(ds, 1000))
		{
			ds18b20writeCommand(ds, DS18B20_CONVERT, ds->currentAddress, 0, 0);

			ds->substate.convertState = DS18b20_Convert_Processing;
		}
		break;

	case DS18b20_Convert_Processing:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->substate.convertState = DS18b20_Convert_Delay;
			ds->oneWire->startTimer(ds->oneWire);
		}
		break;

	case DS18b20_Convert_Delay:
		if (ds_timerPassed(ds, ds18b20conversionTime(ds)))
		{
			ds->state = DS18b20_State_Finished;
			ds->substate.convertState = DS18b20_Convert_Begin;

			if (ds->onOperationFinished)
				ds->onOperationFinished(ds, DS18b20_State_Convert, DS18B20_ROM_NONE, DS18B20_Callback_Normal);

			ds->currentAddress = DS18B20_ROM_NONE;
		}
		break;
	}
}

// READ SCRATCHPAD

static void processReadScratchpad(DS18B20 *ds)
{
	switch(ds->substate.scratchpadState)
	{
	case DS18b20_Scratchpad_Begin:
		onewireStart(ds->oneWire);
		ds->substate.scratchpadState = DS18b20_Scratchpad_Start;
		break;

	case DS18b20_Scratchpad_Start:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->substate.scratchpadState = DS18b20_Scratchpad_Write;
			ds->oneWire->startTimer(ds->oneWire);
		}
		break;

	case DS18b20_Scratchpad_Write:
		if (ds_timerPassed(ds, 1000))
		{
			ds18b20writeCommand(ds, DS18B20_READ_SCRATCHPAD, ds->currentAddress, 0, 0);
			ds->substate.scratchpadState = DS18b20_Scratchpad_Processing;
		}
		break;

	case DS18b20_Scratchpad_Processing:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->substate.scratchpadState = DS18b20_Scratchpad_Reading;

			clearbuffer(ds);
			onewireRead(ds->oneWire, ds->buffer, ds->readMode);
		}
		break;

	case DS18b20_Scratchpad_Reading:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->state = DS18b20_State_Finished;
			ds->substate.scratchpadState = DS18b20_Scratchpad_Begin;

			if (ds->onOperationFinished)
				ds->onOperationFinished(ds, DS18b20_State_ReadScratchpad, ds->currentAddress, DS18B20_Callback_Normal);

			ds->currentAddress = DS18B20_ROM_NONE;
		}
		break;
	}
}

// WRITE SCRATCHPAD

static void processWriteScratchpad(DS18B20 *ds)
{
	switch(ds->substate.scratchpadState)
	{
	case DS18b20_Scratchpad_Reading:
		// Ignore
		break;

	case DS18b20_Scratchpad_Begin:
		onewireStart(ds->oneWire);
		ds->substate.scratchpadState = DS18b20_Scratchpad_Start;
		break;

	case DS18b20_Scratchpad_Start:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->substate.scratchpadState = DS18b20_Scratchpad_Write;
			ds->oneWire->startTimer(ds->oneWire);
		}
		break;

	case DS18b20_Scratchpad_Write:
		if (ds_timerPassed(ds, 1000))
		{
			onewireWrite(ds->oneWire, ds->buffer, ds->datalen);
			ds->substate.scratchpadState = DS18b20_Scratchpad_Processing;
		}
		break;

	case DS18b20_Scratchpad_Processing:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->state = DS18b20_State_Finished;
			ds->substate.scratchpadState = DS18b20_Scratchpad_Begin;

			if (ds->onOperationFinished)
				ds->onOperationFinished(ds, DS18b20_State_WriteScratchpad, DS18B20_ROM_NONE, DS18B20_Callback_Normal);

			ds->currentAddress = DS18B20_ROM_NONE;
		}
		break;
	}
}

// READ ROM

static void processReadROM(DS18B20 *ds)
{
	switch(ds->substate.readRomState)
	{
	case DS18b20_ReadROM_Start:
		{
			OneWire_Result owres = onewireProcess(ds->oneWire);
			if (owres == OneWire_NothingToDo)
				onewireStart(ds->oneWire);
			else if (owres == OneWire_Success)
			{
				ds->substate.readRomState = DS18b20_ReadROM_Writing;
				ds->oneWire->startTimer(ds->oneWire);
			}
		}
		break;

	case DS18b20_ReadROM_Writing:
		if (ds_timerPassed(ds, 1000))
		{
			ds18b20write(ds, (DS18B20_Byte[1]){ DS18B20_READ_ROM }, 1);
			ds->substate.readRomState = DS18b20_ReadROM_Process;
		}
		break;

	case DS18b20_ReadROM_Process:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->substate.readRomState = DS18b20_ReadROM_Reading;

			clearbuffer(ds);
			onewireRead(ds->oneWire, ds->buffer, sizeof(DS18B20_Address));
		}
		break;

	case DS18b20_ReadROM_Reading:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->state = DS18b20_State_Finished;
			ds->substate.readRomState = DS18b20_ReadROM_Start;

			if (ds->onOperationFinished)
				ds->onOperationFinished(ds, DS18b20_State_ReadRom, DS18B20_ROM_NONE, DS18B20_Callback_Normal);

			ds->currentAddress = DS18B20_ROM_NONE;
		}
		break;
	}
}

// COPY SCRATCHPAD

static void processCopyScratchpad(DS18B20 *ds)
{
	switch(ds->substate.copyScratchpadState)
	{
	case DS18b20_CopyScratchpad_Start:
		{
			OneWire_Result owres = onewireProcess(ds->oneWire);
			if (owres == OneWire_NothingToDo)
				onewireStart(ds->oneWire);
			else if (owres == OneWire_Success)
			{
				ds->substate.copyScratchpadState = DS18b20_CopyScratchpad_Writing;
				ds->oneWire->startTimer(ds->oneWire);
			}
		}
		break;

	case DS18b20_CopyScratchpad_Writing:
		if (ds_timerPassed(ds, 1000))
		{
			ds18b20writeCommand(ds, DS18B20_COPY_SCRATCHPAD, ds->currentAddress, 0, 0);
			ds->substate.copyScratchpadState = DS18b20_CopyScratchpad_Process;
		}
		break;

	case DS18b20_CopyScratchpad_Process:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->substate.copyScratchpadState = DS18b20_CopyScratchpad_Wait;
			ds->oneWire->startTimer(ds->oneWire);
		}
		break;

	case DS18b20_CopyScratchpad_Wait:
		if (ds_timerPassed(ds, 20000))
		{
			ds->state = DS18b20_State_Finished;
			ds->substate.copyScratchpadState = DS18b20_CopyScratchpad_Start;

			if (ds->onOperationFinished)
				ds->onOperationFinished(ds, DS18b20_State_CopyScratchpad, ds->currentAddress, DS18B20_Callback_Normal);

			ds->currentAddress = DS18B20_ROM_NONE;
		}
		break;
	}
}

// RECALL EEPROM

static void processRecallEeprom(DS18B20 *ds)
{
	switch(ds->substate.recallEepromState)
	{
	case DS18b20_RecallEeprom_Start:
		{
			OneWire_Result owres = onewireProcess(ds->oneWire);
			if (owres == OneWire_NothingToDo)
				onewireStart(ds->oneWire);
			else if (owres == OneWire_Success)
			{
				ds->substate.recallEepromState = DS18b20_RecallEeprom_Writing;
				ds->oneWire->startTimer(ds->oneWire);
			}
		}
		break;

	case DS18b20_RecallEeprom_Writing:
		if (ds_timerPassed(ds, 1000))
		{
			ds18b20writeCommand(ds, DS18B20_RECALL_EEPROM, ds->currentAddress, 0, 0);
			ds->substate.recallEepromState = DS18b20_RecallEeprom_Process;
			ds->temp = 0;
		}
		break;

	case DS18b20_RecallEeprom_Process:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			if (ds->temp)
			{
				ds->state = DS18b20_State_Finished;
				ds->substate.recallEepromState = DS18b20_RecallEeprom_Start;

				if (ds->onOperationFinished)
					ds->onOperationFinished(ds, DS18b20_State_RecallEeprom, ds->currentAddress, DS18B20_Callback_Normal);

				ds->currentAddress = DS18B20_ROM_NONE;
			}
			else
				onewireRead(ds->oneWire, &ds->temp, sizeof(ds->temp));
		}
		break;
	}
}

// READ POWER SUPPLY

static void processReadPowerSupply(DS18B20 *ds)
{
	switch(ds->substate.powerSupplyState)
	{
	case DS18b20_ReadPowerSupply_Start:
		{
			OneWire_Result owres = onewireProcess(ds->oneWire);
			if (owres == OneWire_NothingToDo)
				onewireStart(ds->oneWire);
			else if (owres == OneWire_Success)
			{
				ds->substate.powerSupplyState = DS18b20_ReadPowerSupply_Writing;
				ds->oneWire->startTimer(ds->oneWire);
			}
		}
		break;

	case DS18b20_ReadPowerSupply_Writing:
		if (ds_timerPassed(ds, 1000))
		{
			ds18b20writeCommand(ds, DS18B20_RECALL_EEPROM, ds->currentAddress, 0, 0);
			ds->substate.powerSupplyState = DS18b20_ReadPowerSupply_Process;
			ds->temp = 0;
		}
		break;

	case DS18b20_ReadPowerSupply_Process:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			onewireRead(ds->oneWire, &ds->temp, sizeof(ds->temp));
			ds->substate.powerSupplyState = DS18b20_ReadPowerSupply_Read;
		}
		break;

	case DS18b20_ReadPowerSupply_Read:
		if (onewireProcess(ds->oneWire) == OneWire_Success)
		{
			ds->state = DS18b20_State_Finished;
			ds->substate.powerSupplyState = DS18b20_ReadPowerSupply_Start;

			if (ds->onOperationFinished)
				ds->onOperationFinished(ds, DS18b20_State_ReadPowersupply, ds->currentAddress, ds->temp == 0 ? DS18B20_Callback_Parasitic : DS18B20_Callback_NoParasitic);

			ds->currentAddress = DS18B20_ROM_NONE;
		}
		break;
	}
}

// Functions

void ds18b20Init(DS18B20 *ds, OneWire *ow)
{
	ds->oneWire = ow;
	ds->resolution = DS18B20_Resolution_12;
}

DS18b20State ds18b20Process(DS18B20 *ds)
{
	switch(ds->state)
	{
	case DS18b20_State_Finished:
	case DS18b20_State_Idle:
		// Ignore
		break;

	case DS18b20_State_Convert:
		processConvert(ds);
		break;

	case DS18b20_State_ReadScratchpad:
		processReadScratchpad(ds);
		break;

	case DS18b20_State_WriteScratchpad:
		processWriteScratchpad(ds);
		break;

	case DS18b20_State_ReadRom:
		processReadROM(ds);
		break;

	case DS18b20_State_CopyScratchpad:
		processCopyScratchpad(ds);
		break;

	case DS18b20_State_RecallEeprom:
		processRecallEeprom(ds);
		break;

	case DS18b20_State_ReadPowersupply:
		processReadPowerSupply(ds);
		break;
	}

	return ds->state;
}

DS18B20Result ds18b20BeginConversion(DS18B20 *ds, DS18B20_Address address)
{
	if (ds->state == DS18b20_State_Idle || ds->state == DS18b20_State_Finished)
	{
		ds->currentAddress = address;
		ds->state = DS18b20_State_Convert;
		return DS18B20_Result_Ok;
	}

	return DS18B20_Result_Busy;
}

DS18B20Result ds18b20ReadScratchpad(DS18B20 *ds, DS18B20_Address address)
{
	if (ds->state == DS18b20_State_Idle || ds->state == DS18b20_State_Finished)
	{
		ds->currentAddress = address;
		ds->state = DS18b20_State_ReadScratchpad;
		return DS18B20_Result_Ok;
	}

	return DS18B20_Result_Busy;
}

DS18B20Result ds18b20RequestReadRom(DS18B20 *ds)
{
	if (ds->state == DS18b20_State_Idle || ds->state == DS18b20_State_Finished)
	{
		ds->state = DS18b20_State_ReadRom;
		return DS18B20_Result_Ok;
	}

	return DS18B20_Result_Busy;
}

DS18B20Result ds18b20WriteScratchpad(DS18B20 *ds, DS18B20_Byte *bytes, DS18B20_Size count, DS18B20_Address address)
{
	if (ds->state == DS18b20_State_Idle || ds->state == DS18b20_State_Finished)
	{
		ds->currentAddress = address;
		ds->datalen = ds18b20prepareBuffer(ds, DS18B20_WRITE_SCRATCHPAD, address, count, bytes);

		ds->state = DS18b20_State_WriteScratchpad;
		return DS18B20_Result_Ok;
	}

	return DS18B20_Result_Busy;
}

DS18B20Result ds18b20SetResolution(DS18B20 *ds, DS18B20Resolution resolution, DS18B20_Byte *userbytes, DS18B20_Address address)
{
	if (ds->state == DS18b20_State_Idle || ds->state == DS18b20_State_Finished)
	{
		ds->resolution = resolution;
		DS18B20_Byte buf[3] = { userbytes[0], userbytes[1], resolution };
		ds18b20WriteScratchpad(ds, buf, sizeof(buf), address);
		return DS18B20_Result_Ok;
	}

	return DS18B20_Result_Busy;
}

DS18B20Result ds18b20CopyScratchpad(DS18B20 *ds, DS18B20_Address address)
{
	if (ds->state == DS18b20_State_Idle || ds->state == DS18b20_State_Finished)
	{
		ds->state = DS18b20_State_CopyScratchpad;
		return DS18B20_Result_Ok;
	}

	return DS18B20_Result_Busy;
}

DS18B20Result ds18b20RecallEeprom(DS18B20 *ds, DS18B20_Address address)
{
	if (ds->state == DS18b20_State_Idle || ds->state == DS18b20_State_Finished)
	{
		ds->state = DS18b20_State_RecallEeprom;
		return DS18B20_Result_Ok;
	}

	return DS18B20_Result_Busy;
}

DS18B20Result ds18b20ReadPowerSupply(DS18B20 *ds)
{
	if (ds->state == DS18b20_State_Idle || ds->state == DS18b20_State_Finished)
	{
		ds->state = DS18b20_State_ReadPowersupply;
		return DS18B20_Result_Ok;
	}

	return DS18B20_Result_Busy;
}

//DS18B20Result ds18b20Search(DS18B20 *ds);

//DS18B20Result ds18b20AlarmSearch(DS18B20 *ds);
