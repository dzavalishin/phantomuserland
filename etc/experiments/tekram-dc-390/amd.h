
/* SCSI Commands */

#define DMA_COMMAND   	    	BIT(7)
#define NOP_CMD 	    	   	0
#define CLEAR_FIFO_CMD	    	1
#define RST_DEVICE_CMD	    	2
#define RST_SCSI_BUS_CMD    	3
#define INFO_XFER_CMD	    	0x10
#define INITIATOR_CMD_CMPLTE	0x11
#define MSG_ACCEPTED_CMD    	0x12
#define XFER_PAD_BYTE	     	0x18
#define SET_ATN_CMD	     	  	0x1A
#define RESET_ATN_CMD   	 	0x1B
#define SEL_W_ATN				0x42
#define SEL_W_ATN_STOP	    	0x43
#define EN_SEL_RESEL	    	0x44
#define SEL_W_ATN2	   	    	0x46
#define DATA_XFER_CMD	    	INFO_XFER_CMD

#define MAX_SCSI_ID		8
#define AMD_MAX_SYNC_OFFSET	15
#define AMD_TARGET_MAX	7
#define AMD_LUN_MAX		7
#define AMD_NSEG		(btoc(MAXPHYS) + 1)
#define AMD_MAXTRANSFER_SIZE	0xFFFFFF /* restricted by 24 bit counter */
#define MAX_DEVICES		10
#define MAX_TAGS_CMD_QUEUE	256
#define MAX_CMD_PER_LUN		6
#define MAX_SRB_CNT		256
#define MAX_START_JOB		256

/*
 *==========================================================
 *           SCSI Chip register address offset
 *==========================================================
 */
#define CTCREG_LOW   	0x00	/* (R)   current transfer count register low */
#define STCREG_LOW   	0x00	/* (W)   start transfer count register low */

#define CTCREG_MID   	0x04	/* (R)   current transfer count register
				 * middle */
#define STCREG_MID   	0x04	/* (W)   start transfer count register middle */

#define SCSIFIFOREG    	0x08	/* (R/W) SCSI FIFO register */

#define SCSICMDREG     	0x0C	/* (R/W) SCSI command register */

#define SCSISTATREG  	0x10	/* (R)   SCSI status register */
#define SCSIDESTIDREG  	0x10	/* (W)   SCSI destination ID register */

#define INTSTATREG   	0x14	/* (R)   interrupt status register */
#define SCSITIMEOUTREG 	0x14	/* (W)   SCSI timeout register */


#define INTERNSTATREG  	0x18	/* (R)   internal state register */
#define SYNCPERIOREG  	0x18	/* (W)   synchronous transfer period register */

#define CURRENTFIFOREG  0x1C	/* (R)   current FIFO/internal state register */
#define SYNCOFFREG 	    0x1C/* (W)   synchronous transfer period register */

#define CNTLREG1    	0x20	/* (R/W) control register 1 */
#define CLKFACTREG  	0x24	/* (W)   clock factor register */
#define CNTLREG2    	0x2C	/* (R/W) control register 2 */
#define CNTLREG3    	0x30	/* (R/W) control register 3 */
#define CNTLREG4    	0x34	/* (R/W) control register 4 */

#define CURTXTCNTREG  	0x38	/* (R)   current transfer count register
				 * high/part-unique ID code */
#define STCREG_HIGH  	0x38	/* (W)   Start current transfer count register
				 * high */

