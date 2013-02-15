
/* TO DO
- Gerer le semaphone ccb_lock
- Integrer la table des SG dans les ccb
- Use driver settings API
- AMD_MAXTRANSFER_SIZE ( 1/2 done : panic if > )
- Gestion des io partielles ( 1/2 done )
- Gestion des timeouts
- learn how to handle messages
- Gestion des erreurs
- Remanier la gestion des interruptions
- Ne pas supprimer/recreer le semaphore
- Mieux traiter les flags cam ( 1/2 done ??? )
- Multiple cards (I only own one :(
- Use "per target ctrl regs" instead of global ctrl regs
- regarder le code de retour dans exec io
- Dans le cas ou il y a 0 data, il ne faut relacher
  qu'une seule fois le semaphore : à la fin de la
  selection. Ou mieux, différentier les "étapes"
  scsi de la fin de l'instruction CAM. ( done )
*/


//-----------------------------------------------------------
// Do it yourself : comment/uncomment these #defines to match your needs

// Uncomment this one if you want debug out on console
// (useful on bootfloppy)
#define debug_print dprintf

// Comment/uncomment whether or not you want performance info
//#define AMDPERF

// Display driver init information IN SYSLOG
#define AMDINITDEBUG

//#define AMD_TRACE_WRITE
//-----------------------------------------------------------


#define debug_sem(when)								\
{													\
	int32 count;									\
	get_sem_count(am->step_lock,&count);			\
	if (count != 0) {								\
		sprintf(debug,"### Semaphore step pb (%s) count = %d\n",when,(int)count);	\
		debug_print(debug);							\
	}												\
	get_sem_count(am->ccb.ccb_lock,&count);			\
	if (count != 0) {								\
		sprintf(debug,"### Semaphore ccb pb (%s) count = %d\n",when,(int)count);	\
		debug_print(debug);							\
	}												\
}

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <PCI.h>
#include <CAM.h>

/* static char debug[1024]; */
#define debug (amd->debugstr)

#define BIT(N) (0x01 << N)

/* driver API version*/
int32  api_version = B_CUR_DRIVER_API_VERSION;

/* PCI Module */
static char	pci_name[] = B_PCI_MODULE_NAME;
static pci_module_info		*pci;

/* CAM Module */
static char	cam_name[] = B_CAM_FOR_SIM_MODULE_NAME;
static cam_for_sim_module_info	*cam;

/* Infos sur la carte tekram */
#ifndef PCI_VENDOR_ID_AMD
 #define PCI_VENDOR_ID_AMD		0x1022
 #define PCI_DEVICE_ID_AMD_SCSI  0x2020
#endif

#define DEF_STP                 8    /* STPREG value assuming 5.0 MB/sec, FASTCLK, FASTSCSI */
#define DEF_SOF_RAD             0    /* REQ/ACK deassertion delay */
#define DEF_SOF_RAA             0    /* REQ/ACK assertion delay */
#define DEF_ETM                 0    /* CNTLREG1, ext. timing mode */
#define DEF_PERE                1    /* CNTLREG1, parity error reporting */
#define DEF_CLKF                0    /* CLKFREG,  0=40 Mhz */
#define DEF_ENF                 1    /* CNTLREG2, enable features */
#define DEF_SCSI2				1    /* CNTLREG2, enable SCSI 2 */
#define DEF_ADIDCHK             0    /* CNTLREG3, additional ID check */
#define DEF_FASTSCSI            DEF_SCSI2    /* CNTLREG3, fast SCSI */
#define DEF_GLITCH              2    /* CNTLREG4, glitch eater, 0=12ns, 1=35ns, 2=25ns, 3=off */
#define DEF_PWD                 0    /* CNTLREG4, reduced power feature */
#define DEF_RAE                 0    /* CNTLREG4, RAE active negation on REQ, ACK only */
#define DEF_RADE                1    /* 1CNTLREG4, active negation on REQ, ACK and data */

/*
 *========================================================= *
   				SCSI Chip register address offset
 *========================================================= */


#define PARITY_ERR_REPO 	BIT(4)
#define ID_MSG_CHECK		BIT(7)
#define EN_QTAG_MSG	    	BIT(6)
#define EN_GRP2_CMD	    	BIT(5)
#define FAST_SCSI	    	BIT(4)	/* ;10MB/SEC */
#define FAST_CLK	    	BIT(3)	/* ;25 - 40 MHZ */
#define CLK_FREQ_40MHZ		0
#define CLK_FREQ_35MHZ		(BIT(2)+BIT(1)+BIT(0))
#define CLK_FREQ_30MHZ		(BIT(2)+BIT(1))
#define CLK_FREQ_25MHZ		(BIT(2)+BIT(0))
#define CLK_FREQ_20MHZ		BIT(2)
#define CLK_FREQ_15MHZ		(BIT(1)+BIT(0))
#define CLK_FREQ_10MHZ		BIT(1)
#define EN_FEATURE	    	BIT(6)
#define EN_SCSI2_CMD		BIT(3)
#define DIS_INT_ON_SCSI_RST	BIT(6)
#define READ_DIRECTION		BIT(7)

#include "amd.h"


/* DMA Commands */
#define DMACMD_IDLE 		0x00	/* idle cmd			 	   */

/* ---------------- FreeBSD ----------------*/

#define AMD_TRANS_CUR		0x01	/* Modify current neogtiation status */
#define AMD_TRANS_ACTIVE	0x03	/* Assume this is the active target */
#define AMD_TRANS_GOAL		0x04	/* Modify negotiation goal */
#define AMD_TRANS_USER		0x08	/* Modify user negotiation settings */


/*
** Constants for the SIM 
*/
#define SIM_VERSION 0x01
#define SIM_VERSION_TXT "0.5.5"
#define HBA_VERSION 0x01

static char sim_vendor_name[]   = "S. Fritsch";
static char hba_vendor_name[]   = "AMD";
static char controller_family[] = "Tekram";
static char productname[] = "DC 390";

static void display_ccb_scsiio(CCB_SCSIIO *ccb, int longformat);
static void display_interrupt(struct amd_softc *am, const char * when);
static status_t init_amd_scsi (Am53c974a *am);

/* Macros */
#define amd_read8(instance, reg) ((*pci->read_io_8)(instance->iobase + reg))

#ifndef AMD_TRACE_WRITE
#define amd_write8(instance, reg, value) (*pci->write_io_8)(instance->iobase + reg, value)
#else
void amd_write8(Am53c974a *instance, unsigned int reg, unsigned char value) {
	if (instance->verbose) {
		switch(reg) {
			case 0: sprintf(debug,">Count low : %X\n",value); break;
			case 4: sprintf(debug,">Count middle : %X\n",value); break;
			case 0x08: sprintf(debug,">FIFO : %X\n",value); break;
			case 0x0c: sprintf(debug,">scsi cmd : %X\n",value); break;
			case 0x10: sprintf(debug,">scsi dest : %X\n",value); break;
			case 0x1c: sprintf(debug,">sync off reg : %X\n",value); break;
			case 0x20: sprintf(debug,">ctrl1 : %X\n",value); break;
			case 0x2c: sprintf(debug,">ctrl2 : %X\n",value); break;
			case 0x30: sprintf(debug,">ctrl3 : %X\n",value); break;
			case 0x34: sprintf(debug,">ctrl4 : %X\n",value); break;
			case 0x38: sprintf(debug,">Count high : %X\n",value); break;
			case 0x40: sprintf(debug,">cmd : %X\n",value); break;
			default: sprintf(debug,">iobase+%X : %X\n",reg,value);
		}
		debug_print(debug);
	}
	(*pci->write_io_8)(instance->iobase + reg, value);
}
#endif

