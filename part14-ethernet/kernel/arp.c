#include "../net/enc28j60.h"
#include "../include/fb.h"

ENC_HandleTypeDef handle;

// Structure for Ethernet header

typedef struct {
   uint8_t DestAddrs[6];
   uint8_t SrcAddrs[6];
   uint16_t type;
} EtherNetII;

// Ethernet packet types

#define ARPPACKET 0x0608
#define IPPACKET  0x0008

// Structure for an ARP Packet

typedef struct {
   EtherNetII eth;
   uint16_t hardware;
   uint16_t protocol;
   uint8_t hardwareSize;
   uint8_t protocolSize;
   uint16_t opCode;
   uint8_t senderMAC[6];
   uint8_t senderIP[4];
   uint8_t targetMAC[6];
   uint8_t targetIP[4];
} ARP;

// ARP OpCodes

#define ARPREPLY   0x0200
#define ARPREQUEST 0x0100

// ARP hardware types

#define ETHERNET   0x0100

// MAC address to be assigned to the ENC28J60

uint8_t myMAC[6] = { 0xc0, 0xff, 0xee, 0xc0, 0xff, 0xee };

// Router MAC is not known to start with, and requires an ARP reply to find out

uint8_t routerMAC[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

// IP address to be assigned to the ENC28J60

uint8_t deviceIP[4] = { 192, 168, 0, 66 };

// IP Address of the router, whose hardware address we will find using the ARP request

uint8_t routerIP[4] = { 192, 168, 0, 1 };

// HELPER FUNCTIONS

void *memset(void *dest, uint8_t val, uint16_t len)
{
    uint8_t *ptr = dest;
    while (len-- > 0)
       *ptr++ = val;
    return dest;
}

void *memcpy(void *dest, const void *src, uint16_t len)
{
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (len--)
       *d++ = *s++;
    return dest;
}

uint8_t memcmp(void *str1, void *str2, unsigned count)
{
    uint8_t *s1 = str1;
    uint8_t *s2 = str2;

    while (count-- > 0)
    {
       if (*s1++ != *s2++)
          return s1[-1] < s2[-1] ? -1 : 1;
    }

    return 0;
}

// MAIN FUNCTIONS

void SendArpPacket(uint8_t *targetIP, uint8_t *deviceMAC)
{
   /* Parameters:
    *   targetIP - The target IP Address for the ARP request (the one whose hardware
    *              address we want)
    *   deviceMAC - The MAC address of the ENC28J60, i.e. the source MAC for the ARP
    *               request
   */

   ARP arpPacket;

   // The source of the packet will be the ENC28J60 MAC address
   memcpy(arpPacket.eth.SrcAddrs, deviceMAC, 6);
    
   // The destination is broadcast - a MAC address of FF:FF:FF:FF:FF:FF */
   memset(arpPacket.eth.DestAddrs, 0xFF, 6);
    
   arpPacket.eth.type = ARPPACKET;
   arpPacket.hardware = ETHERNET;
    
   // We want an IP address resolved

   arpPacket.protocol = IPPACKET;
   arpPacket.hardwareSize = 0x06; // sizeof(deviceMAC);
   arpPacket.protocolSize = 0x04; // sizeof(deviceIP);
   arpPacket.opCode = ARPREQUEST;
  
   // Target MAC is set to 0 as it is unknown
   memset(arpPacket.targetMAC, 0, 6);
    
   // Sender MAC is the ENC28J60's MAC address
   memcpy(arpPacket.senderMAC, deviceMAC, 6);
    
   // The target IP is the IP address we want resolved
   memcpy(arpPacket.targetIP, targetIP, 4);
  
   // Check if the last reply has come from an IP address that we want i.e. someone else is already using it
   if (!memcmp(targetIP, deviceIP, 4)) {
      // Yes, someone is using our IP so set the sender IP to 0.0.0.0
      memset(arpPacket.senderIP, 0, 4);
   } else {
      // No, nobody is using our IP so we can use it confidently
      memcpy(arpPacket.senderIP, deviceIP, 4);
   }

   // Send the packet

   if (ENC_RestoreTXBuffer(&handle, sizeof(ARP)) == 0) {
      debugstr("Sending ARP request.");
      debugcrlf();
      
      ENC_WriteBuffer((unsigned char *)&arpPacket, sizeof(ARP));
      handle.transmitLength = sizeof(ARP);

      ENC_Transmit(&handle);
   }
}

void arp_test(void)
{
   ARP *checkPacket;

   SendArpPacket(routerIP, myMAC);

   debugstr("Waiting for ARP response.");
   debugcrlf();

   while (1) {
      while (!ENC_GetReceivedFrame(&handle));
  
      uint16_t len    = handle.RxFrameInfos.length;
      uint8_t *buffer = (uint8_t *)handle.RxFrameInfos.buffer;
      checkPacket     = (ARP *)buffer;

      if (len > 0) {
         if (!memcmp(checkPacket->senderIP, routerIP, 4)) {
            // Success! We have found our router's MAC address

            memcpy(routerMAC, checkPacket->senderMAC, 6);
            debugstr("Router MAC is ");
            debughex(routerMAC[0]);
            debughex(routerMAC[1]);
            debughex(routerMAC[2]);
            debughex(routerMAC[3]);
            debughex(routerMAC[4]);
            debughex(routerMAC[5]);
            debugcrlf();

            break;
         }
      }
   }
}

void init_network(void)
{
   handle.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
   handle.Init.MACAddr = myMAC;
   handle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
   handle.Init.InterruptEnableBits = EIE_LINKIE | EIE_PKTIE;

   debugstr("Starting network up.");
   debugcrlf();
   if (!ENC_Start(&handle)) {
      debugstr("Could not initialise network card.");
   } else {
      debugstr("Setting MAC address to C0:FF:EE:C0:FF:EE.");
      debugcrlf();

      ENC_SetMacAddr(&handle);

      debugstr("Network card successfully initialised.");
   }
   debugcrlf();

   debugstr("Waiting for ifup... ");
   while (!(handle.LinkStatus & PHSTAT2_LSTAT)) ENC_IRQHandler(&handle);
   debugstr("done.");
   debugcrlf();

   // Re-enable global interrupts
   ENC_EnableInterrupts(EIE_INTIE);
}