/* Am53c974 register table */
#define STCLREG		    	0x00	/* w	start transf. count, low byte      */
#define CTCLREG		    	0x00	/* r	current transf. count, low byte    */
#define STCMREG		    	0x04	/* w	start transf. count, middle byte   */
#define CTCMREG			   	0x04	/* r 	current transf. count, middle byte */
#define STCHREG		    	0x38	/* w 	start transf. count, high byte     */
#define STIMREG		    	0x14	/* w	SCSI timeout reg.		   */
//#define CFIREG		    	0x1C	/* r	current FIFO/internal state reg.   */
#define CNTLREG1	    	0x20	/* rw	control register one		   */
#define CLKFREG		    	0x24	/* w	clock factor reg.		   */
/* Am53c974 DMA Engine */
#define DMACMD		    	0x40	/* rw	command				   */
#define DMASTATUS	      	0x54	/* r	status register			   */
#define DMASTC		    	0x44	/* rw	starting transfer count		   */
#define DMASPA		    	0x48	/* rw	starting physical address	   */
#define DMAWBC		    	0x4C	/* r	working byte counter		   */
#define DMAWAC		    	0x50	/* r	working address counter		   */
#define DMASMDLA	    	0x58	/* rw	starting MDL address		   */
#define DMAWMAC		    	0x5C	/* r	working MDL counter		   */
#define DMACMD_DIR		0x80	/* transfer direction (1=read from device) */
#define DMACMD_INTE_D		0x00/*0x40	/* DMA transfer interrupt enable 	   */
#define DMACMD_INTE_P		0x20	/* page transfer interrupt enable 	   */
#define DMACMD_MDL		0x10	/* map to memory descriptor list 	   */
#define DMACMD_DIAG		0x04	/* diagnostics, set to 0		   */
#define DMACMD_IDLE 		0x00	/* idle cmd			 	   */
#define DMACMD_BLAST		0x01	/* flush FIFO to memory		 	   */
#define DMACMD_ABORT		0x02	/* terminate DMA		 	   */
#define DMACMD_START		0x03	/* start DMA			 	   */
#define DMASTATUS_BCMPLT	0x20	/* BLAST complete			   */
#define DMASTATUS_SCSIINT	0x10	/* SCSI interrupt pending		   */
#define DMASTATUS_DONE		0x08	/* DMA transfer terminated		   */
#define DMASTATUS_ABORT		0x04	/* DMA transfer aborted			   */
#define DMASTATUS_ERROR		0x02	/* DMA transfer error			   */
#define DMASTATUS_PWDN		0x02	/* power down indicator			   */


/* Register bit map */
#define CNTLREG1_DISR		0x40	/* disable interrupt on SCSI reset	   */
#define CNTLREG1_ETM		0x80	/* set extended timing mode		   */
#define CNTLREG1_DISR		0x40	/* disable interrupt on SCSI reset	   */
#define CNTLREG1_PERE		0x10	/* enable parity error reporting	   */
#define CNTLREG1_SID		0x07	/* host adapter SCSI ID			   */

#define STATREG_PHASE       0x07    /* SCSI phase mask 			   */
#define STATREG_IO		0x01	/* SCSI I/O phase (latched?)		   */
#define STATREG_PE		0x10	/* SCSI illegal operation error detected   */
#define STATREG_IOE		0x40	/* SCSI illegal operation error detected   */

#define INSTREG_SRST		0x80	/* SCSI reset detected			   */
#define INSTREG_ICMD		0x40	/* SCSI invalid command detected	   */
#define INSTREG_DIS		0x20	/* target disconnected or sel/resel timeout*/
#define INSTREG_SR		0x10	/* device on bus has service request       */
#define INSTREG_SO		0x08	/* successful operation			   */
#define INSTREG_RESEL		0x04	/* device reselected as initiator	   */
#define INSTREG_SEL		0x02	/* device selected as target	   */
#define INSTREG_SELWA		0x01	/* device selected as target with atn */

#define SDIDREG_MASK		0x07	/* mask					   */
#define CLKFREG_MASK		0x07	/* mask					   */

#define CNTLREG2_ENF		0x40	/* enable features			   */

#define CNTLREG3_ADIDCHK	0x80	/* additional ID check			   */
#define CNTLREG3_FASTSCSI	0x10	/* fast SCSI				   */
#define FAST_SCSI	    	BIT(4)	/* ;10MB/SEC */
#define CNTLREG3_FASTCLK	0x08	/* fast SCSI clocking			   */

#define CNTLREG4_GLITCH		0xC0	/* glitch eater				   */
#define CNTLREG4_PWD		0x20	/* reduced power feature		   */
#define CNTLREG4_RAE		0x08	/* write only, active negot. ctrl.	   */
#define CNTLREG4_RADE		0x04	/* active negot. ctrl.			   */
#define CNTLREG4_RES		0x10	/* reserved bit, must be 1		   */
#define EATER_12NS	    	0
#define EATER_25NS	    	BIT(7)
#define EATER_35NS	    	BIT(6)
#define EATER_0NS	    	(BIT(7)+BIT(6))
#define NEGATE_REQACKDATA	BIT(2)
#define NEGATE_REQACK		BIT(3)
#define COUNT_2_ZERO		BIT(4)

