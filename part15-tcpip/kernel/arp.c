#include "../net/enc28j60.h"
#include "../include/fb.h"
#include "../tcpip/ip_arp_udp_tcp.h"

ENC_HandleTypeDef handle;

// MAC address to be assigned to the ENC28J60

unsigned char myMAC[6] = { 0xc0, 0xff, 0xee, 0xc0, 0xff, 0xee };

// IP address to be assigned to the ENC28J60

unsigned char deviceIP[4] = { 192, 168, 0, 66 };

// HELPER FUNCTIONS

void *memset(void *dest, unsigned char val, unsigned short len)
{
    uint8_t *ptr = dest;
    while (len-- > 0)
       *ptr++ = val;
    return dest;
}

void *memcpy(void *dest, const void *src, unsigned short len)
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

void enc28j60PacketSend(unsigned short buflen, void *buffer) {
   if (ENC_RestoreTXBuffer(&handle, buflen) == 0) {
      ENC_WriteBuffer((unsigned char *) buffer, buflen);
      handle.transmitLength = buflen;
      ENC_Transmit(&handle);
   }
}

// MAIN FUNCTIONS

void net_test(void)
{
   while (1) {
      while (!ENC_GetReceivedFrame(&handle));
  
      uint16_t len = handle.RxFrameInfos.length;
      uint8_t *buffer = (uint8_t *)handle.RxFrameInfos.buffer;
      packetloop_arp_icmp_tcp(buffer, len);
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

   debugstr("Initialising the TCP stack... ");
   init_udp_or_www_server(myMAC, deviceIP);
   debugstr("done.");
   debugcrlf();
}
