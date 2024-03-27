/**
  ******************************************************************************
  * @file    enc28j60.c
  * @author  Christian Schoffit, portions from Gregory Nutt:
  *          Copyright (C) 2010-2012, 2014 Gregory Nutt. All rights reserved.
  *          Author: Gregory Nutt <gnutt@nuttx.org>
  *
  * @version V1.0.0
  * @date    02-June-2015
  * @brief   This file provides a set of functions needed to manage the ENC28J60
  *          Stand-Alone Ethernet Controller with SPI Interface.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 Christian Schoffit</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of Christian Schoffit nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/*
Module         Feature        Issue Issue Summary                                             Affected Revisions
                                                                                              B1 B4 B5 B7
MAC Interface  -              1.    MAC registers unreliable with slow asynchronous SPI clock X  X
Reset          -              2.    CLKRDY set early                                          X  X  X  X
Core           Operating      3.    Industrial (-40캜 to +85캜) temperature range unsupported X  X
               Specifications
Oscillator     CLKOUT pin     4.    CLKOUT unavailable in Power Save mode                     X  X  X  X
Memory         Ethernet       5.    Receive buffer must start at 0000h                        X  X  X  X
               Buffer
Interrupts     -              6.    Receive Packet Pending Interrupt Flag (PKTIF) unreliable  X  X  X  X
PHY            -              7.    TPIN+/- automatic polarity detection and correction
                                    unreliable                                                X  X  X  X
PHY            -              8.    RBIAS resistor value differs between silicon revisions    X  X
PHY            -              9.    Internal loopback in half-duplex unreliable               X  X  X  X
PHY            -              10.   Internal loopback in full-duplex unreliable               X  X  X  X
PHY LEDs       -              11.   Combined Collision and Duplex Status mode unavailable     X  X  X  X
Transmit       -              12.   Transmit abort may stall transmit logic                   X  X  X  X
Logic
PHY            -              13.   Received link pulses potentially cause collisions               X  X
Memory         Ethernet       14.   Even values in ERXRDPT may corrupt receive buffer         X  X  X  X
               Buffer
Transmit       -              15.   LATECOL Status bit unreliable                             X  X  X  X
Logic
PHY LEDs       -              16.   LED auto-polarity detection unreliable                    X  X  X  X
DMA            -              17.   DMA checksum calculations will abort receive packets      X  X  X  X
Receive        -              18.   Pattern match filter allows reception of extra packets    X  X  X  X
Filter
SPI            -              19.   Reset command unavailable in Power Save mode              X  X  X  X
Interface

Only workaround relative to issues affecting B7 silicon revision are implemented. Therefore, issues
specific to Ethernet conformance are not addressed, since they only affect B1 and B3 silicon revisions.

Erratas 7, 8, 16... have workaround implemented by hardware

Errata 18 is implemented in lwip stack
*/

/* Includes ------------------------------------------------------------------*/
#include "enc28j60.h"
#include "../include/fb.h"
#include "../include/io.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup Components
  * @{
  */

/** @defgroup ENC28J60
  * @{
  */

/** @defgroup ENC28J60_Private_Types_Definitions
  * @{
  */

/** @defgroup ENC28J60_Private_Defines
  * @{
  */

/* Poll timeout */

#define ENC_POLLTIMEOUT 50

/**
  * @}
  */

/** @defgroup ENC28J60_Private_Macros
  * @{
  */

/* Packet Memory ************************************************************/

/* Packet memory layout */

#define ALIGNED_BUFSIZE ((CONFIG_NET_ETH_MTU + 255) & ~255)

/* Work around Errata #5 (spurious reset of ERXWRPT to 0) by placing the RX
 * FIFO at the beginning of packet memory.
 */

#  define PKTMEM_RX_START 0x0000                            /* RX buffer must be at addr 0 for errata 5 */
#  define PKTMEM_RX_END   (PKTMEM_END-ALIGNED_BUFSIZE)      /* RX buffer length is total SRAM minus TX buffer */
#  define PKTMEM_TX_START (PKTMEM_RX_END+1)                 /* Start TX buffer after */
#  define PKTMEM_TX_ENDP1 (PKTMEM_TX_START+ALIGNED_BUFSIZE) /* Allow TX buffer for two frames */

/* Misc. Helper Macros ******************************************************/

#define enc_rdgreg(ctrlreg) \
  enc_rdgreg2(ENC_RCR | GETADDR(ctrlreg))
#define enc_wrgreg(ctrlreg, wrdata) \
  enc_wrgreg2(ENC_WCR | GETADDR(ctrlreg), wrdata)
#define enc_bfcgreg(ctrlreg,clrbits) \
  enc_wrgreg2(ENC_BFC | GETADDR(ctrlreg), clrbits)
#define enc_bfsgreg(ctrlreg,setbits) \
  enc_wrgreg2(ENC_BFS | GETADDR(ctrlreg), setbits)

/**
  * @}
  */

/** @defgroup ENC28J60_Private_Variables
  * @{
  */

  /* Stores how many iterations the microcontroller can do in 1 탎 */
static uint32_t iter_per_us=0;

/**
  * @}
  */

/** @defgroup ENC28J60_Private_Function_Prototypes
  * @{
  */
/**
  * @}
  */

/** @defgroup ENC28J60_Private_Functions
  * @{
  */

/**
   Calibrate the constant time
 **/

static void calibrate(void)
{
    uint32_t time;
    volatile uint32_t i;

    iter_per_us = 1000000;

    time = HAL_GetTick();
    /* Wait for next tick */
    while (HAL_GetTick() == time) {
        /* wait */
    }
    for (i=0; i<iter_per_us; i++) {
    }
    iter_per_us /= ((HAL_GetTick()-time)*1000);
}

/**
 * Software delay in 탎
 *  us: the number of 탎 to wait
 **/
void up_udelay(uint32_t us)
{
    volatile uint32_t i;

    for (i=0; i<us*iter_per_us; i++) {
    }
}
/****************************************************************************
 * Function: enc_rdgreg2
 *
 * Description:
 *   Read a global register (EIE, EIR, ESTAT, ECON2, or ECON1).  The cmd
 *   include the CMD 'OR'd with the global address register.
 *
 * Parameters:
 *   cmd   - The full command to received (cmd | address)
 *
 * Returned Value:
 *   The value read from the register
 *
 * Assumptions:
 *
 ****************************************************************************/

static uint8_t enc_rdgreg2(uint8_t cmd)
{
    uint8_t cmdpdata[2];
    cmdpdata[0] = cmd;

  /* Send the read command and collect the data.  The sequence requires
   * 16-clocks:  8 to clock out the cmd + 8 to clock in the data.
   */

  ENC_SPI_SendBuf(cmdpdata, cmdpdata, 2);

  return cmdpdata[1];
}


/****************************************************************************
 * Function: enc_wrgreg2
 *
 * Description:
 *   Write to a global register (EIE, EIR, ESTAT, ECON2, or ECON1).  The cmd
 *   include the CMD 'OR'd with the global address register.
 *
 * Parameters:
 *   cmd    - The full command to received (cmd | address)
 *   wrdata - The data to send
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void enc_wrgreg2(uint8_t cmd, uint8_t wrdata)
{
    uint8_t cmdpdata[2];
    cmdpdata[0] = cmd;
    cmdpdata[1] = wrdata;


    /* Send the write command and data.  The sequence requires 16-clocks:
     * 8 to clock out the cmd + 8 to clock out the data.
     */

    ENC_SPI_SendBuf(cmdpdata, NULL, 2);
}