#define INTERRUPT	    	BIT(7)

#define STPREG_STP		0x1F	/* synchr. transfer period		   */

#define DMASTATUS_BCMPLT	0x20	/* BLAST complete			   */
#define DMASTATUS_SCSIINT	0x10	/* SCSI interrupt pending		   */
#define DMASTATUS_DONE		0x08	/* DMA transfer terminated		   */
#define DMASTATUS_ABORT		0x04	/* DMA transfer aborted			   */
#define DMASTATUS_ERROR		0x02	/* DMA transfer error			   */
#define DMASTATUS_PWDN		0x02	/* power down indicator			   */

/*** SCSI phases ***/
#define PHASE_MSGIN             0x07
#define PHASE_MSGOUT            0x06
#define PHASE_RES_1             0x05
#define PHASE_RES_0             0x04
#define PHASE_STATIN            0x03
#define PHASE_CMDOUT            0x02
#define PHASE_DATAIN            0x01
#define PHASE_DATAOUT           0x00

/*
 *     ------SCSI Register-------
 *     Control Reg. 2(+2CH)
 */
#define EN_FEATURE	    	BIT(6)
#define EN_SCSI2_CMD		BIT(3)


typedef unsigned char u_int8_t;
typedef uint16 u_int16_t;
typedef uint32 u_int32_t;

struct amd_transinfo {
	u_int8_t period;
	u_int8_t offset;
};

/* From Be's example (sim_buslogic) */
#define MAX_SCATTER 130


struct amd_target_info {
	/*
	 * Records the currently active and user/default settings for
	 * tagged queueing and disconnection for each target.
	 */
	u_int8_t disc_tag;
#define		AMD_CUR_DISCENB	0x01
#define		AMD_CUR_TAGENB	0x02
#define		AMD_USR_DISCENB	0x04
#define		AMD_USR_TAGENB	0x08
	u_int8_t   CtrlR1;
	u_int8_t   CtrlR3;
	u_int8_t   CtrlR4;
	u_int8_t   sync_period_reg;
	u_int8_t   sync_offset_reg;


	/*
	 * Currently active transfer settings.
	 */
	struct amd_transinfo current;
	/*
	 * Transfer settings we wish to achieve
	 * through negotiation.
	 */
	struct amd_transinfo goal;
	/*
	 * User defined or default transfer settings.
	 */
	struct amd_transinfo user;
};
struct amd_sg {
	u_int32_t   SGXLen;
	u_int32_t   SGXPtr;
};

#define NB_INTR 10

struct amd_ccb {
    sem_id ccb_lock;			/* indicates command completion  */
	uint32 length;
	uint32 IOnbr2;
	bool command_completed;
	bool command_error;
	uint32 nb_entries;
	physical_entry entries[MAX_SCATTER];
};

typedef struct amd_softc {
    sem_id big_lock;			/* lock :-( */
    sem_id hw_lock;				/* lock protectin the scsi adapter */
    sem_id step_lock;			/* indicates scsi step completion */

	int pciid;					/* PCI card id */
	int iobase;					/* I/A base address */
	int irq;					/* IRQ number */

	u_int8_t AdaptSCSIID;		/* Adapter SCSI Target ID */
	u_int8_t target_selected;	/* target selected (true/false) */
	u_int8_t bus_phase;

	/* temporaire */
	int interrupt;
	int _INSTREG[NB_INTR];
	int _ISREG[NB_INTR];
	int _STATREG[NB_INTR];
	int _DMASTATUS[NB_INTR];
	int _CMDREG[NB_INTR];
	u_int8_t bus_phase_out[NB_INTR];
	u_int8_t _CFIREG[NB_INTR];
	bool _semrel[NB_INTR];
	const char *intr_when[NB_INTR];

	struct	   amd_target_info tinfo[AMD_TARGET_MAX+1];
	u_int16_t  max_id;
	u_int8_t data[100];
	u_int8_t nb_data;

	struct amd_ccb ccb;
	
/* Debug and stats only */
	bool enter;
	bool verbose;
	char debugstr[1024];
	unsigned int nb_io;
	unsigned int nb_errors;
	unsigned char lastcmd;
	const char when[20];
} Am53c974a;
