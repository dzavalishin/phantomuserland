/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Lance 111 driver.
 *
**/

#if 0

#define DEBUG_MSG_PREFIX "Lance111"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <hal.h>
#include <assert.h>
#include <sys/types.h>
#include <string.h>
#include <threads.h>
#include <compat/nutos.h>
#include <newos/err.h>

#include <kernel/ethernet_defs.h>
#include <net.h>

#include "lanc111.h"




#define INLINE
#define CONST const
#define NETBUF char







//static HANDLE maq;
static hal_cond_t maq;

/*!
 * \brief Select specified PHY register for reading or writing.
 *
 * \note NIC interrupts must have been disabled before calling this routine.
 *
 * \param reg PHY register number.
 * \param we  Indicates type of access, 1 for write and 0 for read.
 *
 * \return Contents of the PHY interface rgister.
 */
static uint8_t NicPhyRegSelect(uint8_t reg, uint8_t we)
{
    uint8_t rs;
    uint8_t msk;
    uint8_t i;

    nic_bs(3);
    rs = (nic_inlb(NIC_MGMT) & ~(MGMT_MCLK | MGMT_MDO)) | MGMT_MDOE;

    /* Send idle pattern. */
    for (i = 0; i < 33; i++) {
        nic_outlb(NIC_MGMT, rs | MGMT_MDO);
        nic_outlb(NIC_MGMT, rs | MGMT_MDO | MGMT_MCLK);
    }

    /* Send start sequence. */
    nic_outlb(NIC_MGMT, rs);
    nic_outlb(NIC_MGMT, rs | MGMT_MCLK);
    nic_outlb(NIC_MGMT, rs | MGMT_MDO);
    nic_outlb(NIC_MGMT, rs | MGMT_MDO | MGMT_MCLK);

    /* Write or read mode. */
    if (we) {
        nic_outlb(NIC_MGMT, rs);
        nic_outlb(NIC_MGMT, rs | MGMT_MCLK);
        nic_outlb(NIC_MGMT, rs | MGMT_MDO);
        nic_outlb(NIC_MGMT, rs | MGMT_MDO | MGMT_MCLK);
    } else {
        nic_outlb(NIC_MGMT, rs | MGMT_MDO);
        nic_outlb(NIC_MGMT, rs | MGMT_MDO | MGMT_MCLK);
        nic_outlb(NIC_MGMT, rs);
        nic_outlb(NIC_MGMT, rs | MGMT_MCLK);
    }

    /* Send PHY address. Zero is used for the internal PHY. */
    for (i = 0; i < 5; i++) {
        nic_outlb(NIC_MGMT, rs);
        nic_outlb(NIC_MGMT, rs | MGMT_MCLK);
    }

    /* Send PHY register number. */
    for (msk = 0x10; msk; msk >>= 1) {
        if (reg & msk) {
            nic_outlb(NIC_MGMT, rs | MGMT_MDO);
            nic_outlb(NIC_MGMT, rs | MGMT_MDO | MGMT_MCLK);
        } else {
            nic_outlb(NIC_MGMT, rs);
            nic_outlb(NIC_MGMT, rs | MGMT_MCLK);
        }
    }
    nic_outlb(NIC_MGMT, rs);

    return rs;
}

/*!
 * \brief Read contents of PHY register.
 *
 * \note NIC interrupts must have been disabled before calling this routine.
 *
 * \param reg PHY register number.
 *
 * \return Contents of the specified register.
 */
static uint16_t NicPhyRead(uint8_t reg)
{
    uint16_t rc = 0;
    uint8_t rs;
    uint8_t i;

    /* Select register for reading. */
    rs = NicPhyRegSelect(reg, 0);

    /* Switch data direction. */
    rs &= ~MGMT_MDOE;
    nic_outlb(NIC_MGMT, rs);
    nic_outlb(NIC_MGMT, rs | MGMT_MCLK);

    /* Clock data in. */
    for (i = 0; i < 16; i++) {
        nic_outlb(NIC_MGMT, rs);
        nic_outlb(NIC_MGMT, rs | MGMT_MCLK);
        rc <<= 1;
        rc |= (nic_inlb(NIC_MGMT) & MGMT_MDI) != 0;
    }

    /* This will set the clock line to low. */
    nic_outlb(NIC_MGMT, rs);

    return rc;
}