/****************************************************************************
 * Function: enc_waitgreg
 *
 * Description:
 *   Wait until grouped register bit(s) take a specific value (or a timeout
 *   occurs).
 *
 * Parameters:
 *   ctrlreg - Bit encoded address of banked register to check
 *   bits    - The bits to check (a mask)
 *   value   - The value of the bits to return (value under mask)
 *
 * Returned Value:
 *   OK on success, negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

static bool enc_waitgreg(uint8_t ctrlreg,
                        uint8_t bits, uint8_t value)
{
  uint32_t start = HAL_GetTick();
  uint32_t elapsed;
  uint8_t  rddata;

  /* Loop until the exit condition is met */

  do
    {
      /* Read the byte from the requested banked register */

      rddata  = enc_rdgreg(ctrlreg);
      elapsed = HAL_GetTick() - start;
    }
  while ((rddata & bits) != value && elapsed < ENC_POLLTIMEOUT);

  return (rddata & bits) == value;
}

/****************************************************************************
 * Function: enc_waitwhilegreg
 *
 * Description:
 *   Wait while grouped register bit(s) have a specific value (or a timeout
 *   occurs).
 *
 * Parameters:
 *   ctrlreg - Bit encoded address of banked register to check
 *   bits    - The bits to check (a mask)
 *   value   - The value of the bits to return (value under mask)
 *
 * Returned Value:
 *   OK on success, negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifndef USE_PROTOTHREADS
static bool enc_waitwhilegreg(uint8_t ctrlreg,
                        uint8_t bits, uint8_t value)
{
  uint32_t start = HAL_GetTick();
  uint32_t elapsed;
  uint8_t  rddata;

  /* Loop until the exit condition is met */

  do
    {
      /* Read the byte from the requested banked register */

      rddata  = enc_rdgreg(ctrlreg);
      elapsed = HAL_GetTick() - start;
    }
  while ((rddata & bits) == value && elapsed < ENC_POLLTIMEOUT);

  return (rddata & bits) != value;
}
#endif

/**
  * @brief  Perform a soft reset on enc28j60
  * Description:
  *   Send the single byte system reset command (SRC).
  *
  *   "The System Reset Command (SRC) allows the host controller to issue a
  *    System Soft Reset command.  Unlike other SPI commands, the SRC is
  *    only a single byte command and does not operate on any register. The
  *    command is started by pulling the CS pin low. The SRC opcode is the
  *    sent, followed by a 5-bit Soft Reset command constant of 1Fh. The
  *    SRC operation is terminated by raising the CS pin."
  *
  * @param  None
  * @retval None
  */
void enc_reset(ENC_HandleTypeDef *handle) {

  /* Send the system reset command. */
  ENC_SPI_Send(ENC_SRC);

  /* Check CLKRDY bit to see when the reset is complete.  There is an errata
   * that says the CLKRDY may be invalid.  We'll wait a couple of msec to
   * workaround this condition.
   *
   * Also, "After a System Reset, all PHY registers should not be read or
   * written to until at least 50 탎 have passed since the Reset has ended.
   * All registers will revert to their Reset default values. The dual
   * port buffer memory will maintain state throughout the System Reset."
   */

  handle->bank = 0; /* Initialize the trace on the current selected bank */
  //up_mdelay(2);
  HAL_Delay(2); /* >1000 탎, conforms to errata #2 */
}

