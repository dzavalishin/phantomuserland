/*
 * All tables must be byte-packed to match the ACPI specification, since
 * the tables are provided by the system BIOS.
 */

#define ACPI_TABLE_HEADER_DEF   /* ACPI common table header */ \
        u_int8_t                            signature [4];          /* ACPI signature (4 ASCII characters) */\
        u_int32_t                             length;                 /* Length of table, in bytes, including header */\
        u_int8_t                              revision;               /* ACPI Specification minor version # */\
        u_int8_t                              checksum;               /* To make sum of entire table == 0 */\
        u_int8_t                            oem_id [6];             /* OEM identification */\
        u_int8_t                            oem_table_id [8];       /* OEM table identification */\
        u_int32_t                             oem_revision;           /* OEM revision number */\
        u_int8_t                            asl_compiler_id [4];    /* ASL compiler vendor ID */\
        u_int32_t                             asl_compiler_revision;  /* ASL compiler revision number */


struct acpi_table_header         /* ACPI common table header */
{
        ACPI_TABLE_HEADER_DEF
} __attribute__((__packed__));

struct rsdp_descriptor         /* Root System Descriptor Pointer */
{
        u_int8_t                            signature [8];          /* ACPI signature, contains "RSD PTR " */
        u_int8_t                              checksum;               /* To make sum of struct == 0 */
        u_int8_t                            oem_id [6];             /* OEM identification */
        u_int8_t                              revision;               /* Must be 0 for 1.0, 2 for 2.0 */
        u_int32_t                             rsdt_physical_address;  /* 32-bit physical address of RSDT */
        u_int32_t                             length;                 /* XSDT Length in bytes including hdr */
        u_int64_t                             xsdt_physical_address;  /* 64-bit physical address of XSDT */
        u_int8_t                              extended_checksum;      /* Checksum of entire table */
        u_int8_t                            reserved [3];           /* Reserved field must be 0 */
} __attribute__((__packed__));

/*
 * ACPI 1.0 Root System Description Table (RSDT)
 */
struct rsdt_descriptor_rev1
{
        ACPI_TABLE_HEADER_DEF                           /* ACPI common table header */
#ifdef BX_QEMU
        u_int32_t                             table_offset_entry [4]; /* Array of pointers to other */
#else
        u_int32_t                             table_offset_entry [3]; /* Array of pointers to other */
#endif
                         /* ACPI tables */
} __attribute__((__packed__));

/*
 * ACPI 1.0 Firmware ACPI Control Structure (FACS)
 */
struct facs_descriptor_rev1
{
        u_int8_t                            signature[4];           /* ACPI Signature */
        u_int32_t                             length;                 /* Length of structure, in bytes */
        u_int32_t                             hardware_signature;     /* Hardware configuration signature */
        u_int32_t                             firmware_waking_vector; /* ACPI OS waking vector */
        u_int32_t                             global_lock;            /* Global Lock */
        u_int32_t                             S4bios_f        : 1;    /* Indicates if S4BIOS support is present */
        u_int32_t                             reserved1       : 31;   /* Must be 0 */
        u_int8_t                              resverved3 [40];        /* Reserved - must be zero */
} __attribute__((__packed__));


/*
 * ACPI 1.0 Fixed ACPI Description Table (FADT)
 */