/*!
 * \brief Write value to PHY register.
 *
 * \note NIC interrupts must have been disabled before calling this routine.
 *
 * \param reg PHY register number.
 * \param val Value to write.
 */
static void NicPhyWrite(uint8_t reg, uint16_t val)
{
    uint16_t msk;
    uint8_t rs;

    /* Select register for writing. */
    rs = NicPhyRegSelect(reg, 1);

    /* Switch data direction dummy. */
    nic_outlb(NIC_MGMT, rs | MGMT_MDO);
    nic_outlb(NIC_MGMT, rs | MGMT_MDO | MGMT_MCLK);
    nic_outlb(NIC_MGMT, rs);
    nic_outlb(NIC_MGMT, rs | MGMT_MCLK);

    /* Clock data out. */
    for (msk = 0x8000; msk; msk >>= 1) {
        if (val & msk) {
            nic_outlb(NIC_MGMT, rs | MGMT_MDO);
            nic_outlb(NIC_MGMT, rs | MGMT_MDO | MGMT_MCLK);
        } else {
            nic_outlb(NIC_MGMT, rs);
            nic_outlb(NIC_MGMT, rs | MGMT_MCLK);
        }
    }

    /* Set clock line low and output line int z-state. */
    nic_outlb(NIC_MGMT, rs & ~MGMT_MDOE);
}

/*!
 * \brief Configure the internal PHY.
 *
 * Reset the PHY and initiate auto-negotiation.
 */
static int NicPhyConfig(void)
{
    uint16_t phy_sor;
    uint16_t phy_sr;
    uint16_t phy_to;
    uint16_t mode;

    /* 
     * Reset the PHY and wait until this self clearing bit
     * becomes zero. We sleep 63 ms before each poll and
     * give up after 3 retries. 
     */
    //printf("Reset PHY..");
    NicPhyWrite(NIC_PHYCR, PHYCR_RST);
    for (phy_to = 0;; phy_to++) {
        NutSleep(63);
        if ((NicPhyRead(NIC_PHYCR) & PHYCR_RST) == 0)
            break;
        if (phy_to > 3)
            return -1;
    }
    //printf("OK\n");

    /* Store PHY status output. */
    phy_sor = NicPhyRead(NIC_PHYSOR);

    /* Enable PHY interrupts. */
    NicPhyWrite(NIC_PHYMSK, PHYMSK_MLOSSSYN | PHYMSK_MCWRD | PHYMSK_MSSD |
                PHYMSK_MESD | PHYMSK_MRPOL | PHYMSK_MJAB | PHYMSK_MSPDDT | PHYMSK_MDPLDT);

    /* Set RPC register. */
    mode = RPCR_ANEG | RPCR_LEDA_PAT | RPCR_LEDB_PAT;
    nic_bs(0);
    nic_outw(NIC_RPCR, mode);

#ifdef NIC_FIXED
    /* Disable link. */
    phy_sr = NicPhyRead(NIC_PHYCFR1);
    NicPhyWrite(NIC_PHYCFR1, phy_sr | 0x8000);
    NutSleep(63);

    /* Set fixed capabilities. */
    NicPhyWrite(NIC_PHYCR, NIC_FIXED);
    nic_bs(0);
    nic_outw(NIC_RPCR, mode);

    /* Enable link. */
    phy_sr = NicPhyRead(NIC_PHYCFR1);
    NicPhyWrite(NIC_PHYCFR1, phy_sr & ~0x8000);
    phy_sr = NicPhyRead(NIC_PHYCFR1);

#else
    /*
     * Advertise our capabilities, initiate auto negotiation
     * and wait until this has been completed.
     */
    //printf("Negotiate..");
    NicPhyWrite(NIC_PHYANAD, PHYANAD_TX_FDX | PHYANAD_TX_HDX | PHYANAD_10FDX | PHYANAD_10_HDX | PHYANAD_CSMA);
    NutSleep(63);
    for (phy_to = 0, phy_sr = 0;; phy_to++) {
        /* Give up after 10 seconds. */
        if (phy_to >= 1024)
            return -1;
        /* Restart auto negotiation every 4 seconds or on failures. */
        if ((phy_to & 127) == 0 /* || (phy_sr & PHYSR_REM_FLT) != 0 */ ) {
            NicPhyWrite(NIC_PHYCR, PHYCR_ANEG_EN | PHYCR_ANEG_RST);
            //printf("Restart..");
            NutSleep(63);
        }
        /* Check if we are done. */
        phy_sr = NicPhyRead(NIC_PHYSR);
        //printf("[SR %04X]", phy_sr);
        if (phy_sr & PHYSR_ANEG_ACK)
            break;
        NutSleep(63);
    }
    //printf("OK\n");
#endif

    return 0;
}