/****************************************************************************
 * Function: enc_setbank
 *
 * Description:
 *   Set the bank for these next control register access.
 *
 * Assumption:
 *   The caller has exclusive access to the SPI bus
 *
 * Parameters:
 *   handle - Reference to the driver state structure
 *   bank   - The bank to select (0-3)
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

void enc_setbank(ENC_HandleTypeDef *handle, uint8_t bank) {

  if (bank != handle->bank) {
      /* Select bank 0 (just so that all of the bits are cleared) */

      enc_bfcgreg(ENC_ECON1, ECON1_BSEL_MASK);

      /* Then OR in bits to get the correct bank */

      if (bank != 0)
        {
          enc_bfsgreg(ENC_ECON1, (bank << ECON1_BSEL_SHIFT));
        }

      /* Then remember the bank setting */

      handle->bank = bank;
    }
}

/****************************************************************************
 * Function: enc_rdbreg
 *
 * Description:
 *   Read from a banked control register using the RCR command.
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *   ctrlreg - Bit encoded address of banked register to read
 *
 * Returned Value:
 *   The byte read from the banked register
 *
 * Assumptions:
 *
 ****************************************************************************/

static uint8_t enc_rdbreg(ENC_HandleTypeDef *handle, uint8_t ctrlreg)
{
  uint8_t data[3];

  /* Set the bank */

  enc_setbank(handle, GETBANK(ctrlreg));

  /* Send the RCR command and collect the data.  How we collect the data
   * depends on if this is a PHY/CAN or not.  The normal sequence requires
   * 16-clocks:  8 to clock out the cmd and  8 to clock in the data.
   */

  data[0] = ENC_RCR | GETADDR(ctrlreg);

  /* The PHY/MAC sequence requires 24-clocks:  8 to clock out the cmd,
   * 8 dummy bits, and 8 to clock in the PHY/MAC data.
   */

  ENC_SPI_SendBuf(data, data, (ISPHYMAC(ctrlreg))?3:2);
  return (ISPHYMAC(ctrlreg))?data[2]:data[1];
}

/****************************************************************************
 * Function: enc_wrbreg
 *
 * Description:
 *   Write to a banked control register using the WCR command.  Unlike
 *   reading, this same SPI sequence works for normal, MAC, and PHY
 *   registers.
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *   ctrlreg - Bit encoded address of banked register to write
 *   wrdata  - The data to send
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void enc_wrbreg(ENC_HandleTypeDef *handle, uint8_t ctrlreg,
                       uint8_t wrdata)
{
  uint8_t data[2];

  /* Set the bank */

  enc_setbank(handle, GETBANK(ctrlreg));

  /* Send the WCR command and data.  The sequence requires 16-clocks:
   * 8 to clock out the cmd + 8 to clock out the data.
   */

  data[0] = ENC_WCR | GETADDR(ctrlreg);
  data[1] = wrdata;

  ENC_SPI_SendBuf(data, NULL, 2);
}

/****************************************************************************
 * Function: enc_waitbreg
 *
 * Description:
 *   Wait until banked register bit(s) take a specific value (or a timeout
 *   occurs).
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *   ctrlreg - Bit encoded address of banked register to check
 *   bits    - The bits to check (a mask)
 *   value   - The value of the bits to return (value under mask)
 *
 * Returned Value:
 *   OK on success, negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

static bool enc_waitbreg(ENC_HandleTypeDef *handle, uint8_t ctrlreg,
                        uint8_t bits, uint8_t value)
{
  uint32_t start = HAL_GetTick();
  uint32_t elapsed;
  uint8_t  rddata;

  /* Loop until the exit condition is met */

  do
    {
      /* Read the byte from the requested banked register */

      rddata  = enc_rdbreg(handle, ctrlreg);
      elapsed = HAL_GetTick() - start;
    }
  while ((rddata & bits) != value && elapsed < ENC_POLLTIMEOUT);

  return (rddata & bits) == value;
}


/****************************************************************************
 * Function: enc_rdphy
 *
 * Description:
 *   Read 16-bits of PHY data.
 *
 * Parameters:
 *   priv    - Reference to the driver state structure
 *   phyaddr - The PHY register address
 *
 * Returned Value:
 *   16-bit value read from the PHY
 *
 * Assumptions:
 *
 ****************************************************************************/

static uint16_t enc_rdphy(ENC_HandleTypeDef *handle, uint8_t phyaddr)
{
  uint16_t data = 0;

  /* "To read from a PHY register:
   *
   *   1. Write the address of the PHY register to read from into the MIREGADR
   *      register.
   */

  enc_wrbreg(handle, ENC_MIREGADR, phyaddr);

  /*   2. Set the MICMD.MIIRD bit. The read operation begins and the
   *      MISTAT.BUSY bit is set.
   */

  enc_wrbreg(handle, ENC_MICMD, MICMD_MIIRD);

  /*   3. Wait 10.24 탎. Poll the MISTAT.BUSY bit to be certain that the
   *      operation is complete. While busy, the host controller should not
   *      start any MIISCAN operations or write to the MIWRH register.
   *
   *      When the MAC has obtained the register contents, the BUSY bit will
   *      clear itself.
   */

//  volatile int i;
//  for (i=0; i<12*17; i++) {
//  }

  up_udelay(12);

  if (enc_waitbreg(handle, ENC_MISTAT, MISTAT_BUSY, 0x00))
    {
      /* 4. Clear the MICMD.MIIRD bit. */

      enc_wrbreg(handle, ENC_MICMD, 0x00);

      /* 5. Read the desired data from the MIRDL and MIRDH registers. The
       *    order that these bytes are accessed is unimportant."
       */

      data  = (uint16_t)enc_rdbreg(handle, ENC_MIRDL);
      data |= (uint16_t)enc_rdbreg(handle, ENC_MIRDH) << 8;
    }

  return data;
}

