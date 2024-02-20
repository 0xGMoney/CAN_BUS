#ifndef _MCP2515_H_
#define _MCP2515_H_

/******************************************************************************
 * SPI Commands                                                               *
 ******************************************************************************/
#define SPI_RESET       0xC0
#define SPI_READ        0x03
#define SPI_READ_RX     0x90
#define SPI_WRITE       0x02
#define SPI_WRITE_RX    0x40
#define SPI_RTS         0x80
#define SPI_READ_STATUS 0xA0
#define SPI_RX_STATUS   0xB0
#define SPI_BIT_MODIFY  0x05


#endif