/*!
 * \brief Wait until MMU is ready.
 *
 * Poll the MMU command register until \ref MMUCR_BUSY
 * is cleared.
 *
 * \param tmo Timeout in milliseconds.
 *
 * \return 0 on success or -1 on timeout.
 */
static INLINE int NicMmuWait(uint16_t tmo)
{
    while (tmo--) {
        if ((nic_inlb(NIC_MMUCR) & MMUCR_BUSY) == 0)
            break;
        NutDelay(1);
    }
    return tmo ? 0 : -1;
}

/*!
 * \brief Reset the Ethernet controller.
 *
 * \return 0 on success, -1 otherwise.
 */
static int NicReset(void)
{
#ifdef LANC111_RESET_BIT
    sbi(LANC111_RESET_DDR, LANC111_RESET_BIT);
    sbi(LANC111_RESET_PORT, LANC111_RESET_BIT);
    NutDelay(WAIT100);
    cbi(LANC111_RESET_PORT, LANC111_RESET_BIT);
    NutDelay(WAIT250);
    NutDelay(WAIT250);
#endif

    /* Disable all interrupts. */
    nic_outlb(NIC_MSK, 0);

    /* MAC and PHY software reset. */
    nic_bs(0);
    nic_outw(NIC_RCR, RCR_SOFT_RST);

    /* Enable Ethernet protocol handler. */
    nic_bs(1);
    nic_outw(NIC_CR, CR_EPH_EN);

    NutDelay(10);

    /* Disable transmit and receive. */
    nic_bs(0);
    nic_outw(NIC_RCR, 0);
    nic_outw(NIC_TCR, 0);

    /* Enable auto release. */
    nic_bs(1);
    nic_outw(NIC_CTR, CTR_AUTO_RELEASE);

    /* Reset MMU. */
    nic_bs(2);
    nic_outlb(NIC_MMUCR, MMU_RST);
    if (NicMmuWait(1000))
        return -1;

    return 0;
}

/*
 * Fires up the network interface. NIC interrupts
 * should have been disabled when calling this
 * function.
 *
 * \param mac Six byte unique MAC address.
 */
static int NicStart(CONST uint8_t * mac)
{
    uint8_t i;

    if (NicReset())
        return -1;

    /* Enable receiver. */
    nic_bs(3);
    nic_outlb(NIC_ERCV, 7);
    nic_bs(0);
    nic_outw(NIC_RCR, RCR_RXEN);

    /* Enable transmitter and padding. */
    nic_outw(NIC_TCR, TCR_PAD_EN | TCR_TXENA);

    /* Configure the PHY. */
    if (NicPhyConfig())
        return -1;

    /* Set MAC address. */
    //printf("Set MAC %02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    nic_bs(1);
    for (i = 0; i < 6; i++)
        nic_outlb(NIC_IAR + i, mac[i]);
    //printf("OK\n");

    /* Enable interrupts. */
    nic_bs(2);
    nic_outlb(NIC_MSK, INT_ERCV | INT_RCV | INT_RX_OVRN);

    return 0;
}

/*
 * NIC interrupt entry.
 */
