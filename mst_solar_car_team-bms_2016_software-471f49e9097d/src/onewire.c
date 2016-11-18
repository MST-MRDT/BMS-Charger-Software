/**
 * @file onewire.c
 *
 * A one-wire serial driver implemented to handle three different communication busses.
 *
 * This driver is based on the implementation available in Maxim's application notes, <a href="https://www.maximintegrated.com/en/app-notes/index.mvp/id/126">1-Wire Communication Through Software</a>,
 * and <a href="https://www.maximintegrated.com/en/app-notes/index.mvp/id/187">1-Wire Search Algorithm</a>
 * It has been primarily modified to reflect the multiple banks used on our implementation.
 *
 * @see {@link ds18b20.c}
 *
 * @author Jesse Cureton
 */

#include <msp430.h>
#include "main.h"
#include "onewire.h"
#include "pins.h"

#include "drivers/ds18b20.h"

/**
 * Write a logic 1 to the specified bus.
 * @param bank One of the {@link BATTERY_BANKS bank IDs}
 */
void owWriteOne(uint8_t bank)
{
	switch(bank)
	{
		case BANK1:
			P4DIR |= BANK1_TEMP;	//Set the pin to output
			P4OUT &= ~BANK1_TEMP;	//Pull the bus low
			delayMicros(DELAY_A);	//Delay for time specified in comm spec
			P4DIR &= ~BANK1_TEMP;	//Set the pin to an input to release it back to bus voltage
			delayMicros(DELAY_B);	//Complete the 60uS time slot for the bit, and give 10uS bus recovery time
			break;
		case BANK2:
			P6DIR |= BANK2_TEMP;	//Set the pin to output
			P6OUT &= ~BANK2_TEMP;	//Pull the bus low
			delayMicros(DELAY_A);	//Delay for time specified in comm spec
			P6DIR &= ~BANK2_TEMP;	//Set the pin to an input to release it back to bus voltage
			delayMicros(DELAY_B);	//Complete the 60uS time slot for the bit, and give 10uS bus recovery time
			break;
		case BANK3:
			P1DIR |= BANK3_TEMP;	//Set the pin to output
			P1OUT &= ~BANK3_TEMP;	//Pull the bus low
			delayMicros(DELAY_A);	//Delay for time specified in comm spec
			P1DIR &= ~BANK3_TEMP;	//Set the pin to an input to release it back to bus voltage
			delayMicros(DELAY_B);	//Complete the 60uS time slot for the bit, and give 10uS bus recovery time
			break;
		default:
			ERRORS |= UNKNOWN_ERROR;	//UNKNOWN FAULT error bit
	}
}

/**
 * Write a logic 0 to the specified bus.
 * @param bank One of the {@link BATTERY_BANKS bank IDs}
 */
void owWriteZero(uint8_t bank)
{
	switch(bank)
	{
		case BANK1:
			P4DIR |= BANK1_TEMP;	//Set the pin to output
			P4OUT &= ~BANK1_TEMP;	//Pull the bus low
			delayMicros(DELAY_C);	//Delay for time specified in comm spec
			P4DIR &= ~BANK1_TEMP;	//Set the pin to an input to release it back to bus voltage
			delayMicros(DELAY_D);	//Complete the 60uS time slot for the bit, and give 10uS bus recovery time
			break;
		case BANK2:
			P6DIR |= BANK2_TEMP;	//Set the pin to output
			P6OUT &= ~BANK2_TEMP;	//Pull the bus low
			delayMicros(DELAY_C);	//Delay for time specified in comm spec
			P6DIR &= ~BANK2_TEMP;	//Set the pin to an input to release it back to bus voltage
			delayMicros(DELAY_D);	//Complete the 60uS time slot for the bit, and give 10uS bus recovery time
			break;
		case BANK3:
			P1DIR |= BANK3_TEMP;	//Set the pin to output
			P1OUT &= ~BANK3_TEMP;	//Pull the bus low
			delayMicros(DELAY_C);	//Delay for time specified in comm spec
			P1DIR &= ~BANK3_TEMP;	//Set the pin to an input to release it back to bus voltage
			delayMicros(DELAY_D);	//Complete the 60uS time slot for the bit, and give 10uS bus recovery time
			break;
		default:
			ERRORS |= UNKNOWN_ERROR;
	}
}

/**
 * Reset the specified bus
 * @param bank One of the {@link BATTERY_BANKS bank IDs}
 */