static void amd_write_len(Am53c974a *am, uint32 len) {
	if (len>AMD_MAXTRANSFER_SIZE)
		panic("AMD - Max io len reached"); 
	amd_write8(am, STCLREG, (unsigned char)(len & 0xff));
	amd_write8(am, STCMREG, (unsigned char)((len & 0xff00) >> 8));
	amd_write8(am, STCHREG, (unsigned char)((len & 0xff0000) >> 16));
	(*pci->write_io_32)(am->iobase + DMASTC, len & 0xffffff);
}

/* ### BAD : remove this to handle multiple cards */
Am53c974a *amd;


static void
amdsetsync(struct amd_softc *am, u_int target, u_int clockrate,
	   u_int period, u_int offset, u_int type)
{
	struct amd_target_info *tinfo;
	u_int old_period;
	u_int old_offset;

	tinfo = &am->tinfo[target];
	old_period = tinfo->current.period;
	old_offset = tinfo->current.offset;
	if ((type & AMD_TRANS_CUR) != 0
	 && (old_period != period || old_offset != offset)) {

		tinfo->current.period = period;
		tinfo->current.offset = offset;
		tinfo->sync_period_reg = clockrate;
		tinfo->sync_offset_reg = offset;
		tinfo->CtrlR3 &= ~FAST_SCSI;
		tinfo->CtrlR4 &= ~EATER_25NS;
		if (clockrate > 7)
			tinfo->CtrlR4 |= EATER_25NS;
		else
			tinfo->CtrlR3 |= FAST_SCSI;

		if ((type & AMD_TRANS_ACTIVE) == AMD_TRANS_ACTIVE) {
			amd_write8(am, SYNCPERIOREG, tinfo->sync_period_reg);
			amd_write8(am, SYNCOFFREG, tinfo->sync_offset_reg);
			amd_write8(am, CNTLREG3, tinfo->CtrlR3);
			amd_write8(am, CNTLREG4, tinfo->CtrlR4);
		}
	}
	if ((type & AMD_TRANS_GOAL) != 0) {
		tinfo->goal.period = period;
		tinfo->goal.offset = offset;
	}

	if ((type & AMD_TRANS_USER) != 0) {
		tinfo->user.period = period;
		tinfo->user.offset = offset;
	}
}

/* Interrupt handler */