static void NicInterrupt(void *arg)
{
    uint8_t isr;
    uint8_t imr;
    lanc111_nic_t *ni = (lanc111_nic_t *) ((NUTDEVICE *) arg)->dev_dcb;


    ni->ni_interrupts++;

    /* Read the interrupt mask and disable all interrupts. */
    nic_bs(2);
    imr = nic_inlb(NIC_MSK);
    nic_outlb(NIC_MSK, 0);

    /* Read the interrupt status and acknowledge all interrupts. */
    isr = nic_inlb(NIC_IST);
    //printf("\n!%02X-%02X ", isr, imr);
    isr &= imr;

    /*
     * If this is a transmit interrupt, then a packet has been sent. 
     * So we can clear the transmitter busy flag and wake up the 
     * transmitter thread.
     */
    if (isr & INT_TX_EMPTY) {
        nic_outlb(NIC_ACK, INT_TX_EMPTY);
        imr &= ~INT_TX_EMPTY;
    }
    /* Transmit error. */
    else if (isr & INT_TX) {
        /* re-enable transmit */
        nic_bs(0);
        nic_outw(NIC_TCR, nic_inlb(NIC_TCR) | TCR_TXENA);
        nic_bs(2);
        nic_outlb(NIC_ACK, INT_TX);
        /* kill the packet */
        nic_outlb(NIC_MMUCR, MMU_PKT);
    }


    /*
     * If this is a receive interrupt, then wake up the receiver 
     * thread.
     */
    if (isr & INT_RX_OVRN) {
        nic_outlb(NIC_ACK, INT_RX_OVRN);
        //nic_outlb(NIC_MMUCR, MMU_TOP);
        NutEventPostFromIrq(&ni->ni_rx_rdy);
    }
    if (isr & INT_ERCV) {
        nic_outlb(NIC_ACK, INT_ERCV);
        NutEventPostFromIrq(&ni->ni_rx_rdy);
    }
    if (isr & INT_RCV) {
        nic_outlb(NIC_ACK, INT_RCV);
        imr &= ~INT_RCV;
        NutEventPostFromIrq(&ni->ni_rx_rdy);
    }

    if (isr & INT_ALLOC) {
        imr &= ~INT_ALLOC;
        NutEventPostFromIrq(&maq);
    }
    //printf(" -%02X-%02X- ", nic_inlb(NIC_IST), inb(PINE) & 0x20);
    nic_outlb(NIC_MSK, imr);
}

/*
 * Write data block to the NIC.
 */
static void NicWrite(uint8_t * buf, uint16_t len)
{
    register uint16_t l = len - 1;
    register uint8_t ih = (uint16_t) l >> 8;
    register uint8_t il = (uint8_t) l;

    if (!len)
        return;

    do {
        do {
            nic_outlb(NIC_DATA, *buf++);
        } while (il-- != 0);
    } while (ih-- != 0);
}

/*
 * Read data block from the NIC.
 */
static void NicRead(uint8_t * buf, uint16_t len)
{
    register uint16_t l = len - 1;
    register uint8_t ih = (uint16_t) l >> 8;
    register uint8_t il = (uint8_t) l;

    if (!len)
        return;

    do {
        do {
            *buf++ = nic_inlb(NIC_DATA);
        } while (il-- != 0);
    } while (ih-- != 0);
}

/*!
 * \brief Fetch the next packet out of the receive ring buffer.
 *
 * Nic interrupts must be disabled when calling this funtion.
 *
 */