uint8_t owReset(uint8_t bank)
{
	unsigned char devicePresent = 0;

	switch(bank)
	{
		case BANK1:
			P4DIR |= BANK1_TEMP;	//Set to output
			P4OUT &= ~BANK1_TEMP;	//Drive low
			delayMicros(DELAY_H);	//Delay
			P4DIR &= ~BANK1_TEMP;	//Release to an input
			delayMicros(DELAY_I);	//Delay
			if(!(P4IN & BANK1_TEMP)) devicePresent = 1; else devicePresent = 0;
			delayMicros(DELAY_J);
			break;
		case BANK2:
			P6DIR |= BANK2_TEMP;	//Set to output
			P6OUT &= ~BANK2_TEMP;	//Drive low
			delayMicros(DELAY_H);	//Delay
			P6DIR &= ~BANK2_TEMP;	//Release to an input
			delayMicros(DELAY_I);	//Delay
			if(!(P6IN & BANK2_TEMP)) devicePresent = 1; else devicePresent = 0;
			delayMicros(DELAY_J);
			break;
		case BANK3:
			P1DIR |= BANK3_TEMP;	//Set to output
			P1OUT &= ~BANK3_TEMP;	//Drive low
			delayMicros(DELAY_H);	//Delay
			P1DIR &= ~BANK3_TEMP;	//Release to an input
			delayMicros(DELAY_I);	//Delay
			if(!(P1IN & BANK3_TEMP)) devicePresent = 1; else devicePresent = 0;
			delayMicros(DELAY_J);
			break;
		default:
			ERRORS |= UNKNOWN_ERROR;
	}
	return devicePresent;
}

/**
 * Read in a bit from the specified bus
 * @param bank One of the {@link BATTERY_BANKS bank IDs}
 * @return An {@link uint8_t} with the value read in from the bus in bit 0.
 */
uint8_t owReadBit(uint8_t bank)
{
	unsigned char result = 0;
	switch(bank)
	{
		case BANK1:
			P4DIR |= BANK1_TEMP;	//Set to output
			P4OUT &= ~BANK1_TEMP;	//Drive low
			delayMicros(DELAY_A);	//Delay
			P4DIR &= ~BANK1_TEMP;	//Release to an input
			delayMicros(DELAY_E);	//Delay
			if(P4IN & BANK1_TEMP) result = 1; else result = 0;
			delayMicros(DELAY_F);
			break;
		case BANK2:
			P6DIR |= BANK2_TEMP;	//Set to output
			P6OUT &= ~BANK2_TEMP;	//Drive low
			delayMicros(DELAY_A);	//Delay
			P6DIR &= ~BANK2_TEMP;	//Release to an input
			delayMicros(DELAY_E);	//Delay
			if(P6IN & BANK2_TEMP) result = 1; else result = 0;
			delayMicros(DELAY_F);
			break;
		case BANK3:
			P1DIR |= BANK3_TEMP;	//Set to output
			P1OUT &= ~BANK3_TEMP;	//Drive low
			delayMicros(DELAY_A);	//Delay
			P1DIR &= ~BANK3_TEMP;	//Release to an input
			delayMicros(DELAY_E);	//Delay
			if(P1IN & BANK3_TEMP) result = 1; else result = 0;
			delayMicros(DELAY_F);
			break;
		default:
			ERRORS |= UNKNOWN_ERROR;
	}
	return result;
}

/**
 * Write a byte to the specified bus
 * @param bank One of the {@link BATTERY_BANKS bank IDs}.
 * @param data The data to write to the bus.
 */
void owWriteByte(uint8_t bank, uint8_t data)
{
	//Notice everything below sends data LSB first, as per the 1-wire specification
	for(int i=8; i>0; i--)
	{
		if(data & 0x01) owWriteOne(bank);	//If the LSB of data is a 1, write a 1 to the bus
		else owWriteZero(bank);				//If the LSB isn't a 1, write a zero
		data >>= 1;							//Bit shift data right by one bit
	}
}

/**
 * Read in a byte from the specified bus
 * @param bank One of the {@link BATTERY_BANKS bank IDs}
 * @return The byte read in from the specified bus
 */
uint16_t owReadByte(uint8_t bank)
{
	unsigned int val = 0;

	for(int i=8; i>0; i--)
	{
		val >>= 1;
		if(owReadBit(bank)) val |= 0x80;
	}
	return val;
}

/**
 * Find the "first" device on the specified 1-Wire bus.
 *
 * @param bank One of the {@link BATTERY_BANKS bank IDs}
 * @return 1 if a device is found, and the ROM ID in the {@link curROMAddr} field is valid. 0 if no devices found.
 *
 * @see <a href="https://www.maximintegrated.com/en/app-notes/index.mvp/id/187">1-Wire Search Algorithm</a>
 */