/****************************************************************************
 * Function: enc_wrphy
 *
 * Description:
 *   write 16-bits of PHY data.
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *   phyaddr - The PHY register address
 *   phydata - 16-bit data to write to the PHY
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void enc_wrphy(ENC_HandleTypeDef *handle, uint8_t phyaddr,
                      uint16_t phydata)
{
  /* "To write to a PHY register:
   *
   *    1. Write the address of the PHY register to write to into the
   *       MIREGADR register.
   */

  enc_wrbreg(handle, ENC_MIREGADR, phyaddr);

  /*    2. Write the lower 8 bits of data to write into the MIWRL register. */

  enc_wrbreg(handle, ENC_MIWRL, phydata);

  /*    3. Write the upper 8 bits of data to write into the MIWRH register.
   *       Writing to this register automatically begins the MIIM transaction,
   *       so it must be written to after MIWRL. The MISTAT.BUSY bit becomes
   *       set.
   */

  enc_wrbreg(handle, ENC_MIWRH, phydata >> 8);

  /*    The PHY register will be written after the MIIM operation completes,
   *    which takes 10.24 탎. When the write operation has completed, the BUSY
   *    bit will clear itself.
   *
   *    The host controller should not start any MIISCAN or MIIRD operations
   *    while busy."
   */

  /* wait for approx 12 탎 */
//  volatile int i;
//  for (i=0; i<12*17; i++) {
//  }

  up_udelay(12);
  enc_waitbreg(handle, ENC_MISTAT, MISTAT_BUSY, 0x00);
}


/****************************************************************************
 * Function: enc_pwrfull
 *
 * Description:
 *   When normal operation is desired, the host controller must perform
 *   a slightly modified procedure:
 *
 *   1. Wake-up by clearing ECON2.PWRSV.
 *   2. Wait at least 300 탎 for the PHY to stabilize. To accomplish the
 *      delay, the host controller may poll ESTAT.CLKRDY and wait for it
 *      to become set.
 *   3. Restore receive capability by setting ECON1.RXEN.
 *
 *   After leaving Sleep mode, there is a delay of many milliseconds
 *   before a new link is established (assuming an appropriate link
 *   partner is present). The host controller may wish to wait until
 *   the link is established before attempting to transmit any packets.
 *   The link status can be determined by polling the PHSTAT2.LSTAT bit.
 *   Alternatively, the link change interrupt may be used if it is
 *   enabled.
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

/* Power save mode not used (fix errata 4 and 19) */
#if 0
static void enc_pwrfull(ENC_HandleTypeDef *handle)
{
  /* 1. Wake-up by clearing ECON2.PWRSV. */

  enc_bfcgreg(ENC_ECON2, ECON2_PWRSV);

  /* 2. Wait at least 300 탎 for the PHY to stabilize. To accomplish the
   * delay, the host controller may poll ESTAT.CLKRDY and wait for it to
   * become set.
   */

  /* wait for approx 350 탎 */
//  volatile int i;
//  for (i=0; i<350*17; i++) {
//  }

  up_udelay(350);

  enc_waitbreg(handle, ENC_ESTAT, ESTAT_CLKRDY, ESTAT_CLKRDY);

  /* 3. Restore receive capability by setting ECON1.RXEN.
   *
   * The caller will do this when it is ready to receive packets
   */
}
#endif

/**
  * @brief  Initialize the enc28j60 and configure the needed hardware resources
  * @param  handle: Handle on data configuration.
  * @retval None
  */