static int NicGetPacket( struct phantom_device *dev, void *buf, int len)
{
    NETBUF *nb = 0;
    //uint8_t *buf;
    uint16_t f_status_word;
    uint16_t fbytecount;

    /* Check the fifo empty bit. If it is set, then there is 
       nothing in the receiver fifo. */
    nic_bs(2);
    //if (nic_inw(NIC_FIFO) & 0x8000)
    //    return 0;

    // TODO cond/sem timed wait
    while(nic_inw(NIC_FIFO) & 0x8000)
        hal_sleep_msec(100);

    /* Inialize pointer register. */
    nic_outw(NIC_PTR, PTR_READ | PTR_RCV | PTR_AUTO_INCR);
    _NOP();
    _NOP();
    _NOP();
    _NOP();

    /* Read status word and byte count. */
    f_status_word = nic_inw(NIC_DATA);
    fbytecount = nic_inw(NIC_DATA);
    //printf("[SW=%04X,BC=%04X]", f_status_word, fbytecount);


    int ret = ERR_IO_ERROR;

    /* Check for frame errors. */
    if (f_status_word & 0xAC00) {
        //nb = (NETBUF *) 0xFFFF;
        ret = ERR_IO_ERROR;
    }
    /* Check the byte count. */
    else if (fbytecount < 66 || fbytecount > 1524) {
        //nb = (NETBUF *) 0xFFFF;
        ret = ERR_IO_ERROR;
    }
    else {
        /* 
         * Allocate a NETBUF. 
         * Hack alert: Rev A chips never set the odd frame indicator.
         */
        fbytecount -= 3;

        if( len < fbytecont )
            ret = ERR_VFS_INSUFFICIENT_BUF;
        else
        {
            NicRead( buf, fbytecount);
            ret = fbytecount;
        }
    }

    /* Release the packet. */
    nic_outlb(NIC_MMUCR, MMU_TOP);

    return ret;
}

/*!
 * \brief Load a packet into the nic's transmit ring buffer.
 *
 * Interupts must have been disabled when calling this function.
 *
 * \param nb Network buffer structure containing the packet to be sent.
 *           The structure must have been allocated by a previous
 *           call NutNetBufAlloc(). This routine will automatically
 *           release the buffer in case of an error.
 *
 * \return 0 on success, -1 in case of any errors. Errors
 *         will automatically release the network buffer 
 *         structure.
 */
static int NicPutPacket(NETBUF * nb)
{
    uint16_t sz;
    uint8_t odd = 0;
    uint8_t imsk;

    //printf("[P]");
    /*
     * Calculate the number of bytes to be send. Do not send packets 
     * larger than the Ethernet maximum transfer unit. The MTU
     * consist of 1500 data bytes plus the 14 byte Ethernet header
     * plus 4 bytes CRC. We check the data bytes only.
     */
    if ((sz = nb->nb_nw.sz + nb->nb_tp.sz + nb->nb_ap.sz) > ETH_DATA_LEN)
        return -1;

    /* Disable all interrupts. */
    imsk = nic_inlb(NIC_MSK);
    nic_outlb(NIC_MSK, 0);

    /* Allocate packet buffer space. */
    nic_bs(2);
    nic_outlb(NIC_MMUCR, MMU_ALO);
    if (NicMmuWait(100))
        return -1;

    /* Enable interrupts including allocation success. */
    nic_outlb(NIC_MSK, imsk | INT_ALLOC);

    /* The MMU needs some time. Use it to calculate the byte count. */
    sz += nb->nb_dl.sz;
    sz += 6;
    if (sz & 1) {
        sz++;
        odd++;
    }

    /* Wait for allocation success. */
    while ((nic_inlb(NIC_IST) & INT_ALLOC) == 0) {
        if (NutEventWait(&maq, 125)) {
            nic_outlb(NIC_MMUCR, MMU_RST);
            NicMmuWait(1000);
            nic_outlb(NIC_MMUCR, MMU_ALO);
            if (NicMmuWait(100) || (nic_inlb(NIC_IST) & INT_ALLOC) == 0) {
                if (NutEventWait(&maq, 125)) {
                    return -1;
                }
            }
        }
    }

    /* Disable interrupts. */
    imsk = nic_inlb(NIC_MSK);
    nic_outlb(NIC_MSK, 0);


    nic_outlb(NIC_PNR, nic_inhb(NIC_PNR));

    nic_outw(NIC_PTR, 0x4000);

    /* Transfer control word. */
    nic_outlb(NIC_DATA, 0);
    nic_outlb(NIC_DATA, 0);

    /* Transfer the byte count. */
    nic_outw(NIC_DATA, sz);

    /* Transfer the Ethernet frame. */
    NicWrite(nb->nb_dl.vp, nb->nb_dl.sz);
    NicWrite(nb->nb_nw.vp, nb->nb_nw.sz);
    NicWrite(nb->nb_tp.vp, nb->nb_tp.sz);
    NicWrite(nb->nb_ap.vp, nb->nb_ap.sz);

    if (odd)
        nic_outlb(NIC_DATA, 0);

    /* Transfer the control word. */
    nic_outw(NIC_DATA, 0);

    /* Enqueue packet. */
    if (NicMmuWait(100))
        return -1;
    nic_outlb(NIC_MMUCR, MMU_ENQ);

    /* Enable interrupts. */
    imsk |= INT_TX | INT_TX_EMPTY;
    nic_outlb(NIC_MSK, imsk);

    return 0;
}