static int32
scsi_int_dispatch(void *data)
{
	/*
	Be carefull herein, do not block...
	otherwise cyril@be.com will be angry :
	
	Do not add any function call unless it is allowed in interrupt handler....
	Quite nothing is allowed here.
	
	Anyway 
	- no printf !!!
	- allways B_DO_NOT_RESCHEDULE in release_sem_etc
	*/

	Am53c974a *am = data;
	bool resched = false;
am->verbose=false;	
	if (am->interrupt>=NB_INTR) {
		am->interrupt=0;
		dprintf("AMD Warning : no space left to store intr ctx\n");
	}
	am->intr_when[am->interrupt] = am->when;
	am->_CMDREG[am->interrupt]=(*pci->read_io_8)(am->iobase + SCSICMDREG);

	am->_DMASTATUS[am->interrupt]=(*pci->read_io_32)(am->iobase + DMASTATUS);
			
	am->_STATREG[am->interrupt]=amd_read8(am, SCSISTATREG);
	am->_ISREG[am->interrupt]=(*pci->read_io_8)(am->iobase + INTERNSTATREG);
	am->_INSTREG[am->interrupt]=(*pci->read_io_8)(am->iobase + INTSTATREG);

	if (!(am->_STATREG[am->interrupt] & INTERRUPT) && !(am->_DMASTATUS[am->interrupt] & 8)) {
		// There is no interrupt from this adapter
/*
		kprintf("AMD Warning : IRQ 9 was flaged but no intr for me (irq sharing?)\n");
		kprintf("AMD dma=%X, stat=%X, isreg=%X, intrstat=%X\n",
			am->_DMASTATUS[am->interrupt],
			am->_STATREG[am->interrupt],
			am->_ISREG[am->interrupt],
			am->_INSTREG[am->interrupt]);
*/
	    return B_UNHANDLED_INTERRUPT;
	}

	am->_CFIREG[am->interrupt]=(amd_read8(am,CURRENTFIFOREG) & 0x1f);
// ### PATCH : on DMA intr, FIFO is not allways empty but we ignore it !
	if (!(am->_STATREG[am->interrupt] & INTERRUPT)) {
		am->_CFIREG[am->interrupt]=0;
	}

	if (am->_INSTREG[am->interrupt] & INSTREG_DIS) {
		am->target_selected = !(am->_INSTREG[am->interrupt] & INSTREG_DIS);
		release_sem_etc(am->ccb.ccb_lock, 1, B_DO_NOT_RESCHEDULE);
	}
	am->bus_phase = am->_STATREG[am->interrupt] & 0x7;


// Error detection
	if (am->ccb.command_completed) {
		/* This is an unexpected intr : what shoukd i do ? well, obviously handle it - but how ? */
//		dprintf("Amd intr : intr stat reg (+14h) 0x%X\n", am->_INSTREG[am->interrupt]);
	}
	if (am->_STATREG[am->interrupt] & STATREG_IOE) 
		am->ccb.command_error = true;
	if (am->_INSTREG[am->interrupt] & INSTREG_ICMD) 
		am->ccb.command_error = true;

// POST
	if(am->_CMDREG[am->interrupt]==SEL_W_ATN ) {
		resched = true;
		am->target_selected = !(am->_INSTREG[am->interrupt] & INSTREG_DIS);
	} else {
		if (am->_INSTREG[am->interrupt] & INSTREG_SR) {
			resched = true;
			am->ccb.IOnbr2--;
		}
	}
	if (am->target_selected != !(am->_INSTREG[am->interrupt] & INSTREG_DIS)) {
		/* This is an unexpected intr : what shoukd i do ? well, obviously handle it - but how ? */
		kprintf("AMD intr -- unexpected interruption on cmd 0x%X\n", am->_CMDREG[am->interrupt]);
	}
// PRE
	/* After DMA IO we get 2 intr (why ?) : DMA DONE and then SVC REQ
	   We ignore the first one
	*/
	if (am->bus_phase==PHASE_MSGIN) {
		int nb, t;
		t=(amd_read8(am,CURRENTFIFOREG) & 0x1f);
		for (nb=0;am->nb_data<100 && nb<t; nb++,am->nb_data++) { // nb_data is initialized once (at the beginning of IO)
			am->data[am->nb_data] = amd_read8(am,SCSIFIFOREG);
		} // add a panic if nd_data == 100
		amd_write8(am, SCSICMDREG, MSG_ACCEPTED_CMD); // 0x12
	} else
	if ((am->_INSTREG[am->interrupt] & INSTREG_SR)
			&& am->target_selected
			&& am->ccb.IOnbr2<=0
			&& !am->ccb.command_completed) {
		am->ccb.command_completed = true;
//		amd_write8(am, SCSICMDREG, CLEAR_FIFO_CMD); // ### WHY ?
		amd_write8(am, SCSICMDREG, INITIATOR_CMD_CMPLTE); // 0x11
	} 
/*	else if(am->ccb.command_completed && am->bus_phase==PHASE_MSGIN) {
		int nb, t;
		t=(amd_read8(am,CURRENTFIFOREG) & 0x1f);
		for (nb=0;am->nb_data<100 && nb<t; nb++,am->nb_data++) { // nb_data is initialized once (at the beginning of IO)
			am->data[am->nb_data] = amd_read8(am,SCSIFIFOREG);
		} // add a panic if nd_data == 100
		amd_write8(am, SCSICMDREG, MSG_ACCEPTED_CMD); // 0x12
//			amd_write8(am, SCSICMDREG, CLEAR_FIFO_CMD);
	}
*/
	am->_semrel[am->interrupt] = resched;

	am->interrupt++;
am->verbose=true;	
	if (resched) {
		release_sem_etc(am->step_lock, 1, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}
	am->bus_phase_out[am->interrupt] = am->_STATREG[am->interrupt] & 0x7;
	return B_HANDLED_INTERRUPT;
}

static void recover_phase(Am53c974a *am) {
	/* this function is called when an scsi error is detected. This
	mostly happends because of a bug in this driver :-(

	The purpose of this function is to "close" the scsi transaction.
	Maybe this should be changed to an scsi reset ?
	*/
	int t,len = 10000;
	int nb_retry;

	display_interrupt(am,"bfor recover");
	am->interrupt=0;
	strcpy(am->when,"recover");
	delete_sem(am->step_lock);
	am->step_lock = create_sem(0, "amd_step_lock");
	
	strcpy(debug,"### PHASE : ");
	switch(am->bus_phase) {
		case PHASE_MSGIN: strcat(debug,"Message In\n"); break;
		case PHASE_MSGOUT: strcat(debug,"Message Out\n"); break;
		case PHASE_STATIN: strcat(debug,"Status\n"); break;
		case PHASE_DATAIN : strcat(debug,"Data In\n"); break;
		case PHASE_DATAOUT: strcat(debug,"Data Out\n"); break;
		case PHASE_CMDOUT: strcat(debug,"Command\n"); break;
		default: strcat(debug," Unknown ###\n");
	}
	debug_print(debug);
	am->ccb.command_completed = true;
	debug_print("### AMD -- TRYING TO RECOVER FROM ERROR\n");
	/*-----------------------------------------------*/
	t=(amd_read8(am,CURRENTFIFOREG) & 0x1f);
	if (t>0) {
		int nb;
		debug_print("FIFO dump : ");
		for (nb=0 ; nb<t; nb++) {
			debug_print("%x ",amd_read8(am,SCSIFIFOREG));
		}
	}

	acquire_sem_etc(am->step_lock,1,B_RELATIVE_TIMEOUT,2000000);
	/*-----------------------------------------------*/
	display_interrupt(am,"After clean up");

	am->ccb.IOnbr2=0;
	am->interrupt=0;
	t = 0;

	amd_write_len(am,len);
	
	nb_retry = 5;
	for (t=0;t<7;t++)
	while (am->target_selected && nb_retry-->0) {
		/* ### Trying "hard" to disconnect */
		int RECOVER_COMMANDS[]= {
			XFER_PAD_BYTE, /* +DMA_COMMAND, */
			INITIATOR_CMD_CMPLTE,
			MSG_ACCEPTED_CMD,
			0x04,0x23,0x24,0x27,};
		amd_write8(am, SCSICMDREG, RECOVER_COMMANDS[t]);
		acquire_sem_etc(am->step_lock,1,B_RELATIVE_TIMEOUT,10000000);
		display_interrupt(am,"all cmds");
	}
	/* ### Last chance : scsi reset */
	init_amd_scsi (am);
	am->target_selected = !((*pci->read_io_8)(am->iobase + INTSTATREG) & INSTREG_DIS);
	if (am->target_selected)
		panic("AMD -- can't disconnect from target\n");
	else
		debug_print("### AMD -- forced disconnection successfull\n");
}

static void startCmd(Am53c974a *am, u_char target_lun, short cdb_len, u_char *cdb) {
	int t;
	/* Transfert 1-byte msg + cdb */

	amd_write_len(am,cdb_len+1);
	
	amd_write8(am, SCSIFIFOREG,(target_lun & 0x7)|0x80);

	for (t=0; t<cdb_len; t++) {
		amd_write8(am, SCSIFIFOREG,cdb[t]);
	}
	amd_write8(am, SCSICMDREG,SEL_W_ATN);
}

static void select_target(Am53c974a *am, CCB_SCSIIO *ccb) {
	u_char *cdb;
	struct	   amd_target_info *tinfo;
	tinfo=& am->tinfo[ccb->cam_ch.cam_target_id];

	amd_write8(am, SCSIDESTIDREG, SDIDREG_MASK & ccb->cam_ch.cam_target_id);
	amd_write8(am, SYNCOFFREG, (DEF_SOF_RAD<<6) | (DEF_SOF_RAA<<4) );

	/* Old way : */
	amd_write8(am, CNTLREG1, (DEF_ETM<<7) /*| CNTLREG1_DISR*/ | (DEF_PERE<<4) | am->AdaptSCSIID);
	amd_write8(am, CNTLREG2, EN_FEATURE | EN_SCSI2_CMD);
	amd_write8(am, CNTLREG3, (DEF_ADIDCHK<<7) | (DEF_FASTSCSI<<4) | FAST_CLK);
	amd_write8(am, CNTLREG4, (DEF_GLITCH<<6) | (DEF_PWD<<5) | (DEF_RAE<<3) | (DEF_RADE<<2) );
	/* New way : */
	amd_write8(am, CNTLREG1, tinfo->CtrlR1 | am->AdaptSCSIID);
	amd_write8(am, CNTLREG2, EN_FEATURE | EN_SCSI2_CMD);
	amd_write8(am, CNTLREG3, tinfo->CtrlR3);
	amd_write8(am, CNTLREG4, tinfo->CtrlR4 );

	amd_write8(am, DMACMD, DMACMD_IDLE);
	amd_write8(am, SCSICMDREG, CLEAR_FIFO_CMD);

	if (ccb->cam_ch.cam_flags & CAM_CDB_POINTER) {
		cdb=ccb->cam_cdb_io.cam_cdb_ptr;
	} else {
		cdb=ccb->cam_cdb_io.cam_cdb_bytes;
	}

	startCmd(am, ccb->cam_ch.cam_target_lun, ccb->cam_cdb_len, cdb);
}

static void requestSense(Am53c974a *am, u_char target_lun, short sense_len) {
	const u_char REQUEST_SENSE = 0x03;
	u_char cdb[6];
	cdb[0] = REQUEST_SENSE;
	cdb[1] = target_lun << 5;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = sense_len;
	cdb[5] = 0;
	startCmd(am, target_lun, sizeof(cdb), cdb);
}

/*static*/ int32 sim_execute_scsi_io(Am53c974a *am, CCB_HEADER *ccbh)
{
	int err, t;
	uint32 nb_left = 0;
	CCB_SCSIIO *ccb;
	u_char *cdb;
	physical_entry *chunk;

#ifdef AMDPERF
	bigtime_t t1, t2;
	t1 = system_time();
#endif

	am->ccb.command_completed = false;
	am->nb_io++;

	acquire_sem(am->big_lock);

	ccb = (CCB_SCSIIO *) ccbh;
	if (ccb->cam_ch.cam_flags & CAM_CDB_POINTER) {
		cdb=ccb->cam_cdb_io.cam_cdb_ptr;
	} else {
		cdb=ccb->cam_cdb_io.cam_cdb_bytes;
	}	

/* ### 
	dprintf("Amd: t%dl%d cmd 0x%X\n", ccb->cam_ch.cam_target_id, ccb->cam_ch.cam_target_lun,cdb[0]);
*/
	if (ccb->cam_ch.cam_flags & CAM_CDB_LINKED)
		debug_print("# WARNING : linked ccb (flag)\n");

	acquire_sem(am->hw_lock); 

	/* serious things !!! */
	am->bus_phase = amd_read8(am,SCSISTATREG) & STATREG_PHASE;

/* ### not good(tm) */
if(am->target_selected || am->bus_phase != PHASE_DATAOUT) {
	recover_phase(am);  
}
	if (am->target_selected) {
		ccbh->cam_status = CAM_REQ_INVALID;
		release_sem(am->big_lock);
	    release_sem(am->hw_lock);
am->enter=false;
  		return B_ERROR;
	}
/* ### This is really bad !!! but sem will be in a correct state */
	debug_sem("before dest sem step");
	delete_sem(am->step_lock);
	am->step_lock = create_sem(0, "amd_step_lock");
	am->ccb.ccb_lock = create_sem(0, "amd_ccb_lock");
/* ### ------------ */

	strcpy(am->when,"before select");
	display_interrupt(am,"before selection");
	am->interrupt = 0;

/* Fill the CCB_AMD */
	am->ccb.command_completed = false;
	am->ccb.command_error = false;
	am->nb_data = 0;
	am->ccb.nb_entries=0;

	/***
		Compute physical IO
	***/

	if ((ccb->cam_ch.cam_flags & CAM_DIR_NONE) == CAM_DIR_NONE) {
		// no data transfert
		am->ccb.IOnbr2 = 0;
		am->ccb.length = 0;
	} else {
		chunk=am->ccb.entries;
		nb_left = 0;
		if(ccb->cam_ch.cam_flags & CAM_SCATTER_VALID){
			int i;
			/* we're using scatter gather -- things just got trickier */
			iovec *iov = (iovec *) ccb->cam_data_ptr;

			for(i=0; i<ccb->cam_sglist_cnt && am->ccb.nb_entries<MAX_SCATTER ; i++){

			    get_memory_map(iov[i].iov_base, iov[i].iov_len, chunk, MAX_SCATTER-am->ccb.nb_entries);
			    while (am->ccb.nb_entries<MAX_SCATTER && chunk->size>0) {
			    	nb_left += chunk->size;
			    	am->ccb.nb_entries++;
			    	chunk++;
			    }
			}
			/* ------ debug only ------ */
			if (am->ccb.nb_entries>ccb->cam_sglist_cnt)
				debug_print("# nb SG %d / nb chunk %ld\n",ccb->cam_sglist_cnt,am->ccb.nb_entries);
			/* ------------------------ */

		} else { // Simple DMA (not SG)
		    get_memory_map(ccb->cam_data_ptr, ccb->cam_dxfer_len, am->ccb.entries, MAX_SCATTER);
		    while (am->ccb.nb_entries<MAX_SCATTER && chunk->size>0) {
			nb_left += chunk->size;
			am->ccb.nb_entries++;
			chunk++;
		    }
		}
		/* ------ debug only ------ */
		if (am->ccb.nb_entries>=MAX_SCATTER) panic("AMD -- MAX_SCATTER limit reached");
		/* ------------------------ */
		am->ccb.length = nb_left;
		am->ccb.IOnbr2=am->ccb.nb_entries;
	}

	/***
		Connection to target
	***/

	strcpy(am->when,"selecting");
	select_target(am,ccb);
	{
		// Target selection may be long (power mngt) multiple retries are allowed
		int retry = 10;
		while (retry>0 && acquire_sem_etc(am->step_lock,1,B_RELATIVE_TIMEOUT,2000000) != B_OK) {
			debug_print("# warning, target selection still in progress...\n");
			retry--;
		}
		if (retry <= 0) {
			debug_print("### error acquire sem/select 1\n");
			display_interrupt(am,"failed selection");
		}
	}
/*----------------------------------------------------------------------------------*/

	if (!am->target_selected && !am->ccb.command_completed) {
		// Inexistant Device or connection failed
		if (acquire_sem_etc(am->ccb.ccb_lock,1,B_RELATIVE_TIMEOUT,2000000) != B_OK) {
			debug_print("### error acquire sem/ccb end on select\n");
		}
display_ccb_scsiio(ccb,false);
//				display_interrupt("selection");
		debug_print(">>> Target not found\n");
		amd_write8(am, SCSICMDREG, CLEAR_FIFO_CMD);
	    ccbh->cam_status = CAM_SEL_TIMEOUT;
		debug_sem("Selection failed");
	    release_sem(am->hw_lock);
	    release_sem(am->big_lock);
am->enter=false;
	    return B_OK;
	}
		 
	/***
		Transfert phase
	***/

	display_ccb_scsiio(ccb,true);
	strcpy(am->when,"selected");
	display_interrupt(am,"selection");
	am->interrupt=0;

//	if ((ccb->cam_dxfer_len > 0) && ((ccb->cam_ch.cam_flags & CAM_DIR_NONE) !=CAM_DIR_NONE))
	if (am->ccb.nb_entries>0)
	{
		// got something to transfert
		unsigned char io_dir;
		int i;
		
		debug_sem("DMA xfert start");

		if (ccb->cam_data_ptr == NULL) {
			// but no buffer
			panic("AMD -- Invalid DMA address");
			ccb->cam_ch.cam_status = CAM_REQ_INVALID;
			/* ### BAD : Finish SCSI transaction */
			release_sem(am->hw_lock);
			release_sem(am->big_lock);
			am->enter=false;
			return CAM_REQ_CMP_ERR;
		}

		if ((ccb->cam_ch.cam_flags & CAM_DIR_NONE) == CAM_DIR_IN)
			io_dir = READ_DIRECTION;
		else
			io_dir = 0;

		amd_write8(am, SCSICMDREG, NOP_CMD);  
		amd_write8(am, DMACMD, io_dir| DMACMD_INTE_D); 
		
		for (i=0, chunk=am->ccb.entries; i<MAX_SCATTER && chunk->size>0; i++, chunk++) {
			u_int32_t resid;

			sprintf(am->when,"%d",i);
if (am->ccb.IOnbr2 != am->ccb.nb_entries - i) debug_print("AMD -- incoherent ionbr2\n");

			amd_write_len(am,chunk->size);
			(*pci->write_io_32)(am->iobase + DMASPA, (unsigned int)chunk->address);

			amd_write8(am, SCSICMDREG, DATA_XFER_CMD | DMA_COMMAND);
			amd_write8(am, DMACMD, io_dir | DMACMD_INTE_D | DMACMD_START);
				
			err = acquire_sem_etc(am->step_lock,1,B_RELATIVE_TIMEOUT,10000000);
						if (err != B_OK) {
							debug_print("### error acquire sem/dma end 3\n");
						}
			/*-----------------------------------------------*/
			amd_write8(am, DMACMD, io_dir); 

			if (am->ccb.command_error) break;

			strcpy(am->when,"dma done");
			display_interrupt(am,"DMA");
			am->interrupt=0;
			resid = amd_read8(am,STCLREG);
			resid += (unsigned)amd_read8(am,STCMREG) << 8;
			resid += (unsigned)amd_read8(am,STCHREG) << 16;
			nb_left -= chunk->size - resid;
				if (resid!=0) {
					sprintf(debug,"# Warning: DMA not completed %ld byte(s) left\n",resid);
					debug_print(debug);
				}
		}
	} /* End of xfert */

	strcpy(am->when,"waiting end");

/*	err = acquire_sem_etc(am->step_lock,1,B_RELATIVE_TIMEOUT,4000000);
						if (err != B_OK) {
							debug_print("### error acquire sem/cmd end 4\n");
						}
				/*-----------------------------------------------*/

	display_interrupt(am,"xfer end");


	if (am->target_selected) {
		/* Give it a chance to disconnect... */
		spin(100000);
		if (am->target_selected) {
			/* Still connected ? Something's wrong */
			am->ccb.command_error = true;
			debug_print("### Target selected !\n");
		}
	}

	if (am->ccb.IOnbr2 != 0) {
		debug_print("### IOnbr2=%ld",am->ccb.IOnbr2);
	}

	if (ccb->cam_dxfer_len>0) {
		if (nb_left>0) {
			/* ### NOT GOOD(tm) : DMA not complete */
			debug_print("### incomplete IO : %ld",nb_left);
			amd_write8(am, STCLREG, 0);
			amd_write8(am, STCMREG, 0);
			amd_write8(am, STCHREG, 0);
			(*pci->write_io_32)(am->iobase + DMASTC, 0);
			/* Should consider this as an error ? Probably no ! but BeOS freeze if not...*/ 
			am->ccb.command_error = true;
		}
		ccb->cam_resid = nb_left;
	}

	err = acquire_sem_etc(am->ccb.ccb_lock,1,B_RELATIVE_TIMEOUT,200000);
			if (err != B_OK) {
				debug_print("### error acquire ccb io disconnect\n");
			}

/* ### DEBUG ONLY ---- */
debug_sem("end of cmd");
strcpy(am->when,"done");
display_interrupt(am,"### END");
/* ### --------------- */
	
	if (am->ccb.command_error) {
		am->nb_errors++;
		ccb->cam_ch.cam_status = CAM_REQ_CMP_ERR;
		//ccb->cam_ch.cam_status = CAM_REQ_INVALID;
	} else {
		ccb->cam_ch.cam_status = CAM_REQ_CMP;
	}


	ccb->cam_sense_resid = ccb->cam_sense_len;
	if (am->nb_data == 0) {
		debug_print("NOTICE : no result message\n");
	} else if (am->nb_data != 2) {
		sprintf(debug,"### Bad Result message : nb bytes %d [", am->nb_data);
		debug_print(debug);
		for (t=0; t<am->nb_data; t++) {
			sprintf(debug,"%X ",am->data[t]);
			debug_print(debug);
		}
		debug_print("]\n");
// ### THIS MAY BE AN ERROR (according to the scsi spec)
		if (am->nb_data>0 && am->data[0]==0) {
			debug_print("# Warning : msg byte0=0 => continuing\n");
		} else {
			am->ccb.command_error = true;
			ccb->cam_ch.cam_status = CAM_REQ_CMP_ERR;
		}
// ###
	} else {
		// We have a 2-byte message
		int resid;
		// result message seems ok
		ccb->cam_scsi_status = am->data[0];
		switch (am->data[0]) {
			case 0x02: {
			if (ccb->cam_sense_len == 0) {
				debug_print("# Check condition but not Rq sense\n");
			} else {
				// "Check condition" -> request sense
				am->ccb.command_completed = false;
				am->ccb.command_error = false;
				am->ccb.length= ccb->cam_sense_len;
				am->ccb.IOnbr2 = 1;
				am->nb_data = 0;

				strcpy(am->when,"rq sel'ing");
				sprintf(am->when,"%p %p",am,ccb);
				requestSense(am, ccb->cam_ch.cam_target_lun, ccb->cam_sense_len);
				err = acquire_sem_etc(am->step_lock,1,B_RELATIVE_TIMEOUT,2000000);
				/*----------------------------------------------------------------*/
				if (err != B_OK) {
					panic("AMD -- error acquire sem/rq ss select 5\n");
				} else {
					amd_write8(am, SCSICMDREG, NOP_CMD);  
					amd_write8(am, DMACMD, READ_DIRECTION| DMACMD_INTE_D); 
		
					strcpy(am->when,"rq sel'ing mmap");
				    err = get_memory_map(ccb->cam_sense_ptr, ccb->cam_sense_len, am->ccb.entries, MAX_SCATTER);
					if (err) {
						debug_print("### error on get_memory_map : err %d",err);
						ccb->cam_ch.cam_status = CAM_REQ_CMP_ERR ;
						ccb->cam_sense_resid = 0;
					} else {
						if (am->ccb.entries[1].size>0)
							panic("AMD -- request sense too long");
						(*pci->write_io_32)(am->iobase + DMASPA, (unsigned int)am->ccb.entries[0].address);
						amd_write_len(am,am->ccb.entries[0].size);
			
						strcpy(am->when,"rq starting");
						amd_write8(am, SCSICMDREG, DATA_XFER_CMD | DMA_COMMAND);
						amd_write8(am, DMACMD, READ_DIRECTION | DMACMD_INTE_D | DMACMD_START);
	
						strcpy(am->when,"rq started");
						err = acquire_sem_etc(am->step_lock,1,B_RELATIVE_TIMEOUT,2000000);
								if (err != B_OK) {
									debug_print("### error acquire sem/rq ss io 1\n");
								}
						/*-----------------------------------------------*/
						strcpy(am->when,"rq done");
						display_interrupt(am,"ReqSense");
						am->interrupt=0;
			
						resid = amd_read8(am,STCLREG);
						resid += (unsigned)amd_read8(am,STCMREG) << 8;
						resid += (unsigned)amd_read8(am,STCHREG) << 16;
						if (resid) {
							debug_print("# Warning : request sense resid = %d",resid);
						}
			
						ccb->cam_ch.cam_status = CAM_REQ_CMP_ERR | CAM_AUTOSNS_VALID;
						ccb->cam_sense_resid = resid;
					}
				}
				err = acquire_sem_etc(am->ccb.ccb_lock,1,B_RELATIVE_TIMEOUT,200000);
						if (err != B_OK) {
							debug_print("### error acquire ccb sem/ disconnect reqsense\n");
						}
			}
		} break;
			case 0x00:
				//debug_print("NOTICE : default result\n");
				break;
			default:
				debug_print("# WARNING : unknown mesg code\n");
		}
	}

#ifdef AMDPERF
	t2 = system_time();
	sprintf(debug,"AMD[perf] duration %ld, size %ld, rate %d\n", (long)(t2-t1), ccb->cam_dxfer_len,(int)(((float)ccb->cam_dxfer_len)/(t2-t1)*1000));
	debug_print(debug);	
#endif


release_sem(am->hw_lock);

	switch(ccb->cam_ch.cam_status) {
		case CAM_REQ_CMP_ERR | CAM_AUTOSNS_VALID:
			strcpy(debug,"Result CMP_ERR + AUTOSNS_VALID ");
			sprintf(debug+strlen(debug),"  Autosense:%d[", ccb->cam_sense_len);
			for (t=0; t<ccb->cam_sense_len; t++) {
				sprintf(debug+strlen(debug),"%X ",ccb->cam_sense_ptr[t]);
			}
			strcat(debug,"]\n");
			debug_print(debug);	
			 break;
		case CAM_REQ_CMP_ERR:
			debug_print("Result ERROR\n"); break;
		case CAM_REQ_CMP:
			break;
		default : 
			debug_print("### Result unknown\n");
	}

release_sem(am->big_lock);
am->enter=false;
	return B_OK;
}

static long sim_path_inquiry(Am53c974a *am, CCB_HEADER *ccbh)
{
	CCB_PATHINQ	*ccb;
	dprintf("AMD -- sim_path_inquiry()\n");

	ccb = (CCB_PATHINQ *) ccbh;

	ccb->cam_version_num = SIM_VERSION;
	ccb->cam_target_sprt = 0;
	ccb->cam_hba_eng_cnt = 0;
	memset (ccb->cam_vuhba_flags, 0, VUHBA);
	ccb->cam_sim_priv = SIM_PRIV;
	ccb->cam_async_flags = 0;
	ccb->cam_initiator_id = am->AdaptSCSIID;
	/*~ccb->cam_hba_inquiry = am->wide ? PI_WIDE_16 : 0;*/
	ccb->cam_hba_inquiry = 0;
	strncpy (ccb->cam_sim_vid, sim_vendor_name, SIM_ID);
	strncpy (ccb->cam_hba_vid, hba_vendor_name, HBA_ID);
	ccb->cam_osd_usage = 0;
	ccbh->cam_status = CAM_REQ_CMP;
	return 0;
}

static long sim_extended_path_inquiry(Am53c974a *am, CCB_HEADER *ccbh)
{
	CCB_EXTENDED_PATHINQ *ccb;

	sim_path_inquiry(am, ccbh);
	ccb = (CCB_EXTENDED_PATHINQ *) ccbh;
	sprintf(ccb->cam_sim_version, SIM_VERSION_TXT);
	sprintf(ccb->cam_hba_version, "%d.0", HBA_VERSION);
	strncpy(ccb->cam_controller_family, controller_family, FAM_ID);
	strncpy(ccb->cam_controller_type, productname, TYPE_ID);
	return 0;
}

static long sim_init(void) {
	dprintf("AMD -- sim_init()\n");

	return 0;
}

static long sim_invalid(Am53c974a *am, CCB_HEADER *ccbh)
{
	ccbh->cam_status = CAM_REQ_INVALID;
	panic("AMD -- sim_invalid !!!!");
	return B_ERROR;
}
/*
** sim_release_queue unfreezes the target/lun's request queue.
*/
static long sim_sim_release_queue(Am53c974a *am, CCB_HEADER *ccbh)
{
	ccbh->cam_status = CAM_REQ_INVALID;
	return B_ERROR;
}


/*
** sim_set_async_callback registers a callback routine for the
** target/lun;
*/
static long sim_set_async_callback(Am53c974a *am, CCB_HEADER *ccbh)
{
	ccbh->cam_status = CAM_REQ_INVALID;
	return B_ERROR;
}


/*
** sim_abort aborts a pending or queued scsi operation.
*/
static long sim_abort(Am53c974a *am, CCB_HEADER *ccbh)
{
	ccbh->cam_status = CAM_REQ_INVALID;
	return B_ERROR;
}


/*
** sim_reset_bus resets the scsi bus.
*/
static long sim_reset_bus(Am53c974a *am, CCB_HEADER *ccbh)
{
	ccbh->cam_status = CAM_REQ_CMP;
	return 0;
}


/*
** sim_reset_device resets the target/lun with the scsi "bus
** device reset" command.
*/
static long sim_reset_device(Am53c974a *am, CCB_HEADER *ccbh)
{
	ccbh->cam_status = CAM_REQ_INVALID;
	return B_ERROR;
}


/*
** sim_terminate_process terminates the scsi i/o request without
** corrupting the medium.  It is used to stop lengthy requests
** when a higher priority request is available.
**
** Not yet implemented.
*/
static long sim_terminate_process(Am53c974a *am, CCB_HEADER *ccbh)
{
	ccbh->cam_status = CAM_REQ_INVALID;
	return B_ERROR;
}

/*static*/ long sim_action(CCB_HEADER *ccbh) {
	static long (*sim_functions[])(Am53c974a *, CCB_HEADER *) = {
		sim_invalid,		/* do nothing */
		sim_execute_scsi_io,	/* execute a scsi i/o command */
		sim_invalid,		/* get device type info */
		sim_path_inquiry,	/* path inquiry */
		sim_sim_release_queue,	/* release a frozen SIM queue */
		sim_set_async_callback,	/* set async callback parameters */
		sim_invalid,		/* set device type info */
		sim_invalid,		/* invalid function code */
		sim_invalid,		/* invalid function code */
		sim_invalid,		/* invalid function code */
		sim_invalid,		/* invalid function code */
		sim_invalid,		/* invalid function code */
		sim_invalid,		/* invalid function code */
		sim_invalid,		/* invalid function code */
		sim_invalid,		/* invalid function code */
		sim_invalid,		/* invalid function code */
		sim_abort,		/* abort the selected CCB */
		sim_reset_bus,		/* reset a SCSI bus */
		sim_reset_device,	/* reset a SCSI device */
		sim_terminate_process	/* terminate an i/o process */
	};
	uchar op;

		/* check for function codes out of range of dispatch table */
	op = ccbh->cam_func_code;
	if ((op >= sizeof (sim_functions) / sizeof (long (*)())) &&
	   (op != XPT_EXTENDED_PATH_INQ)) {
	    /* check for our vendor-uniques (if any) here... */
		ccbh->cam_status = CAM_REQ_INVALID;
		panic("AMD -- unknown operation code : %x",op);
		return -1;
	}

	ccbh->cam_status = CAM_REQ_INPROG;
	if (op == XPT_EXTENDED_PATH_INQ) {
		/*return sim_extended_path_inquiry(am, ccbh);*/
		return sim_extended_path_inquiry(amd, ccbh);
	} else {
		amd->enter=true;
		return (*sim_functions [op])(amd, ccbh);
	}
}


static void ResetDevParam(struct amd_softc * am)
{
	u_int target;

	for (target = 0; target <= am->max_id; target++) {
		if (am->AdaptSCSIID != target) {
			amdsetsync(am, target, /*clockrate*/0,
				   /*period*/0, /*offset*/0, AMD_TRANS_CUR);
		}
	}
}


static status_t
init_amd_scsi (Am53c974a *am)
{
	U32 bval;
	int i;
	
	am->verbose=true;
	dprintf("AMD -- init_amd_scsi\n");

	if(install_io_interrupt_handler(am->irq, scsi_int_dispatch, am, 0)
		== B_ERROR) {
		dprintf("AMD SCSI : can't install irq handler\n");
		return B_ERROR;
	}

	/* disable interrupts during SCSI Reset */
	bval = (*pci->read_io_8)(am->iobase + CNTLREG1);
	amd_write8(am, CNTLREG1,bval | CNTLREG1_DISR);
	amd_write8(am, SCSICMDREG, RST_SCSI_BUS_CMD);  /* SCSI BUS RESET */
	amd_write8(am, SCSICMDREG, NOP_CMD);
	spin(100000);
	amd_write8(am, CNTLREG1,bval);
	
	amd_write8(am, SCSICMDREG, CLEAR_FIFO_CMD);

	/* configure device */
	am->max_id = 7;
	for (i = 0; i <= am->max_id; i++) {

		if (am->AdaptSCSIID != i) {
			struct amd_target_info *tinfo;

			tinfo = &am->tinfo[i];
			tinfo->CtrlR1 = am->AdaptSCSIID;
			tinfo->CtrlR1 |= PARITY_ERR_REPO;
			tinfo->CtrlR3 = FAST_CLK;
			tinfo->CtrlR4 = EATER_25NS;
			tinfo->CtrlR4 |= NEGATE_REQACKDATA;
		}
	}
	amd_write8(am, SCSITIMEOUTREG, 153); /* 250ms selection timeout */
	/* Conversion factor = 0 , 40MHz clock */
	amd_write8(am, CLKFACTREG, CLK_FREQ_40MHZ);
	/* NOP cmd - clear command register */
	amd_write8(am, SCSICMDREG, NOP_CMD);	
	amd_write8(am, CNTLREG2, EN_FEATURE|EN_SCSI2_CMD);
	amd_write8(am, CNTLREG3, FAST_CLK);

	bval = EATER_25NS;
	bval |= NEGATE_REQACKDATA;
	amd_write8(am, CNTLREG4, bval);

	ResetDevParam(am);

	return B_OK;
}


static Am53c974a *create_cardinfo(int num, int iobase, int irq)
{
/* ### Il faut gérer le num pour les multi cartes ! */
	Am53c974a *am;

	am = (Am53c974a*) malloc(sizeof(Am53c974a));
	amd = am;
	am->iobase = iobase;
	am->irq = irq;
	am->interrupt=0;
	am->hw_lock = create_sem(1, "amd_hw_lock");
	am->step_lock = create_sem(0, "amd_step_lock");
	am->ccb.ccb_lock = create_sem(0, "amd_ccb_lock");
	am->big_lock = create_sem(1, "amd_big_lock");
	strcpy(am->when,"init");
	am->target_selected = 0;
	am->nb_data = 0;
	am->nb_io = 0;
	am->nb_errors = 0;

	/* ### BAD !!! Not necessarily ID 7 !!! */
	am->AdaptSCSIID = 7;

	init_amd_scsi(am);

	return am;
}


/*************************************************************/

status_t
init_hardware (void)
{
	dprintf("Amd init_hardware()\n");
	return B_OK;
}



int sim_install_amd(void)
{
	int i, irq, iobase, ret;
	pci_info h;
	CAM_SIM_ENTRY entry;

	dprintf("AMD -- sim_install()\n");

	for (i = 0; ; i++) {
		if ((*pci->get_nth_pci_info) (i, &h) != B_NO_ERROR) {
			/*if(!cardcount) d_printf("AMD SCSI: no controller found\n");*/
			break;
		}
		if ((h.vendor_id == PCI_VENDOR_ID_AMD) &&
		    (h.device_id == PCI_DEVICE_ID_AMD_SCSI)) {
			    dprintf("AMD -- sim_install : card found\n");
		    irq = h.u.h0.interrupt_line;
#ifdef __INTEL__
		    iobase = h.u.h0.base_registers[0];
#else
		    iobase = h.u.h0.base_registers[1];
#endif
				create_cardinfo(i,iobase,irq);
				
				if (get_module(cam_name,(module_info **) &cam) == B_OK) {
					entry.sim_init = sim_init;
					entry.sim_action = sim_action;
					ret = (*cam->xpt_bus_register)(&entry);
				} else {
					debug_print("AMD -- can't load cam module\n");
				}
				load_driver_symbols("amd");
	
	
		}
	}
	return B_OK;
}


//------------------------------------------------
static int amdstate(int argc, char **argv)
{

	kprintf("amd version %s : phase: %s\n",SIM_VERSION_TXT,amd->when);
	kprintf("total nbr of io: %d / nbr of errors: %d\n",amd->nb_io,amd->nb_errors);
	kprintf("amd last cmd : completed? %d / error? %d / entered %d\n",amd->ccb.command_completed,amd->ccb.command_error,amd->enter);
	kprintf("%d intr pending / LAST COMMAND %xh\n",amd->interrupt,amd->lastcmd);
	return 0;
}

static status_t std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		dprintf("AMD -- Module init\n");

		if (get_module(pci_name, (module_info **) &pci) != B_OK)
			return B_ERROR;

		if (get_module(cam_name, (module_info **) &cam) != B_OK) {
			put_module(pci_name);
			return B_ERROR;
		}

		if(sim_install_amd() == B_OK){
			add_debugger_command("amdstate", amdstate, "print amd(tekram) card info");
			return B_OK;
		}

		put_module(cam_name);
		put_module(pci_name);
		return B_ERROR;
		
	case B_MODULE_UNINIT:
		dprintf("AMD -- Module uninit\n");
		if (amd != NULL)
			remove_io_interrupt_handler(amd->irq, scsi_int_dispatch, amd);
		put_module(pci_name);
		put_module(cam_name);
		return B_OK;
	
	default:
		dprintf("AMD -- ### std_ops DEFAULT\n");
		return B_ERROR;
	}
}


