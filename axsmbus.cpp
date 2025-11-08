#include "axsmbus.h"
#include <sys/io.h>
#include <unistd.h>

int AxDIOInit()
{
    return ioperm( DIOBASE, 2, 1 );
}

int CfgDIO(uint8_t u8Cfg)
{
    outb( u8Cfg, DIOBASE );
    return 0;
}
int SetDIO(uint8_t u8Set)
{
    outb( u8Set, DIOBASE+1 );
    return 0;
}

int AxI2cInit()
{
    return ioperm( SMIOBASE, 6, 1 );
}

//4.2 Read Byte/Word
EFI_STATUS AxI2cBusReadByteWord(UINT8 Addr, UINT8 Command, UINT8 *Buffer, AXEC_I2CBUS_OPERATION Operation )
{
    int             Counter = 0;
    unsigned char   ucStatus = 0;

    while( Counter < 10 )
    {
        if( ( (ucStatus = inb(I2C_STATUS)) & 0x02 ) != 0x02 ) //Check I2C Busy
        break;
        usleep( 30000 );
        Counter++;
    }
    if( Counter == 10 )
    {
        //std::cout << "Check I2C Busy inb(I2C_STATUS) return: " << (int)ucStatus << std::endl;
        return EFI_UNSUPPORTED;
    }

     outb( Command, I2C_COMMAND );
     outb( Addr+1, I2C_ADDR );
     usleep( I2C_DelayTime );
     if( Operation == AxEcI2cbusReadByte )
        outb( I2C_ByteAccess, I2C_CONTROL );
     else if( Operation == AxEcI2cbusReadWord )
        outb( I2C_WordAccess, I2C_CONTROL );

     usleep( I2C_DelayTime );

    if( ( (ucStatus = inb(I2C_STATUS)) & 0x04) == 0x04 ) //Command Error
    {
        outb( 0x01, I2C_STATUS );
        //std::cout << "Command error inb(I2C_STATUS) return: " << (int)ucStatus << std::endl;
        return EFI_UNSUPPORTED;
    }

     *Buffer = (UINT8)inb(I2C_DATA0);
     if( Operation == AxEcI2cbusReadWord )
     *(Buffer+1) = inb( I2C_DATA1 );

    return EFI_SUCCESS;
}

EFI_STATUS AxSmbBusReadByteWord(UINT8 Addr, UINT8 Command, UINT8 *Buffer, AXEC_I2CBUS_OPERATION Operation )
{
    int             Counter = 0;
    unsigned char   ucStatus = 0;

    while( Counter < 10 )
    {
        if( ( (ucStatus = inb(I2C_STATUS)) & 0x02 ) != 0x02 ) //Check I2C Busy
        break;
        usleep( 30000 );
        Counter++;
    }
    if( Counter == 10 )
    {
        //std::cout << "Check I2C Busy inb(I2C_STATUS) return: " << (int)ucStatus << std::endl;
        return EFI_UNSUPPORTED;
    }

     outb( Command, I2C_COMMAND );
     outb( Addr+1, I2C_ADDR ); // +1 - read
     usleep( I2C_DelayTime );
     if( Operation == AxEcI2cbusReadByte )
        outb( SMB_ByteAccess, I2C_CONTROL );
     else if( Operation == AxEcI2cbusReadWord )
        outb( SMB_WordAccess, I2C_CONTROL );

     usleep( I2C_DelayTime );

    if( ( (ucStatus = inb(I2C_STATUS)) & 0x04) == 0x04 ) //Command Error
    {
        outb( 0x01, I2C_STATUS );
        //std::cout << "Command error inb(I2C_STATUS) return: " << (int)ucStatus << std::endl;
        return EFI_CMDERR;
    }

     *Buffer = (UINT8)inb(I2C_DATA0);
     if( Operation == AxEcI2cbusReadWord )
        *(Buffer+1) = inb( I2C_DATA1 );

    return EFI_SUCCESS;
}

//4.3 Write Byte/Word
// EFI_STATUS AxI2cBusWriteByteWord( UINT8 Addr, UINT8 Command, UINT8 *Buffer, AXEC_I2CBUS_OPERATION Operation )
//{
//    int Counter = 0;
//    while( Counter < 10 ){
//        if( ( inb(I2C_STATUS) & 0x02 ) != 0x02 ) //Check I2C Busy
//        break;
//        //gBS -> Stall(30000);
//        usleep( 30000 );
//        Counter++;
//    }