uint8_t owFindFirst(uint8_t bank)
{
	//Reset the search state
	LastDiscrepancy = 0;
	LastDeviceFlag  = FALSE;
	LastFamilyDiscrepancy = 0;

	return owSearch(bank);
}

/**
 * Find the next device on the specified 1-Wire bus.
 *
 * @param bank One of the {@link BATTERY_BANKS bank IDs}
 * @return 1 if a device is found, and the ROM ID in the {@link curROMAddr} field is valid. 0 if no devices found.
 *
 * @see <a href="https://www.maximintegrated.com/en/app-notes/index.mvp/id/187">1-Wire Search Algorithm</a>
 */
uint8_t owFindNext(uint8_t bank)
{
	return owSearch(bank);
}

/**
 * Perform a 1-Wire search on the specified bus.
 *
 * @param bank One of the {@link BATTERY_BANKS bank IDs}
 * @return 1 if a devices is found, and the ROM ID in the {@link curROMAddr} field is valid. 0 if no devices found.
 *
 * @see <a href="https://www.maximintegrated.com/en/app-notes/index.mvp/id/187">1-Wire Search Algorithm</a>
 */
uint8_t owSearch(uint8_t bank)
{
	uint8_t  id_bit_number;			//The ROM bit number 1-64 that is currently being searched
	uint8_t  last_zero;				//The ROM bit position of the last zero written where there was a discrepancy
	uint8_t  search_result;			//Boolean value of whether or not a device was found
	uint8_t  id_bit, cmp_id_bit;	//Current ID bit and it's compliment as read from the bus
	uint8_t  search_direction;		//Bit value indicating the direction of the source. Devices w/ this bit in this position stay on the bus, others drop until reset
	uint64_t addr_bit_mask;			//Bit mask for setting the values in the global curROMAddr

	id_bit_number 	= 1;
	last_zero 		= 0;
	search_result 	= 0;
	crc8 			= 0;
	addr_bit_mask 	= 1;

	if(!LastDeviceFlag) 	//If the last call was not the last one
	{
		if(!owReset(bank))	//Reset the 1-wire bus
		{
			//If no devices are present on the bus something has gone horribly wrong;
			//reset the search and declare a temp fault and an unknown fault
			LastDiscrepancy 	  = 0;
			LastDeviceFlag  	  = FALSE;
			LastFamilyDiscrepancy = 0;
			ERRORS |= COMMS_FAULT;
			return FALSE;
		}

		owWriteByte(bank, SEARCH_ROM);	//Issue a search command

		do	//Loop to do the search
		{
			//Read a bit and its compliment
			id_bit 	   = owReadBit(bank);
			cmp_id_bit = owReadBit(bank);

			if((id_bit == 1) && (cmp_id_bit == 1)) break;
			else
			{
				if(id_bit != cmp_id_bit) 		//All devices still in search have either a 0 or a 1
					search_direction = id_bit;	//Write bit value for search
				else
				{
					if(id_bit_number < LastDiscrepancy) //If this discrepancy is before the last discrepancy on a previous next then pick the same as last time
						search_direction = ((curROMAddr & addr_bit_mask) > 0);
					else //If equal to last discrepancy, pick 1, if not, pick 0
						search_direction = (id_bit_number == LastDiscrepancy);

					if(search_direction == 0) //If 0 was picked record its position in LastZero
					{
						last_zero = id_bit_number;
						if(last_zero < 9)
							LastFamilyDiscrepancy = last_zero;
					}
				}

				//Set or clear the bit in the curROMAddr
				if(search_direction == 1)
					curROMAddr |= addr_bit_mask;
				else
					curROMAddr &= ~addr_bit_mask;

				//Serial number search direction write bit
				if(search_direction == 1) owWriteOne(bank);
				else owWriteZero(bank);

				id_bit_number++;			//Move to the next bit in the address
				addr_bit_mask <<= 1;
			}
		} while(id_bit_number < 65);  		// loop until through all ROM bits 1-64

		if(!(id_bit_number < 65))			//Was the search successful?
		{
			LastDiscrepancy = last_zero;	//Successful so set LastDiscrepancy,LastDeviceFlag,search_result

			if(LastDiscrepancy == 0)		//Check for last device
					LastDeviceFlag = TRUE;

			search_result = TRUE;			//We found a device
		}
	}

	if(!search_result || !curROMAddr)
	{
		LastDiscrepancy 	  = 0;
		LastDeviceFlag 		  = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result		  = FALSE;
	}

	return search_result;
}