static sim_module_info sim_amd_module = {
	{ "busses/scsi/amd", 0, &std_ops }
};

_EXPORT module_info  *modules[] = {
	(module_info *) &sim_amd_module,
	NULL
};


static void display_ccb_scsiio(CCB_SCSIIO *ccb, int longformat){
	u_char *cdb;
	const char *op_name=NULL;
	
	if (ccb->cam_ch.cam_flags & CAM_CDB_POINTER) {
		cdb=ccb->cam_cdb_io.cam_cdb_ptr;
	} else {
		cdb=ccb->cam_cdb_io.cam_cdb_bytes;
	}
	amd->lastcmd = cdb[0];
	switch(cdb[0]) {
		case 0x00: op_name="TestUnitReady "; break;
		case 0x01: op_name="RezeroUnit "; break;
		case 0x08: op_name="Read(6) "; break;
		case 0x0a: op_name="Write "; break;
		case 0x12: op_name="Inquiry "; break;
		case 0x15: op_name="ModeSelect "; break;
		case 0x1a: op_name="ModeSense "; break;
		case 0x1B: op_name="AllowPreventRemoval "; break;
		case 0x1E: op_name="StartStopUnit "; break;
		case 0x25: op_name="ReadCapacity "; break;
		case 0x28: op_name="Read(10) "; break;
		case 0x35: op_name="SyncCache "; break;
		case 0x42: op_name="(CD)ReadSubChannel "; break;
		case 0x43: op_name="(CD)ReadToc "; break;
		case 0x55: op_name="ModeSelect(10) "; break;
		default:
			sprintf(debug,"* T%dL%d : %d bytes [", ccb->cam_ch.cam_target_id, ccb->cam_ch.cam_target_lun,ccb->cam_cdb_len);
			sprintf(debug+strlen(debug),"# %X ",cdb[0]);
	}
	if (op_name) {
		sprintf(debug,"* T%dL%d : %d bytes [%s", ccb->cam_ch.cam_target_id, ccb->cam_ch.cam_target_lun,ccb->cam_cdb_len,op_name);
	}
/*	{ int t; for (t=1; t<ccb->cam_cdb_len; t++) {
		sprintf(debug,"%X ",cdb[t]);
		debug_print(debug);
	}}
*/
	if ((ccb->cam_ch.cam_flags & CAM_DIR_NONE) == CAM_DIR_NONE) { sprintf(debug+strlen(debug),"] Data dir NONE\n"); }
	else if (ccb->cam_ch.cam_flags & CAM_DIR_IN) { sprintf(debug+strlen(debug),"] Data IN : %ld\n",ccb->cam_dxfer_len); }
	else if (ccb->cam_ch.cam_flags & CAM_DIR_OUT){ sprintf(debug+strlen(debug),"] Data OUT: %ld\n",ccb->cam_dxfer_len); }

	debug_print(debug);
	
	if (!longformat) return;
	
	if (ccb->cam_sense_resid) {
		sprintf(debug,"cam_sense_resid %d bytes\n", ccb->cam_sense_resid);
		debug_print(debug);
	}
	
	if (ccb->cam_msgb_len) {
		debug_print("###  Num of bytes in the message buf : %d",ccb->cam_msgb_len);
	}
	switch(ccb->cam_ch.cam_flags) {
	case 0x40:
	case 0xC0:
	case 0x80c0:
	case 0x8040:
	case 0x8050:
	case 0x8080:
	case 0x8090:
		break;
	default:
		debug_print("# WARNING : cam flag value : %x",(unsigned)ccb->cam_ch.cam_flags);
	}
}

