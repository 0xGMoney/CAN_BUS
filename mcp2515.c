#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "mcp2515.h"

#define CHIP_SELECT   2
#define INTERRUPT_LOW 2
#define SERIAL_CLK    5
#define MOSI          4
#define MISO          3

#define FILLER 0xFF

/****************************************************************************** 
 * When the MCP2515 receives a valid CAN message, it generates an interrupt   *
 * on the INT pin (which is an active LOW.) The MCP2515 INT pin is the D2 pin *
 * on the Arduino, so to check the state of the D2 pin on the PIN register,   *
 * we shift 0x1 two bits to the left. Again, since INT is an active LOW, if   *
 * D2 is LOW, that signals that the interrupt has been generated and that     *
 * there is a valid CAN message to be read.                                   *
 ******************************************************************************/
uint8_t messageReceived(void) {
    return!(PIND & (1<<INTERRUPT_LOW));
}

/******************************************************************************
 * For SPI, data is transmitted and received using the SPDR register          *
 * (SPI Data Register.) How the data transfer works between the Master and    *
 * Slave is that when the clock is pulsed, data is shifted out of the         *
 * Master's register and into the Slave's register. Yet at the same time,     *
 * data (or a single bit rather) is shifted out of the Slave's register and   *
 * into the Master's register. In summary, on each clock pulse, a bit is      *
 * exchanged in each direction between the Master and the Slave. We then wait *
 * until the SPIF bit is set in the SPSR register (SPI Status Register) to    *
 * indicate that the 8-bit transfer of data is complete.                      *
 ******************************************************************************/
uint8_t sendData(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1<<SPIF))) {;}

    return SPDR;
}

/******************************************************************************
 * First, set the "Chip Select" pin LOW so that the Slave is enabled relative *
 * to the Master. Next, send the "SPI_WRITE" instruction. Next, send the      *
 * memory address to read from. Next, send the data that you want to write to *
 * the specified memory address. Lastly, set the "Chip Select" pin HIGH,      *
 * disabling the Slave.                                                       *
 ******************************************************************************/
void writeToRegister(uint8_t address, uint8_t data) {
    PORTB &= ~(1<<CHIP_SELECT);

    sendData(SPI_WRITE);
    sendData(address);
    sendData(data);

    PORTB |= (1<<CHIP_SELECT);

    return;
}

/******************************************************************************
 * First, set the "Chip Select" pin LOW so that the Slave is enabled relative *
 * to the Master. Next, send the "SPI_READ" instruction. Next, send the       *
 * memory address to read from. Next, send some filler data to the SPDR, and  *
 * the return data from the SPDR should be the data read from the memory      *
 * address. Lastly, set the "Chip Select" pin HIGH, disabling the Slave.      *
 ******************************************************************************/
uint8_t readRegister(uint8_t address) {
    PORTB &= ~(1<<CHIP_SELECT);

    sendData(SPI_READ);
    sendData(address);

    uint8_t data = sendData(FILLER);

    PORTB |= (1<<CHIP_SELECT);

    return data;
}

/******************************************************************************
 * First, set the "Chip Select" pin LOW so that the Slave is enabled relative *
 * to the Master. Next, send the "SPI_BIT_MODIFY" instruction. Next, send the *
 * memory address of the register that you want to modify. Next, send the     *
 * "mask" bitmask, this mask while define which bits in the register are able *
 * to be modified. A value of "1" in the "mask" bit mask means the bit at     *
 * that position can be modified, a "0" signifies that that bit can't be      *
 * modified. Next, send the "data" bit mask, this is where the new values for *
 * the register are set. Lastly, set the "Chip Select" pin HIGH, disabling    *
 * the Slave.                                                                 *
 *                                                                            *
 *                * *   *   *    * = value of "1" in this mask means the      *
 *      MASK: 0 0 1 1 0 1 0 1        bit at that bit position can change      *
 *                                                                            *
 *   & (DATA: 0 0 1 0 0 0 0 1)                                                *
 *   -------------------------                                                *
 * NEW VALUE: 0 0 1 0 0 0 0 1                                                 *
 ******************************************************************************/
void bitModify(uint8_t address, uint8_t mask, uint8_t data) {
    PORTB &= ~(1<<CHIP_SELECT);

    sendData(SPI_BIT_MODIFY);
    sendData(address);
    sendData(mask);
    sendData(data);  
                    

    PORTB |= (1<<CHIP_SELECT);

    return;
}

/******************************************************************************
 * First, set the "Chip Select" pin low so that the Slave is enabled relative *
 * to the Master. Next, one of the "READ_STATUS" or "RX_STATUS" instructions  *
 * will be sent, depending on which one was used as an argument with the      *
 * function call. Filler data is sent to the SPDR, and the data written back  *
 * to the SPDR will be the data returned from the instruction sent earlier.   *
 * Lastly, set the "Chip Select" pin HIGH, disabling the Slave.               *
 ******************************************************************************/
uint8_t readStatus(uint8_t readType) {
    PORTB &= ~(1<<CHIP_SELECT);

    sendData(readType);
    uint8_t data = sendData(FILLER);

    PORTB |= (1<<CHIP_SELECT);

    return data;
}

/******************************************************************************
 * First, set the "Chip Select" pin low so that the Slave is enabled relative *
 * to the Master. Next, send the "SPI_RESET" instruction. MCP2515 data sheet  *
 * recommends performing a Reset after power-up to ensure that the logic and  *
 * registers are in their default state prior to configuration and operations.*
 * Performing a Reset also puts the chip into Configuration Mode.             *
 ******************************************************************************/
void resetController(void) {
    PORTB &= ~(1<<CHIP_SELECT);

    sendData(SPI_RESET);

    PORTB |=  (1<<CHIP_SELECT);

    return;
}

/******************************************************************************
 ******************************************************************************/
uint8_t initController(void) {
    // Pull "Chip Select" pin HIGH (disabling Slave)
    // Configure "Chip Select" pin as an OUTPUT
    PORTB |= (1<<CHIP_SELECT);
    DDRB  |= (1<<CHIP_SELECT);

    // Pull "Serial Clock" pin LOW 
    // Pull "MOSI" (Master Out Slave In) pin LOW 
    // Pull "MISO" (Master In Slave Out) pin LOW
    PORTB &= ~((1<<SERIAL_CLK) | (1<<MOSI) | (1<<MISO));
    
    // Configure "Serial Clock" pin as an OUTPUT
    // Configure "MOSI" pin as an OUTPUT
    // Configure "MISO" pin as in INPUT
    DDRB  |= ((1<<SERIAL_CLK) | (1<<MOSI));
    DDRB  &= ~(1<<MISO);

    // Configure "Interrupt" pin as an INPUT
    // Configure "Interrupt" pin HIGH
    DDRD  &= ~(1<<INTERRUPT_LOW);
    PORTD |=  (1<<INTERRUPT_LOW);

    // Enable SPI
    // Data order: MSB first
    // Set Arduino to Controller Mode
    // Configure serial clock speed
    SPCR = (1<<SPE) | (0<<DORD) | (1<<MSTR) | (0<<SPR1) | (1<<SPR0);
    // Baseline/default Status register
    SPSR = 0;

    // Perform a reset on MCP2515 controller
    resetController();

    // Give controller time for reset to complete
    _delay_us(10);

    // Load Configuration registers
    PORTB &= ~(1<<CHIP_SELECT);
    sendData(SPI_WRITE);
    sendData(CNF3);
    sendData((1<<PHSEG21));
    



    return SPDR;
}

int main() {
    return 0;
}