struct fadt_descriptor_rev1
{
        ACPI_TABLE_HEADER_DEF                           /* ACPI common table header */
        u_int32_t                             firmware_ctrl;          /* Physical address of FACS */
        u_int32_t                             dsdt;                   /* Physical address of DSDT */
        u_int8_t                              model;                  /* System Interrupt Model */
        u_int8_t                              reserved1;              /* Reserved */
        u_int16_t                             sci_int;                /* System vector of SCI interrupt */
        u_int32_t                             smi_cmd;                /* Port address of SMI command port */
        u_int8_t                              acpi_enable;            /* Value to write to smi_cmd to enable ACPI */
        u_int8_t                              acpi_disable;           /* Value to write to smi_cmd to disable ACPI */
        u_int8_t                              S4bios_req;             /* Value to write to SMI CMD to enter S4BIOS state */
        u_int8_t                              reserved2;              /* Reserved - must be zero */
        u_int32_t                             pm1a_evt_blk;           /* Port address of Power Mgt 1a acpi_event Reg Blk */
        u_int32_t                             pm1b_evt_blk;           /* Port address of Power Mgt 1b acpi_event Reg Blk */
        u_int32_t                             pm1a_cnt_blk;           /* Port address of Power Mgt 1a Control Reg Blk */
        u_int32_t                             pm1b_cnt_blk;           /* Port address of Power Mgt 1b Control Reg Blk */
        u_int32_t                             pm2_cnt_blk;            /* Port address of Power Mgt 2 Control Reg Blk */
        u_int32_t                             pm_tmr_blk;             /* Port address of Power Mgt Timer Ctrl Reg Blk */
        u_int32_t                             gpe0_blk;               /* Port addr of General Purpose acpi_event 0 Reg Blk */
        u_int32_t                             gpe1_blk;               /* Port addr of General Purpose acpi_event 1 Reg Blk */
        u_int8_t                              pm1_evt_len;            /* Byte length of ports at pm1_x_evt_blk */
        u_int8_t                              pm1_cnt_len;            /* Byte length of ports at pm1_x_cnt_blk */
        u_int8_t                              pm2_cnt_len;            /* Byte Length of ports at pm2_cnt_blk */
        u_int8_t                              pm_tmr_len;              /* Byte Length of ports at pm_tm_blk */
        u_int8_t                              gpe0_blk_len;           /* Byte Length of ports at gpe0_blk */
        u_int8_t                              gpe1_blk_len;           /* Byte Length of ports at gpe1_blk */
        u_int8_t                              gpe1_base;              /* Offset in gpe model where gpe1 events start */
        u_int8_t                              reserved3;              /* Reserved */
        u_int16_t                             plvl2_lat;              /* Worst case HW latency to enter/exit C2 state */
        u_int16_t                             plvl3_lat;              /* Worst case HW latency to enter/exit C3 state */
        u_int16_t                             flush_size;             /* Size of area read to flush caches */
        u_int16_t                             flush_stride;           /* Stride used in flushing caches */
        u_int8_t                              duty_offset;            /* Bit location of duty cycle field in p_cnt reg */
        u_int8_t                              duty_width;             /* Bit width of duty cycle field in p_cnt reg */
        u_int8_t                              day_alrm;               /* Index to day-of-month alarm in RTC CMOS RAM */
        u_int8_t                              mon_alrm;               /* Index to month-of-year alarm in RTC CMOS RAM */
        u_int8_t                              century;                /* Index to century in RTC CMOS RAM */
        u_int8_t                              reserved4;              /* Reserved */
        u_int8_t                              reserved4a;             /* Reserved */
        u_int8_t                              reserved4b;             /* Reserved */
#if 0
        u_int32_t                             wb_invd         : 1;    /* The wbinvd instruction works properly */
        u_int32_t                             wb_invd_flush   : 1;    /* The wbinvd flushes but does not invalidate */
        u_int32_t                             proc_c1         : 1;    /* All processors support C1 state */
        u_int32_t                             plvl2_up        : 1;    /* C2 state works on MP system */
        u_int32_t                             pwr_button      : 1;    /* Power button is handled as a generic feature */
        u_int32_t                             sleep_button    : 1;    /* Sleep button is handled as a generic feature, or not present */
        u_int32_t                             fixed_rTC       : 1;    /* RTC wakeup stat not in fixed register space */
        u_int32_t                             rtcs4           : 1;    /* RTC wakeup stat not possible from S4 */
        u_int32_t                             tmr_val_ext     : 1;    /* The tmr_val width is 32 bits (0 = 24 bits) */
        u_int32_t                             reserved5       : 23;   /* Reserved - must be zero */
#else
        u_int32_t flags;
#endif
} __attribute__((__packed__));

