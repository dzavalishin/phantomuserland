#ifndef ES1370_H
#define ES1370_H

#include <sys/types.h>

// Registers
//#define ControlRegister           0x00 // Interrupt/Chip Select Control Register
#define ES1370_CONTROL            0x00
//#define StatusRegister            0x04 // Interrupt/Chip Select Status Register
#define ES1370_IRQ_STATUS	  0x04 // Interrupt/Chip Select Status Register
#define UartDataRegister          0x08
#define UartStatusRegister        0x09
#define UartControlRegister       0x09
#define UartReservedRegister      0x0a
#define MemoryPageRegister        0x0c // Memory Page Register
#define CodecWriteRegister        0x10
//#define SerialControlRegister     0x20 // Serial Interface Control Register
#define ES1370_SERIAL_CONTROL     0x20
#define Dac1SampleCountRegister   0x24
#define Dac2SampleCountRegister   0x28
#define AdcSampleCountRegister    0x2c
#define Dac1PciAddressRegister    0x30
#define Dac1BufferSizeRegister    0x34
#define Dac2PciAddressRegister    0x38
#define Dac2BufferSizeRegister    0x3c
#define AdcPciAddressRegister     0x30
#define AdcBufferSizeRegister     0x34

// Interrupt/chip celect control register
#define ControlAdcStop           (1<<31)
#define ControlXctl1             (1<<30)
#define ControlOpen              (1<<30)    // not used.
#define ControlPclkdiv           0x1fff0000 // mask
#define ControlPclkdivShift      16
#define ControlMsfmtsel          (1<<15)
#define ControlMsbb              (1<<14)
#define ControlWtsrsel           0x00003000 // mask
#define ControlWtsrselShift      12
#define ControlDacSync           (1<<11)
#define ControlCcbIntrm          (1<<10)
#define ControlMcb               (1<<9)
#define ControlXctl0             (1<<8)
#define ControlBreq              (1<<7)
#define ControlDac1En            (1<<6)
#define ControlDac2En            (1<<5)
#define ControlAdcEn             (1<<4)
#define ControlUartEn            (1<<3)
#define ControlJystkEn           (1<<2)
#define ControlCdcEn             (1<<1)
#define ControlSerrDis           (1<<0)

// Interrupt/Chip Select Status Register
#define StatusIntr               (1<<31)
#define StatusCstat              (1<<10)
#define StatusCbusy              (1<<9)
#define StatusCwrip              (1<<8)
#define StatusVc                 0x00000060 // mask
#define StatusVcShift            5
#define StatusMccb               (1<<4)
#define StatusUart               (1<<3)
#define StatusDac1               (1<<2)
#define StatusDac2               (1<<1)
#define StatusAdc                (1<<0)

// Serial Interface Control Register
#define SerialP2endinc           0x00380000 // mask
#define SerialP2endincShift      19
#define SerialP2stinc            0x00070000 // mask
#define SerialP2stincShift       16
#define SerialR1loopsel          (1<<15)
#define SerialP2loopsel          (1<<14)
#define SerialP1loopsel          (1<<13)
#define SerialP2pause            (1<<12)
#define SerialP1pause            (1<<11)
#define SerialR1inten            (1<<10)
#define SerialP2inten            (1<<9)
#define SerialP1inten            (1<<8)
#define SerialP1sctrld           (1<<7)
#define SerialP2dacsen           (1<<6)
#define SerialR1seb              (1<<5)
#define SerialR1smb              (1<<4)
#define SerialP2seb              (1<<3)
#define SerialP2smb              (1<<2)
#define SerialP1seb              (1<<1)
#define SerialP1smb              (1<<0)

// Memory page register
#define DacFrameInformation      0x0c // These bits are set to memory page register
#define AdcFrameInformation      0x0d // to select what memory page will be accessed.
#define UartFifo1                0x0e
#define UartFifo2                0x0f

// CODEC register addresses
#define CodecVolMasterL           0x00
#define CodecVolMasterR           0x01
#define CodecVolVoiceL            0x02
#define CodecVolVoiceR            0x03
#define CodecVolFmL               0x04
#define CodecVolFmR               0x05
#define CodecVolCdL               0x06
#define CodecVolCdR               0x07
#define CodecVolLineL             0x08
#define CodecVolLineR             0x09
#define CodecVolAuxL              0x0A
#define CodecVolAuxR              0x0B
#define CodecVolMono1             0x0C
#define CodecVolMono2             0x0D
#define CodecVolMic               0x0E
#define CodecVolMonoOut           0x0F
#define CodecOmix1                0x10
#define CodecOmix2                0x11
#define CodecLimix1               0x12
#define CodecRimix1               0x13
#define CodecLimix2               0x14
#define CodecRimix2               0x15
#define CodecResPd                0x16
#define CodecCsel                 0x17
#define CodecAdsel                0x18
#define CodecMgain                0x19