bool ENC_Start(ENC_HandleTypeDef *handle)
{
    /* register value */
    uint8_t regval;

    /* Calibrate time constant */
    calibrate();

    /* System reset */
	enc_reset(handle);

	/* Use bank 0 */
	enc_setbank(handle, 0);

    /* Check if we are actually communicating with the ENC28J60.  If its
     * 0x00 or 0xff, then we are probably not communicating correctly
     * via SPI.
     */

    regval = enc_rdbreg(handle, ENC_EREVID);
    if (regval == 0x00 || regval == 0xff) {
      return false;
    }
 
    /* Initialize ECON2: Enable address auto increment.
     */

    enc_wrgreg(ENC_ECON2, ECON2_AUTOINC /* | ECON2_VRPS*/);

    /* Initialize receive buffer.
     * First, set the receive buffer start address.
     */

    handle->nextpkt = PKTMEM_RX_START;
    enc_wrbreg(handle, ENC_ERXSTL, PKTMEM_RX_START & 0xff);
    enc_wrbreg(handle, ENC_ERXSTH, PKTMEM_RX_START >> 8);

    /* Set the receive data pointer */

    /* Errata 14 */
    enc_wrbreg(handle, ENC_ERXRDPTL, PKTMEM_RX_END & 0xff);
    enc_wrbreg(handle, ENC_ERXRDPTH, PKTMEM_RX_END >> 8);
/*
    enc_wrbreg(handle, ENC_ERXRDPTL, PKTMEM_RX_START & 0xff);
    enc_wrbreg(handle, ENC_ERXRDPTH, PKTMEM_RX_START >> 8);
*/

    /* Set the receive buffer end. */

    enc_wrbreg(handle, ENC_ERXNDL, PKTMEM_RX_END & 0xff);
    enc_wrbreg(handle, ENC_ERXNDH, PKTMEM_RX_END >> 8);

    /* Set transmit buffer start. */

    handle->transmitLength = 0;
    enc_wrbreg(handle, ENC_ETXSTL, PKTMEM_TX_START & 0xff);
    enc_wrbreg(handle, ENC_ETXSTH, PKTMEM_TX_START >> 8);

    /* Set filter mode: unicast OR broadcast AND crc valid */

    enc_wrbreg(handle, ENC_ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN);

	do {
		HAL_Delay(10); /* Wait for 10 ms to let the clock be ready */
		regval = enc_rdbreg(handle, ENC_ESTAT);
	} while ((regval & ESTAT_CLKRDY) == 0);

    /* Enable MAC receive */

    enc_wrbreg(handle, ENC_MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);

    /* Enable automatic padding and CRC operations */

    if (handle->Init.DuplexMode == ETH_MODE_HALFDUPLEX) {
      enc_wrbreg(handle, ENC_MACON3,
                 ((handle->Init.ChecksumMode == ETH_CHECKSUM_BY_HARDWARE)?MACON3_PADCFG0 | MACON3_TXCRCEN:0) |
                 MACON3_FRMLNEN);
      enc_wrbreg(handle, ENC_MACON4, MACON4_DEFER);        /* Defer transmission enable */

      /* Set Non-Back-to-Back Inter-Packet Gap */

      enc_wrbreg(handle, ENC_MAIPGL, 0x12);
      enc_wrbreg(handle, ENC_MAIPGH, 0x0c);

      /* Set Back-to-Back Inter-Packet Gap */

      enc_wrbreg(handle, ENC_MABBIPG, 0x12);
    } else {
      /* Set filter mode: unicast OR broadcast AND crc valid AND Full Duplex */

      enc_wrbreg(handle, ENC_MACON3,
                ((handle->Init.ChecksumMode == ETH_CHECKSUM_BY_HARDWARE)?MACON3_PADCFG0 | MACON3_TXCRCEN:0) |
                MACON3_FRMLNEN | MACON3_FULDPX);

      /* Set Non-Back-to-Back Inter-Packet Gap */

      enc_wrbreg(handle, ENC_MAIPGL, 0x12);

      /* Set Back-to-Back Inter-Packet Gap */

      enc_wrbreg(handle, ENC_MABBIPG, 0x15);
    }

    /* Set the maximum packet size which the controller will accept */

    enc_wrbreg(handle, ENC_MAMXFLL, (CONFIG_NET_ETH_MTU+18) & 0xff);
    enc_wrbreg(handle, ENC_MAMXFLH, (CONFIG_NET_ETH_MTU+18) >> 8);

  /* Configure LEDs (No, just use the defaults for now) */
  /* enc_wrphy(priv, ENC_PHLCON, ??); */

    /* Setup up PHCON1 & 2 */

    if (handle->Init.DuplexMode == ETH_MODE_HALFDUPLEX) {
      enc_wrphy(handle, ENC_PHCON1, 0x00);
      enc_wrphy(handle, ENC_PHCON2, PHCON2_HDLDIS); /* errata 9 workaround */
    } else {
      enc_wrphy(handle, ENC_PHCON1, PHCON1_PDPXMD); /* errata 10 workaround */
      enc_wrphy(handle, ENC_PHCON2, 0x00);
    }

    /* Not used Restore normal operation mode
    enc_pwrfull(handle); */

    /* Process interrupt settings */
    if (handle->Init.InterruptEnableBits & EIE_LINKIE) {
      /* Enable link change interrupt in PHY module */
      enc_wrphy(handle, ENC_PHIE, PHIE_PGEIE | PHIE_PLNKIE);
    }

    /* Since we not modify PHLCON register, we don't fall in errata 11 case */

    /* Reset all interrupt flags */
    enc_bfcgreg(ENC_EIR, EIR_ALLINTS);

    regval = handle->Init.InterruptEnableBits;
    if (regval) {
        /* Ensure INTIE is set when at least an interruption is selected */
        regval |= EIE_INTIE;
    }
    /* Enable selected interrupts in ethernet controller module */
    enc_bfsgreg(ENC_EIE, regval);

    /* Enable the receiver */
    enc_bfsgreg(ENC_ECON1, ECON1_RXEN);

    return true;
}

/**
  * @}
  */

/****************************************************************************
 * Function: ENC_SetMacAddr
 *
 * Description:
 *   Set the MAC address to the configured value.  This is done after ifup
 *   or after a TX timeout.  Note that this means that the interface must
 *   be down before configuring the MAC addr.
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

void ENC_SetMacAddr(ENC_HandleTypeDef *handle)
{
  /* Program the hardware with it's MAC address (for filtering).
   *   MAADR1  MAC Address Byte 1 (MAADR<47:40>), OUI Byte 1
   *   MAADR2  MAC Address Byte 2 (MAADR<39:32>), OUI Byte 2
   *   MAADR3  MAC Address Byte 3 (MAADR<31:24>), OUI Byte 3
   *   MAADR4  MAC Address Byte 4 (MAADR<23:16>)
   *   MAADR5  MAC Address Byte 5 (MAADR<15:8>)
   *   MAADR6  MAC Address Byte 6 (MAADR<7:0>)
   */

  enc_wrbreg(handle, ENC_MAADR1, handle->Init.MACAddr[0]);
  enc_wrbreg(handle, ENC_MAADR2, handle->Init.MACAddr[1]);
  enc_wrbreg(handle, ENC_MAADR3, handle->Init.MACAddr[2]);
  enc_wrbreg(handle, ENC_MAADR4, handle->Init.MACAddr[3]);
  enc_wrbreg(handle, ENC_MAADR5, handle->Init.MACAddr[4]);
  enc_wrbreg(handle, ENC_MAADR6, handle->Init.MACAddr[5]);
}