/*! \fn NicRxLanc(void *arg)
 * \brief NIC receiver thread.
 *
 */
THREAD(NicRxLanc, arg)
{
    NUTDEVICE *dev;
    IFNET *ifn;
    lanc111_nic_t *ni;
    NETBUF *nb;
    uint8_t imsk;

    dev = arg;
    ifn = (IFNET *) dev->dev_icb;
    ni = (lanc111_nic_t *) dev->dev_dcb;

    /*
     * This is a temporary hack. Due to a change in initialization,
     * we may not have got a MAC address yet. Wait until a valid one
     * has been set.
     */
    while (!ETHER_IS_UNICAST(ifn->if_mac)) {
        NutSleep(10);
    }

    /*
     * Do not continue unless we managed to start the NIC. We are
     * trapped here if the Ethernet link cannot be established.
     * This happens, for example, if no Ethernet cable is plugged
     * in.
     */
    while(NicStart(ifn->if_mac)) {
        NutSleep(1000);
    }

    //LANC111_SIGNAL_MODE();

    // Enable IRQs. Enabled on irq allocation.
    //sbi(EIMSK, LANC111_SIGNAL_IRQ);

    NutEventPost(&nic->ni_tx_rdy);

    /* Run at high priority. */
    NutThreadSetPriority(9);

    for (;;) {

        /*
         * Wait for the arrival of new packets or
         * check the receiver every two second.
         */
        NutEventWait(&ni->ni_rx_rdy, 2000);

        /*
         * Fetch all packets from the NIC's internal
         * buffer and pass them to the registered handler.
         */
        imsk = nic_inlb(NIC_MSK);
        nic_outlb(NIC_MSK, 0);
        while ((nb = NicGetPacket()) != 0) {
            if (nb != (NETBUF *) 0xFFFF) {
                ni->ni_rx_packets++;
                (*ifn->if_recv) (dev, nb);
            }
        }
        nic_outlb(NIC_MSK, imsk | INT_RCV | INT_ERCV);
    }
}

/*!
 * \brief Send Ethernet packet.
 *
 * \param dev   Identifies the device to use.
 * \param nb    Network buffer structure containing the packet to be sent.
 *              The structure must have been allocated by a previous
 *              call NutNetBufAlloc().
 *
 * \return 0 on success, -1 in case of any errors.
 */
int LancOutput(NUTDEVICE * dev, NETBUF * nb)
{
    static uint32_t mx_wait = 5000;
    int rc = -1;
    lanc111_nic_t *ni;

    /*
     * After initialization we are waiting for a long time to give
     * the PHY a chance to establish an Ethernet link.
     */
    if (NutEventWait(&nic->ni_tx_rdy, mx_wait) == 0) {
        ni = (lanc111_nic_t *) dev->dev_dcb;

        if (NicPutPacket(nb) == 0) {
            ni->ni_tx_packets++;
            rc = 0;
            /* Ethernet works. Set a long waiting time in case we
               temporarly lose the link next time. */
            mx_wait = 5000;
        }
        NutEventPost(&nic->ni_tx_rdy);
    }
    /*
     * Probably no Ethernet link. Significantly reduce the waiting
     * time, so following transmission will soon return an error.
     */
    else {
        mx_wait = 500;
    }
    return rc;
}

