#ifndef ISO9660_H
#define ISO9660_H

#define TypeCode_BootRecord                     0x00
#define TypeCode_PrimaryVolume                  0x01
#define TypeCode_SupplementaryVolume            0x02
#define TypeCode_VolumePartition                0x03
#define TypeCode_VolumeSetTerminator            0xFF

/* ----------- <RockRidge> ------------ */

// PX - POSIX file attributes
struct __attribute__((packed)) ISO9660_RR_PX_Entry_s
{
    Byte    Signature[2];
    Byte    Length;
    Byte    Version;
    Int32   FileMode[2];
    Int32   FileLinks[2];
    Int32   FileUserID[2];
    Int32   FileGroupID[2];
    Int32   FileSerialNumber[2];
};

// PN - POSIX device number
struct __attribute__((packed)) ISO9660_RR_PN_Entry_s
{
    Byte    Signature[2];
    Byte    Length;
    Byte    Version;
    Int32   DeviceNumberHi[2];
    Int32   DeviceNumberLo[2];
};

// SL - Symbolic Link
struct __attribute__((packed)) ISO9660_RR_SL_Entry_s
{
    Byte    Signature[2];
    Byte    Length;
    Byte    Version;
    Byte    Flags;
    Byte    ComponentArea[];
};

// NM - Alternate Name
struct __attribute__((packed)) ISO9660_RR_NM_Entry_s
{
    Byte    Signature[2];
    Byte    Length;
    Byte    Version;
    Byte    Flags;
    char    Name[];
};

// TF - Time stamp(s) for a file
struct __attribute__((packed)) ISO9660_RR_TF_Entry_s
{
    Byte    Signature[2];
    Byte    Length;
    Byte    Version;
    Byte    Flags;
    Byte    Timestamps[];
};

/* ----------- </RockRidge> ----------- */


struct __attribute__((packed)) ISO9660_BootRecord_s
{
    Byte    Type;
    char    ID[5];
    Byte    Version;
    char    BootSystemID[32];
    char    BootID[32];
    Byte    BootSystemUse[1977];
};

struct __attribute__((packed)) ISO9660_DirectoryEntry_s
{
    Byte    Length;
    Byte    ExtAttributeLength;
    Int32   LBA[2];
    Int32   DataLength[2];
    Byte    RecordingDateAndTime[7];
    Byte    FileFlags;
    Byte    FileUnitSize;   // interleaved mode only (zero otherwise)
    Byte    GapSize;        // interleaved mode only (zero otherwise)
    Int16   VolumeSequenceNumber[2];
    Byte    LengthOfFileIdentifier;
    char    FileIdentifier[];
};

struct __attribute__((packed)) ISO9660_PVD_s 
{
    Byte    Type;
    char    ID[5];
    Byte    Version;
    Byte    Unused0;
    char    SystemID[32];
    char    VolumeID[32];
    Byte    Unused1[8];
    Int32   VolumeSpaceSize[2];
    Byte    Unused2[32];
    Int16   VolumeSetSize[2];
    Int16   VolumeSequenceNumber[2];
    Int16   LogicalBlockSize[2];
    Int32   PathTableSize[2];
    Int32   LocationOfTypeLPathTable;
    Int32   LocationOfOptionalTypeLPathTable;
    Int32   LocationOfTypeMPathTable;
    Int32   LocationOfOptionalTypeMPathTable; 
    Byte    RootDirectoryEntry[34];
    char    VolumeSetIdentifier[128];
    char    DataPreparerIdentifier[128];
    char    ApplicationIdentifier[128];
    char    CopyrightFileIdentifier[38];
    char    AbstractFileIdentifier[36];
    char    BibliographicFileIdentifier[37];
    char    VolumeCreationDateAndTime[17];
    char    VolumeModificationDateAndTime[17];
    char    VolumeExpirationDateAndTime[17];
    char    VolumeEffectiveDateAndTime[17];
    Byte    FileStructureVersion;
    Byte    Unused3;
    Byte    ApplicationUsed[512];
    Byte    Reserved[653];
};

FILE *ISO9660_ReadFile(char *device, const char *restrict filepath);

#endif