/****************************************************************************
 * Function: ENC_WriteBuffer
 *
 * Description:
 *   Write a buffer of data.
 *
 * Parameters:
 *   buffer  - A pointer to the buffer to write from
 *   buflen  - The number of bytes to write
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Read pointer is set to the correct address
 *
 ****************************************************************************/

void ENC_WriteBuffer(void *buffer, uint16_t buflen)
{
  /* Send the WBM command and copy the packet itself into the transmit
   * buffer at the position of the EWRPT register.
   */

  /* Select ENC28J60 chip
   *
   * "The WBM command is started by lowering the CS pin. ..."
   * We explicitly select the ENC28J60 chip because we have to transmits several pieces of
   * information while keeping CS low
   *
   */

  ENC_SPI_Select(true);

  /* Send the write buffer memory command (ignoring the response)
   *
   * "...The [3-bit]WBM opcode should then be sent to the ENC28J60,
   *  followed by the 5-bit constant, 1Ah."
   */


  ENC_SPI_SendWithoutSelection(ENC_WBM);

  /* Send the buffer
   *
   * "... After the WBM command and constant are sent, the data to
   *  be stored in the memory pointed to by EWRPT should be shifted
   *  out MSb first to the ENC28J60. After 8 data bits are received,
   *  the Write Pointer will automatically increment if AUTOINC is
   *  set. The host controller can continue to provide clocks on the
   *  SCK pin and send data on the SI pin, without raising CS, to
   *  keep writing to the memory. In this manner, with AUTOINC
   *  enabled, it is possible to continuously write sequential bytes
   *  to the buffer memory without any extra SPI command
   *  overhead.
   */

  ENC_SPI_SendBuf(buffer, NULL, buflen);

  /* De-select ENC28J60 chip
   *
   * "The WBM command is terminated by bringing up the CS pin. ..."
   * done in ENC_SPI_SendBuf callback
   */

}

/****************************************************************************
 * Function: enc_rdbuffer
 *
 * Description:
 *   Read a buffer of data.
 *
 * Parameters:
 *   buffer  - A pointer to the buffer to read into
 *   buflen  - The number of bytes to read
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *   Read pointer is set to the correct address
 *
 ****************************************************************************/

static void enc_rdbuffer(void *buffer, int16_t buflen)
{
  /* Select ENC28J60 chip */

  ENC_SPI_Select(true);

  /* Send the read buffer memory command (ignoring the response) */

  ENC_SPI_SendWithoutSelection(ENC_RBM);

  /* Then read the buffer data */

  ENC_SPI_SendBuf(NULL, buffer, buflen);

  /* De-select ENC28J60 chip: done in ENC_SPI_SendBuf callback */
}

/****************************************************************************
 * Function: ENC_RestoreTXBuffer
 *
 * Description:
 *   Prepare TX buffer
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *   len     - length of buffer
 *
 * Returned Value:
 *    ERR_OK          0    No error, everything OK.
 *    ERR_MEM        -1    Out of memory error.
 *    ERR_TIMEOUT    -3    Timeout.
 *
 * Assumptions:
 *
 ****************************************************************************/

int8_t ENC_RestoreTXBuffer(ENC_HandleTypeDef *handle, uint16_t len)
{
  uint16_t txend;
  uint8_t control_write[2];

  /* Wait while TX is busy */
  if (!enc_waitgreg(ENC_ECON1, ECON1_TXRTS, 0)) {
    return ERR_TIMEOUT;
  }

  /* Verify that the hardware is ready to send another packet.  The driver
   * starts a transmission process by setting ECON1.TXRTS. When the packet is
   * finished transmitting or is aborted due to an error/cancellation, the
   * ECON1.TXRTS bit will be cleared.
   *
   * NOTE: If we got here, then we have committed to sending a packet.
   * higher level logic must have assured that TX-related interrupts are disabled.
   */

  /* Send the packet: address=priv->dev.d_buf, length=priv->dev.d_len */

  /* Set transmit buffer start (is this necessary?). */

  enc_wrbreg(handle, ENC_ETXSTL, PKTMEM_TX_START & 0xff);
  enc_wrbreg(handle, ENC_ETXSTH, PKTMEM_TX_START >> 8);

  /* Reset the write pointer to start of transmit buffer */

  enc_wrbreg(handle, ENC_EWRPTL, PKTMEM_TX_START & 0xff);
  enc_wrbreg(handle, ENC_EWRPTH, PKTMEM_TX_START >> 8);

  /* Set the TX End pointer based on the size of the packet to send. Note
   * that the offset accounts for the control byte at the beginning the
   * buffer plus the size of the packet data.
   */

  txend = PKTMEM_TX_START + len;

  if (txend+8>PKTMEM_TX_ENDP1) {
    return ERR_MEM;
  }

  enc_wrbreg(handle, ENC_ETXNDL, txend & 0xff);
  enc_wrbreg(handle, ENC_ETXNDH, txend >> 8);

  /* Send the write buffer memory command (ignoring the response)
   *
   * "...The [3-bit]WBM opcode should then be sent to the ENC28J60,
   *  followed by the 5-bit constant, 1Ah."
   *
   * "...the ENC28J60 requires a single per packet control byte to
   * precede the packet for transmission."
   *
   * POVERRIDE: Per Packet Override bit (Not set):
   *   1 = The values of PCRCEN, PPADEN and PHUGEEN will override the
   *       configuration defined by MACON3.
   *   0 = The values in MACON3 will be used to determine how the packet
   *       will be transmitted
   * PCRCEN: Per Packet CRC Enable bit (Set, but won't be used because
   *   POVERRIDE is zero).
   * PPADEN: Per Packet Padding Enable bit (Set, but won't be used because
   *   POVERRIDE is zero).
   * PHUGEEN: Per Packet Huge Frame Enable bit (Set, but won't be used
   *   because POVERRIDE is zero).
   */

  control_write[0] = ENC_WBM;
  control_write[1] = PKTCTRL_PCRCEN | PKTCTRL_PPADEN | PKTCTRL_PHUGEEN;
  ENC_SPI_SendBuf(control_write, control_write, 2);

  return ERR_OK;
}