//    if( Counter == 10 )
//        return EFI_UNSUPPORTED;
//    //IoWrite8( I2C_COMMAND, Command );
//    outb( Command, I2C_COMMAND );
//    //IoWrite8( I2C_ADDR, Addr );
//    outb( Addr, I2C_ADDR );
//    //IoWrite8( I2C_DATA0, *Buffer );
//    outb( *Buffer, I2C_DATA0 );
//    //gBS -> Stall(I2C_DelayTime);
//    usleep(I2C_DelayTime);

//    if( Operation == AxEcI2cbusWriteByte )
//        //IoWrite8( I2C_CONTROL, I2C_byteAccess );
//        outb( I2C_ByteAccess, I2C_CONTROL );
//    else
//    if( Operation == AxEcI2cbusWriteWord ){
//        //IoWrite8( I2C_DATA1, *(Buffer+1) );
//        outb( *(Buffer+1), I2C_DATA1 );
//        //IoWrite8( I2C_CONTROL, I2C_WordAccess );
//        outb( I2C_WordAccess, I2C_CONTROL );
//    }
//    if( ( inb(I2C_STATUS) & 0x04) == 0x04 ) //Command Error
//     return EFI_UNSUPPORTED;

//    //gBS -> Stall(I2C_DelayTime);
//    usleep( I2C_DelayTime );

//    return EFI_SUCCESS;
//}
//4.4 Read Block
// EFI_STATUS AxI2cBusReadBlock( UINT8 Addr,
// UINT8 Command,
// UINT8 BlockSize,
// UINT8 *Buffer )
//{
// UINT8 COUNT = 0;
//int Counter = 0;
// while( Counter < 10 ){
// if( ( IoRead8(I2C_STATUS) & 0x02 ) != 0x02 ) //Check I2C Busy
// break;
// gBS -> Stall(30000);
// Counter++;
// }
//if( Counter == 10 )
// return EFI_UNSUPPORTED;
// IoWrite8( I2C_STATUS, 0x01 );
//gBS -> Stall(I2C_DelayTime);
// IoWrite8( I2C_COMMAND, Command );
// IoWrite8( I2C_ADDR, Addr+1 );
//IoWrite8( I2C_DATA0, BlockSize );
// IoWrite8( I2C_CONTROL, I2C_BlockAccess );
// gBS -> Stall(I2C_DelayTime);

//if( ( IoRead8(I2C_STATUS) & 0x04) == 0x04 ) //Command Error
// return EFI_UNSUPPORTED;
// while((IoRead8(I2C_STATUS) & 0x08) != 0x08 ){ // Check Last Data
// Buffer[COUNT] = IoRead8(I2C_DATA1);
// COUNT++;
// IoWrite8( I2C_STATUS, I2C_BlockNeDa );
// gBS -> Stall(I2C_DelayTime);
// }
//Buffer[COUNT] = IoRead8(I2C_DATA1);
//return EFI_SUCCESS;
//}

////4.5 Write Block
// EFI_STATUS AxI2cBusWriteBlock( UINT8 Addr,
// UINT8 Command,
// UINT8 BlockSize,
// UINT8 *Buffer )
//{
// UINTN Index;
//int Counter = 0;
//while( Counter < 10 ){
// if( ( IoRead8(I2C_STATUS) & 0x02 ) != 0x02 ) //Check I2C Busy
// break;
// gBS -> Stall(30000);
// Counter++;
// }
//if( Counter == 10 )
// return EFI_UNSUPPORTED;
// IoWrite8( I2C_STATUS, 0x01 );
//gBS -> Stall(I2C_DelayTime);
// IoWrite8( I2C_COMMAND, Command );
// IoWrite8( I2C_ADDR, Addr );
// IoWrite8( I2C_DATA0, BlockSize );
//IoWrite8( I2C_DATA1, Buffer[0] );
// IoWrite8( I2C_CONTROL, I2C_BlockAccess );
// gBS -> Stall(I2C_DelayTime);

// for( Index = 1 ; Index < BlockSize ; Index++ ){
// IoWrite8( I2C_DATA1, Buffer[Index] );
// IoWrite8( I2C_STATUS, I2C_BlockNeDa );
// gBS -> Stall(I2C_DelayTime);
// }
//if( ( IoRead8(I2C_STATUS) & 0x04) == 0x04 ) //Command Error
// return EFI_UNSUPPORTED;
//return EFI_SUCCESS;
//}