// CODEC control bits
#define VolMute                   (1<<7)
#define VolAtt4                   (1<<4) // attenuation level
#define VolAtt3                   (1<<3) // 32 levels with 2dB step
#define VolAtt2                   (1<<2) // 00000:   0dB
#define VolAtt1                   (1<<1) // 11111: -62dB
#define VolAtt0                   (1<<0)

#define VolGai4                   (1<<4) // gain level
#define VolGai3                   (1<<3) // 32 levels with 2dB step
#define VolGai2                   (1<<2) // 00000: +12dB
#define VolGai1                   (1<<1) // 00110:   0dB
#define VolGai0                   (1<<0) // 11111: -50dB

#define Omix1FmL                  (1<<6) // ON/OFF of Mixer switches.
#define Omix1FmR                  (1<<5) // 0: OFF, 1: ON.
#define Omix1LineL                (1<<4)
#define Omix1LineR                (1<<3)
#define Omix1CdL                  (1<<2)
#define Omix1CdR                  (1<<1)
#define Omix1Mic                  (1<<0)

#define Omix2AuxL                 (1<<5)
#define Omix2AuxR                 (1<<4)
#define Omix2VoiceL               (1<<3)
#define Omix2VoiceR               (1<<2)
#define Omix2Mono2                (1<<1)
#define Omix2Mono1                (1<<0)

#define Imix1FmL                  (1<<6)
#define Imix1FmR                  (1<<5)
#define Imix1LineL                (1<<4)
#define Imix1LineR                (1<<3)
#define Imix1CdL                  (1<<2)
#define Imix1CdR                  (1<<1)
#define Imix1Mic                  (1<<0)

#define Imix2Tmic                 (1<<7)
#define Imix2Tmono1               (1<<6)
#define Imix2Tmono2               (1<<5)
#define Imix2AuxL                 (1<<4)
#define Imix2AuxR                 (1<<3)
#define Imix2Voice                (1<<2)
#define Imix2Mono2                (1<<1)
#define Imix2Mono1                (1<<0)

#define RespdPd                   (1<<1) // Enables the power down. (1: Normal Operation, 0: Power down)
#define RespdRst                  (1<<0) // initializes the contents of all registers. (1: Normal Operation, 0: Initialize)

#define Csel2                     (1<<1) // Select the clocks for codec.
#define Csel1                     (1<<0)

#define AdselAdcSource            (1<<0) // Selects the input source to ADC.
                                                      // (0: output from input mixer, 1: AINL/AINR inputs)
#define MgainAmpOn                (1<<0) // Selects the gain of MIC amp. (0: 0dB, 1: 30dB)

// alias
#define DefaultAttLevel           0      // default attenuation level (0dB).
#define DefaultGainLevel          (VolGai2 | VolGai1) // 0dB.
#define MixerAllOff               0
#define RespdNormalOperation      (RespdPd | RespdRst)

#define ES1370_BUFSIZE 		(4096*2)


#define ES1370_CBUF 0

#if ES1370_CBUF
#  include <newos/cbuf.h>
#  include <kernel/dpc.h>
#endif

#define ES1370_WTTY 1

#if ES1370_WTTY
#  include <wtty.h>
#  include <kernel/dpc.h>
#endif

typedef struct es1370_private
{
    int         adc_active;
    int         dac_active;

    //int		adc_bitsPerSample;
    //int		dac_bitsPerSample;

    int		oddDac2;
    int         oddAdc;

    physaddr_t	dac2BufferPa;
    physaddr_t	adcBufferPa;

    void *      dac2Buffer;
    void *      adcBuffer;

    size_t      dac2BufferSize;
    size_t      adcBufferSize;

    int         samplerate;
    int         nchannels;
    int         nbits;
    int         silence;

    int		writtenSamples; // dac2 - count of samples we actually sent

    hal_sem_t   w_sem;
#if ES1370_CBUF
    cbuf *      w_cbuf;
    hal_spinlock_t w_lock;

    off_t       w_write_pos;
    off_t       w_read_pos;

    dpc_request w_dpc;
#elif ES1370_WTTY
    wtty_t      *rdq;
    wtty_t      *wrq;
    dpc_request w_dpc;
#else
    const void *w_buf;
    size_t      w_len;
#endif
    int         volume;


} es1370_t;













#endif // ES1370_H