/*static*/ void display_interrupt(struct amd_softc *am, const char * when)
{
#ifdef AMDINITDEBUG
	int i;
	for (i=0; i<am->interrupt; i++) {
		if ((am->_ISREG[i] & 0xF)!=4)
			sprintf(debug,"cmd %Xh:(%s,%s) ISREG %xh ",am->_CMDREG[i],when,am->intr_when[i],am->_ISREG[i] & 0xF);
		else
			sprintf(debug,"cmd %Xh:(%s,%s) ",am->_CMDREG[i],when,am->intr_when[i]);
/*		
//		debug_print("*** INSTREG : ");
//		debug_hexa(am->_INSTREG[i]);
*/
		if (am->_semrel[i]) 
			strcat(debug," Sched -");
		if (am->_INSTREG[i] & INSTREG_SRST) 
			strcat(debug," # SCSI RESET -");
		if (am->_INSTREG[i] & INSTREG_ICMD) 
			strcat(debug," ### INVALID CMD -");
		if (am->_INSTREG[i] & INSTREG_DIS)
			strcat(debug," disc -");
		if (am->_INSTREG[i] & INSTREG_SR)
			strcat(debug," SR -");
		if (am->_INSTREG[i] & INSTREG_SO)
			strcat(debug," SO -");
		else
			strcat(debug," !so -");
		if (am->_INSTREG[i] & INSTREG_RESEL)
			strcat(debug," dev resel as init -");
		if (am->_INSTREG[i] & INSTREG_SEL)
			strcat(debug," # SEL -");
		if (am->_INSTREG[i] & INSTREG_SELWA)
			strcat(debug," # SELWA -");

		if (am->_STATREG[i] & STATREG_IOE) 
			strcat(debug," # ILLOP -");
		if (am->_STATREG[i] & STATREG_PE) 
			strcat(debug,"pe-");
		if (!(am->_STATREG[i] & COUNT_2_ZERO))
			strcat(debug,"#!C2Z-");
		if (am->_DMASTATUS[i] & DMASTATUS_BCMPLT)	strcat(debug,"*** : BLAST complete\n");
		if (am->_DMASTATUS[i] & DMASTATUS_DONE)		strcat(debug," DMA term");
		if (am->_DMASTATUS[i] & DMASTATUS_ABORT)	strcat(debug," DMA transfer aborted\n");
		if (am->_DMASTATUS[i] & DMASTATUS_ERROR)	strcat(debug," DMA transfer error\n");
		if (am->_DMASTATUS[i] & DMASTATUS_PWDN)		strcat(debug," power down indicator\n");

		switch(am->_STATREG[i] & 0x7) {
			case PHASE_MSGIN:	strcat(debug,"MsgIn"); break;
			case PHASE_MSGOUT:	strcat(debug,"MsgOut"); break;
			case PHASE_STATIN:	strcat(debug,"Sts"); break;
			case PHASE_DATAIN:	strcat(debug,"DtaIn"); break;
			case PHASE_DATAOUT:	strcat(debug,"DtaOut"); break;
			case PHASE_CMDOUT:	strcat(debug,"Cmd"); break;
			default:			strcat(debug,"### Unknown phase\n");
		}
		strcat(debug,"->");
		switch(am->bus_phase_out[i]) {
			case PHASE_MSGIN:	strcat(debug,"MsgIn"); break;
			case PHASE_MSGOUT:	strcat(debug,"MsgOut"); break;
			case PHASE_STATIN:	strcat(debug,"Sts"); break;
			case PHASE_DATAIN:	strcat(debug,"DtaIn"); break;
			case PHASE_DATAOUT:	strcat(debug,"DtaOut"); break;
			case PHASE_CMDOUT:	strcat(debug,"Cmd"); break;
			default:			strcat(debug,"### Unknown phase\n");
		}
		strcat(debug,"* FIFO : ");
		if (am->_CFIREG[i]>0) {
			int nb;
			char debug2[10];
			sprintf(debug2,"%d [", am->_CFIREG[i]);
			strcat(debug,debug2);
			// ### BAD !!!!
			if (am->nb_data>30) panic("FIFO buffer overflow");
			for (nb=0;nb<am->nb_data;nb++) {
				sprintf(debug2,"%X ",am->data[nb]);
				strcat(debug,debug2);
			}
			strcat(debug,"]\n");
		} else {
			strcat(debug,"0\n");
		}
		debug_print(debug);
	}
#endif
	am->interrupt=0;
}