/****************************************************************************
 * Function: ENC_Transmit
 *
 * Description:
 *   Start hardware transmission.  Called either from:
 *
 *   -  pkif interrupt when an application responds to the receipt of data
 *      by trying to send something, or
 *   -  From watchdog based polling.
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *   len     - length of buffer
 *
 * Returned Value:
 *   none
 *
 * Assumptions:
 *
 ****************************************************************************/

#ifdef USE_PROTOTHREADS
PT_THREAD(ENC_Transmit(struct pt *pt, ENC_HandleTypeDef *handle))
#else
void ENC_Transmit(ENC_HandleTypeDef *handle)
#endif
{
    PT_BEGIN(pt);

    if (handle->transmitLength != 0) {
        /* A frame is ready for transmission */
        /* Set TXRTS to send the packet in the transmit buffer */

        //enc_bfsgreg(ENC_ECON1, ECON1_TXRTS);
        /* Implement erratas 12, 13 and 15 */
        /* Reset transmit logic */
        handle->retries = 16;
        do {
            enc_bfsgreg(ENC_ECON1, ECON1_TXRST);
            enc_bfcgreg(ENC_ECON1, ECON1_TXRST);
            enc_bfcgreg(ENC_EIR, EIR_TXERIF | EIR_TXIF);

            /* Start transmission */
            enc_bfsgreg(ENC_ECON1, ECON1_TXRTS);

#ifdef USE_PROTOTHREADS
            handle->startTime = HAL_GetTick();
            handle->duration = 20; /* Timeout after 20 ms */
            PT_WAIT_UNTIL(pt, (((enc_rdgreg(ENC_EIR) & (EIR_TXIF | EIR_TXERIF)) != 0) ||
                          (HAL_GetTick() - handle->startTime > handle->duration)));
#else
            /* Wait for end of transmission */
            enc_waitwhilegreg(ENC_EIR, EIR_TXIF | EIR_TXERIF, 0);
#endif

            HAL_Delay(20); // Added by AGB - fixes weird timing bug

            /* Stop transmission */
            enc_bfcgreg(ENC_ECON1, ECON1_TXRTS);

            {
                uint16_t addtTsv4;
                uint8_t tsv4, regval;

                /* read tsv */
                addtTsv4 = PKTMEM_TX_START + handle->transmitLength + 4;

                enc_wrbreg(handle, ENC_ERDPTL, addtTsv4 & 0xff);
                enc_wrbreg(handle, ENC_ERDPTH, addtTsv4 >> 8);

                enc_rdbuffer(&tsv4, 1);

                regval = enc_rdgreg(ENC_EIR);
                if (!(regval & EIR_TXERIF) || !(tsv4 & TSV_LATECOL)) {
                    break;
                }
            }
            handle->retries--;
        } while (handle->retries > 0);
        /* Transmission finished (but can be unsuccessful) */
        handle->transmitLength = 0;
    }
    PT_END(pt);
}

/****************************************************************************
 * Function: ENC_GetReceivedFrame
 *
 * Description:
 *   Check if we have received packet, and if so, retrieve them.
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *
 * Returned Value:
 *   true if new packet is available; false otherwise
 *
 * Assumptions:
 *
 ****************************************************************************/

