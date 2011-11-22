/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * EDID - display identification structs.
 *
 * See http://en.wikipedia.org/wiki/Extended_display_identification_data
 *
**/


#if !defined(__EDID_H__)
#define __EDID_H__

#include <sys/types.h>

struct CompleteSerialNumber {
    u_int8_t   manufacturerID[2];
    u_int8_t   productIDCode[2];
    u_int8_t   serialNumber[4];
    u_int8_t   weekOfManufacture;
    u_int8_t   yearOfManufacture;
};

struct BasicDisplayParameters {
    u_int8_t   videoInputDefinition;
    u_int8_t   maxHImageSize;
    u_int8_t   maxVImageSize;
    u_int8_t   gamma;
    u_int8_t   powerMgmtAndFeatures;
};

struct ChromaInfo {
    u_int8_t                                   info[10];
};

struct StandardTiming {
    u_int8_t                                   horizontalRes;
    u_int8_t                                   verticalRes;
};

struct DescriptorBlock1 {
    u_int16_t                                  pixelClock;
    u_int8_t                                   reserved0;
    u_int8_t                                   blockType;
    u_int8_t                                   reserved1;
    u_int8_t                                   info[13];
};

struct DescriptorBlock2 {
    u_int8_t                                   info[18];
};

struct DescriptorBlock3 {
    u_int8_t                                   info[18];
};

struct DescriptorBlock4 {
    u_int8_t                                   info[18];
};

struct EDID {
    u_int8_t                                   header[8];
    struct CompleteSerialNumber                       serialNumber;
    u_int8_t                                   EDIDVersionNumber;
    u_int8_t                                   EDIDRevisionNumber;
    struct BasicDisplayParameters                     displayParams;
    struct ChromaInfo                                 chromaInfo;
    u_int8_t                                   establishedTiming1;
    u_int8_t                                   establishedTiming2;
    u_int8_t                                   manufacturerReservedTiming;
    struct StandardTiming                             standardTimings[8];
    struct DescriptorBlock1                descriptorBlock1;
    struct DescriptorBlock2                descriptorBlock2;
    struct DescriptorBlock3                descriptorBlock3;
    struct DescriptorBlock4                descriptorBlock4;
    u_int8_t                                       extension;                                                                                      // 0 in EDID 1.1
    u_int8_t                                       checksum;
};

#endif
