// Axiomtek SBC i2c and smbus control
#ifndef AXSMBUS_H
#define AXSMBUS_H

#include <stdint.h>

typedef enum _AXEC_I2CBUS_OPERATION {
    AxEcI2cbusReadByte,
    AxEcI2cbusWriteByte,
    AxEcI2cbusReadWord,
    AxEcI2cbusWriteWord,
    AxEcI2cbusReadBlock,
    AxEcI2cbusWriteBlock
} AXEC_I2CBUS_OPERATION;

#ifndef UINT8
#define UINT8 uint8_t
#endif

#define DIOBASE 0xFA31

#define SMIOBASE 0xFA60
#define I2C_STATUS (SMIOBASE)
#define I2C_CONTROL (SMIOBASE+1)
#define I2C_COMMAND (SMIOBASE+2)
#define I2C_ADDR (SMIOBASE+3)
#define I2C_DATA0 (SMIOBASE+4)
#define I2C_DATA1 (SMIOBASE+5)
#define I2C_ByteAccess 0x91
#define I2C_WordAccess 0xA1
#define SMB_ByteAccess 0x90
#define SMB_WordAccess 0xA0
#define I2C_BlockAccess 0xC1
#define I2C_BlockNeDa 0xFE

#define I2C_DelayTime 50000

typedef enum{ EFI_SUCCESS = 0, EFI_UNSUPPORTED = -1, EFI_CMDERR = -2}EFI_STATUS;

//EFI_STATUS AxI2cBusExecute( UINT8 SlaveAddress,
// UINT8 Command,
// AXEC_I2CBUS_OPERATION Operation,
// UINT8 *Length,
// UINT8 *Buffer );

int AxDIOInit();

int CfgDIO(uint8_t u8Cfg);
int SetDIO(uint8_t u8Set);

int AxI2cInit();

EFI_STATUS AxI2cBusReadByteWord( UINT8 Addr,
 UINT8 Command,
 UINT8 *Buffer,
 AXEC_I2CBUS_OPERATION Operation );

EFI_STATUS AxSmbBusReadByteWord(UINT8 Addr, UINT8 Command, UINT8 *Buffer, AXEC_I2CBUS_OPERATION Operation );

//EFI_STATUS AxI2cBusWriteByteWord( UINT8 Addr,
// UINT8 Command,
// UINT8 *Buffer,
// AXEC_I2CBUS_OPERATION Operation );

//EFI_STATUS AxI2cBusReadBlock( UINT8 Addr,
// UINT8 Command,
// UINT8 BlockSize,
// UINT8 *Buffer );

//EFI_STATUS AxI2cBusWriteBlock( UINT8 Addr,
// UINT8 Command,
// UINT8 BlockSize,
// UINT8 *Buffer );


#endif // AXSMBUS_H