bool ENC_GetReceivedFrame(ENC_HandleTypeDef *handle)
{
    uint8_t  rsv[6];
    uint16_t pktlen;
    uint16_t rxstat;

    uint8_t pktcnt;

    bool result = true;

    pktcnt = enc_rdbreg(handle, ENC_EPKTCNT);
    if (pktcnt == 0) {
        return false;
    };

    /* Set the read pointer to the start of the received packet (ERDPT) */

    enc_wrbreg(handle, ENC_ERDPTL, (handle->nextpkt) & 0xff);
    enc_wrbreg(handle, ENC_ERDPTH, (handle->nextpkt) >> 8);

    /* Read the next packet pointer and the 4 byte read status vector (RSV)
    * at the beginning of the received packet. (ERDPT should auto-increment
    * and wrap to the beginning of the read buffer as necessary)
    */

    enc_rdbuffer(rsv, 6);

    /* Decode the new next packet pointer, and the RSV.  The
    * RSV is encoded as:
    *
    *  Bits 0-15:  Indicates length of the received frame. This includes the
    *              destination address, source address, type/length, data,
    *              padding and CRC fields. This field is stored in little-
    *              endian format.
    *  Bits 16-31: Bit encoded RX status.
    */

    handle->nextpkt = (uint16_t)rsv[1] << 8 | (uint16_t)rsv[0];
    pktlen        = (uint16_t)rsv[3] << 8 | (uint16_t)rsv[2];
    rxstat        = (uint16_t)rsv[5] << 8 | (uint16_t)rsv[4];

  /* Check if the packet was received OK */

    if ((rxstat & RXSTAT_OK) == 0) {
#ifdef CONFIG_ENC28J60_STATS
        priv->stats.rxnotok++;
#endif
        result = false;
    } else { /* Check for a usable packet length (4 added for the CRC) */
        if (pktlen > (CONFIG_NET_ETH_MTU + 4) || pktlen <= (ETH_HDRLEN + 4)) {
    #ifdef CONFIG_ENC28J60_STATS
            priv->stats.rxpktlen++;
    #endif
            result = false;
        } else { /* Otherwise, read and process the packet */
            /* Save the packet length (without the 4 byte CRC) in handle->RxFrameInfos.length*/

            handle->RxFrameInfos.length = pktlen - 4;

            /* Copy the data data from the receive buffer to priv->dev.d_buf.
            * ERDPT should be correctly positioned from the last call to to
            * end_rdbuffer (above).
            */

            enc_rdbuffer(handle->RxFrameInfos.buffer, handle->RxFrameInfos.length);

        }
    }

    /* Move the RX read pointer to the start of the next received packet.
    * This frees the memory we just read.
    */

    /* Errata 14 (on se sert de rxstat comme variable temporaire */
    rxstat = handle->nextpkt;
    if (rxstat == PKTMEM_RX_START) {
        rxstat = PKTMEM_RX_END;
    } else {
        rxstat--;
    }
    enc_wrbreg(handle, ENC_ERXRDPTL, rxstat & 0xff);
    enc_wrbreg(handle, ENC_ERXRDPTH, rxstat >> 8);
/*
    enc_wrbreg(handle, ENC_ERXRDPTL, (handle->nextpkt));
    enc_wrbreg(handle, ENC_ERXRDPTH, (handle->nextpkt) >> 8);
*/

    /* Decrement the packet counter indicate we are done with this packet */

    enc_bfsgreg(ENC_ECON2, ECON2_PKTDEC);

    return result;
}

/****************************************************************************
 * Function: enc_linkstatus
 *
 * Description:
 *   The current link status can be obtained from the PHSTAT1.LLSTAT or
 *   PHSTAT2.LSTAT.
 *
 * Parameters:
 *   priv    - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void enc_linkstatus(ENC_HandleTypeDef *handle)
{
  handle->LinkStatus = enc_rdphy(handle, ENC_PHSTAT2);
}

/****************************************************************************
 * Function: ENC_EnableInterrupts
 *
 * Description:
 *   Enable individual ENC28J60 interrupts
 *
 * Parameters:
 *   bits - The individual bits to enable
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

void ENC_EnableInterrupts(uint8_t bits)
{
    enc_bfsgreg(ENC_EIE, bits);
}


/****************************************************************************
 * Function: ENC_IRQHandler
 *
 * Description:
 *   Perform interrupt handling logic outside of the interrupt handler (on
 *   the work queue thread).
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

void ENC_IRQHandler(ENC_HandleTypeDef *handle)
{
    uint8_t eir;

    /* Disable further interrupts by clearing the global interrupt enable bit.
     * "After an interrupt occurs, the host controller should clear the global
     * enable bit for the interrupt pin before servicing the interrupt. Clearing
     * the enable bit will cause the interrupt pin to return to the non-asserted
     * state (high). Doing so will prevent the host controller from missing a
     * falling edge should another interrupt occur while the immediate interrupt
     * is being serviced."
     */

    enc_bfcgreg(ENC_EIE, EIE_INTIE);

    /* Read EIR for interrupt flags
     */

    eir = enc_rdgreg(ENC_EIR) & EIR_ALLINTS;

    /* PKTIF is not reliable, check PKCNT instead */
    if (enc_rdbreg(handle, ENC_EPKTCNT) != 0) {
        /* Manage EIR_PKTIF by software */
        eir |= EIR_PKTIF;
    }

    /* Store interrupt flags in handle */
    handle->interruptFlags = eir;

    /* If link status has changed, read it */
    if ((eir & EIR_LINKIF) != 0) /* Link change interrupt */
    {
        enc_linkstatus(handle);       /* Get current link status */
        enc_rdphy(handle, ENC_PHIR);  /* Clear the LINKIF interrupt */
    }

    /* Reset ENC28J60 interrupt flags, except PKTIF form which interruption is deasserted when PKTCNT reaches 0 */
    enc_bfcgreg(ENC_EIR, EIR_ALLINTS);

    /* Enable Ethernet interrupts */
    /* done after effective process on interrupts enc_bfsgreg(ENC_EIE, EIE_INTIE); */
}

/****************************************************************************
 * Function: ENC_GetPkcnt
 *
 * Description:
 *   Get the number of pending receive packets
 *
 * Parameters:
 *   handle  - Reference to the driver state structure
 *
 * Returned Value:
 *   the number of receive packet not processed yet
 *
 * Assumptions:
 *
 ****************************************************************************/

void ENC_GetPkcnt(ENC_HandleTypeDef *handle)
{
    handle->pktCnt = enc_rdbreg(handle, ENC_EPKTCNT);
}


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
