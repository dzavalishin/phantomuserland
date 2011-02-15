#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "ata.h"
#include "atapi.h"
#include "iso9660.h"

#define ATAPI_ALIGN_BUFFER(x)   if(x % ATAPI_SECTOR_SIZE) { x = x + (ATAPI_SECTOR_SIZE - (x % ATAPI_SECTOR_SIZE)); }

// We assume that the ISO9660 FS we're reading from is on an ATAPI device (CDROM)
FILE *ISO9660_ReadFile(char *device, const char *restrict filepath)
{
    Int64 i;
    Byte adapter, drive;
    char *tokenizer;

    for(adapter = 0; adapter < 2; adapter++)
    {
        int sc = -1;
        
        for(drive = 0; drive < 2; drive++)
        {
            sc = strcmp(ATA_Disk[adapter][drive].Name, device);
            if(!sc) break;
        }
        
        if(!sc) break;
    }
     
    if(adapter >= 2 && drive >= 2)
        return (FILE *)(0);
    
    FILE *f = (FILE *)malloc(sizeof(struct FILE_s));
    f->Name = (char *)malloc(strlen(filepath) + 16);
    strcpy(f->Name, filepath);
    
    if(!(tokenizer = strtok((char *)filepath, "/")))
    {
        free(f->Name);
        free(f);
        return (FILE *)(0);
    }
          
    struct ISO9660_PVD_s *PVD = (struct ISO9660_PVD_s *)ATAPI_ReadSector(adapter, drive, 16);
    struct ISO9660_DirectoryEntry_s *RDE = (struct ISO9660_DirectoryEntry_s *)PVD->RootDirectoryEntry;
    
    // take what we need, store it locally, and free the buffer
    Int32 RootDirLBA = RDE->LBA[0];
    Int32 DIR_DataLengthAligned = RDE->DataLength[0];
    ATAPI_ALIGN_BUFFER(DIR_DataLengthAligned);
    free(RDE);
      
    struct ISO9660_DirectoryEntry_s *DIRBUFFER = (struct ISO9660_DirectoryEntry_s *)malloc(DIR_DataLengthAligned);
    struct ISO9660_DirectoryEntry_s *DIR = DIRBUFFER;
    
    for(i = 0; i < (DIR_DataLengthAligned / ATAPI_SECTOR_SIZE); i++)
    {
        memcpy((Byte *)((IntPtr)DIRBUFFER + (i * ATAPI_SECTOR_SIZE)), ATAPI_ReadSector(adapter, drive, RootDirLBA + i), ATAPI_SECTOR_SIZE);
    }
   
    i = 0;
    Boolean NM_FLAG = false;
    //printf("searching for \"%s\"\n", tokenizer);
    while(i < DIR_DataLengthAligned)
    {        
        while(!DIR->Length)
        {
            DIR = (struct ISO9660_DirectoryEntry_s *)((IntPtr)DIR + 1);
            i++;
        }

        if(i >= DIR_DataLengthAligned)
            break;

        //printf("DIR->Length = %x\n", DIR->Length);
        //printf("DIR->DataLength = %x\n", DIR->DataLength[0]);
        //printf("DIR->LBA = %x\n", DIR->LBA[0]);
        //printf("DIR->FileFlags = %x\n", DIR->FileFlags);
        //printf("DIR->LengthOfFileIdentifier = %u\n", DIR->LengthOfFileIdentifier);
        
        Int64 x, y;
        for(x = 32 + DIR->LengthOfFileIdentifier; x < DIR->Length - 1; x++)
        {
            if(*(Byte *)((IntPtr)DIR + x) == 'N' && *(Byte *)((IntPtr)DIR + x + 1) == 'M')
            {
                struct ISO9660_RR_NM_Entry_s *NM = (struct ISO9660_RR_NM_Entry_s *)((IntPtr)DIR + x);
                
                //printf("NM->Length = %x\n", NM->Length);
                //printf("NM->Version = %x\n", NM->Version);
                //printf("NM->Name = \"%s\"\n", NM->Name);
                
                if(!memcmp(tokenizer, NM->Name, MAX(strlen(tokenizer), NM->Length - 5)))
                {
                    Int32 DIR_LBA;
                
                    //printf("\"%s\" found\n", tokenizer);
                    if(!(tokenizer = strtok(NULL, "/")))
                    {
                        //printf("We've found the full file!\n");
                        //printf("filesize = %ubytes\n", DIR->DataLength[0]);
                        
                        DIR_DataLengthAligned = DIR->DataLength[0];
                        f->Size = DIR->DataLength[0];
                        ATAPI_ALIGN_BUFFER(DIR_DataLengthAligned);
                            
                        DIR_LBA = DIR->LBA[0];
                        free(DIRBUFFER);
                        DIRBUFFER = (struct ISO9660_DirectoryEntry_s *)malloc(DIR_DataLengthAligned);
                        for(y = 0; y < (DIR_DataLengthAligned / ATAPI_SECTOR_SIZE); y++)
                        {
                            memcpy((Byte *)((IntPtr)DIRBUFFER + (y * ATAPI_SECTOR_SIZE)), ATAPI_ReadSector(adapter, drive, DIR_LBA + y), ATAPI_SECTOR_SIZE);
                        }
                        
                        f->Buffer = (Byte *)DIRBUFFER;
                        
                        //printf("f->Size = %u\n", f->Size);
                        return f;
                    }
                    
                    //printf("searching for \"%s\"\n", tokenizer); 
                    DIR_DataLengthAligned = DIR->DataLength[0];
                    ATAPI_ALIGN_BUFFER(DIR_DataLengthAligned);
                        
                    DIR_LBA = DIR->LBA[0];
                    free(DIRBUFFER);
                    DIRBUFFER = (struct ISO9660_DirectoryEntry_s *)malloc(DIR_DataLengthAligned);
                    for(y = 0; y < (DIR_DataLengthAligned / ATAPI_SECTOR_SIZE); y++)
                    {
                        memcpy((Byte *)((IntPtr)DIRBUFFER + (y * ATAPI_SECTOR_SIZE)), ATAPI_ReadSector(adapter, drive, DIR_LBA + y), ATAPI_SECTOR_SIZE);
                    }
                    
                    DIR = DIRBUFFER;
                    NM_FLAG = true;
                }
                
                break;
            }
        }
        
        //printf("\n");
        
        if(!NM_FLAG)
        {
            i += DIR->Length;
            DIR = (struct ISO9660_DirectoryEntry_s *)((IntPtr)DIR + DIR->Length);
        }
        else
        {   
            NM_FLAG = false;
            i = 0;
        }
    }

    free(f->Name);
    free(f);
    return (FILE *)(0);
}