/*!
 * \brief Initialize Ethernet hardware.
 *
 * Resets the LAN91C111 Ethernet controller, initializes all required 
 * hardware registers and starts a background thread for incoming 
 * Ethernet traffic.
 *
 * Applications should do not directly call this function. It is 
 * automatically executed during during device registration by 
 * NutRegisterDevice().
 *
 * If the network configuration hasn't been set by the application
 * before registering the specified device, this function will
 * call NutNetLoadConfig() to get the MAC address.
 *
 * \param dev Identifies the device to initialize.
 */
static int LancInit(phantom_device_t * dev)
{
    lanc111_nic_t *nic = dev->drv_private;
    /* Disable NIC interrupt and clear lanc111_nic_t structure. */
    //cbi(EIMSK, LANC111_SIGNAL_IRQ);
    //memset(dev->dev_dcb, 0, sizeof(lanc111_nic_t));


    assert( 0 == hal_cond_init( &nic->ni_rx_rdy, DEBUG_MSG_PREFIX ) );

    assert( 0 == hal_cond_init( &maq, DEBUG_MSG_PREFIX ".MAQ" ) );
    assert( 0 == hal_cond_init( &nic->ni_tx_rdy, DEBUG_MSG_PREFIX ) );


    /* Register interrupt handler and enable interrupts. */
    //if (NutRegisterIrqHandler(&LANC111_SIGNAL, NicInterrupt, dev))        return -1;

    /*
     * Start the receiver thread.
     *
     * avr-gcc size optimized code used 76 bytes.
     */
    NutThreadCreate("rxi5", NicRxLanc, dev, (NUT_THREAD_LANCRXSTACK * NUT_THREAD_STACK_MULT) + NUT_THREAD_STACK_ADD);

    //NutSleep(500);

    return 0;
}
















#define WIRED_ADDRESS 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 134)
#define WIRED_NETMASK 	0xffffff00
#define WIRED_BROADCAST IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0xFF)

#define WIRED_NET 	IPV4_DOTADDR_TO_ADDR(10, 0, 2, 0)
#define WIRED_ROUTER    WIRED_ADDRESS

#define DEF_ROUTE_ROUTER        IPV4_DOTADDR_TO_ADDR(10, 0, 2, 2)




static int seq_number = 0;


phantom_device_t * driver_lanc111_probe( int port, int irq, int stage )
{
    (void) stage;



    lanc111_nic_t *nic = NULL;

    SHOW_INFO0( 0, "probe");
    nic = calloc( 1, sizeof(lanc111_nic_t) );

    assert(nic != NULL);



    //pcnet32_stop(nic);
    hal_sleep_msec(10);

    if(lanc111_init(nic) < 0)
    {
        //if(DEBUG)
        SHOW_ERROR0( 0, "init failed");
        goto free1;
    }

    //pcnet32_start(nic);


    phantom_device_t * dev = calloc(1, sizeof(phantom_device_t));
    dev->name = "lanc111";
    dev->seq_number = seq_number++;
    dev->drv_private = nic;

    //NutEtherInput,              /*!< \brief Routine to pass received data to, if_recv(). */
    //LancOutput,                 /*!< \brief Driver output routine, if_send(). */
    //NutEtherOutput              /*!< \brief Media output routine, if_output(). */

    //dev->dops.read = pcnet32_read;
    //dev->dops.write = pcnet32_write;
    //dev->dops.get_address = pcnet32_get_address;

    dev->iobase = port;
    dev->irq = irq;
    //dev->iomem = ;
    //dev->iomemsize = ;


    if( hal_irq_alloc( irq, &NicInterrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", irq );
        goto free2;
    }


    ifnet *interface;
    if( if_register_interface( IF_TYPE_ETHERNET, &interface, dev) )
    {
        SHOW_ERROR( 0, "Failed to register interface for %s", dev->name );
    }
    else
    {
        if_simple_setup(interface, WIRED_ADDRESS, WIRED_NETMASK, WIRED_BROADCAST, WIRED_NET, WIRED_ROUTER, DEF_ROUTE_ROUTER );
    }


    return dev;

free2:
    free( dev );
free1:
    free( nic );

    return 0;
}










#endif



