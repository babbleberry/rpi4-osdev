#include "../include/fb.h"
#include "../include/spi.h"
#include "../net/enc28j60.h"
#include "../tcpip/ip_arp_udp_tcp.h"

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

int strncmp(const char *s1, const char *s2, unsigned short n)
{
    unsigned char u1, u2;

    while (n-- > 0)
    {
       u1 = (unsigned char) *s1++;
       u2 = (unsigned char) *s2++;
       if (u1 != u2) return u1 - u2;
       if (u1 == '\0') return 0;
    }

    return 0;
}

// NETWORKING GLOBALS AND FUNCTIONS

ENC_HandleTypeDef handle;

// MAC address to be assigned to the ENC28J60
unsigned char myMAC[6] = { 0xc0, 0xff, 0xee, 0xc0, 0xff, 0xee };

// IP address to be assigned to the ENC28J60
unsigned char deviceIP[4] = { 192, 168, 0, 66 };

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

void enc28j60PacketSend(unsigned short buflen, void *buffer) {
   if (ENC_RestoreTXBuffer(&handle, buflen) == 0) {
      ENC_WriteBuffer((unsigned char *) buffer, buflen);
      handle.transmitLength = buflen;
      ENC_Transmit(&handle);
   }
}

void serve(void)
{
   while (1) {
      while (!ENC_GetReceivedFrame(&handle));

      uint8_t *buf = (uint8_t *)handle.RxFrameInfos.buffer;
      uint16_t len = handle.RxFrameInfos.length;
      uint16_t dat_p = packetloop_arp_icmp_tcp(buf, len);

      if (dat_p != 0) {
         debugstr("Incoming web request... ");

         if (strncmp("GET ", (char *)&(buf[dat_p]), 4) != 0) {
            debugstr("not GET");
            dat_p = fill_tcp_data(buf, 0, "HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>ERROR</h1>");
         } else {
            if (strncmp("/ ", (char *)&(buf[dat_p+4]), 2) == 0) {
               // just one web page in the "root directory" of the web server
               debugstr("GET root");
               dat_p = fill_tcp_data(buf, 0, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello world!</h1>");
            } else {
               // just one web page not in the "root directory" of the web server
               debugstr("GET not root");
               dat_p = fill_tcp_data(buf, 0, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Goodbye cruel world.</h1>");
            }
         }

         www_server_reply(buf, dat_p); // send web page data
         debugcrlf();
      }
   }
}

// MAIN FUNCTION

void main(void)
{
    fb_init();

    // Init network and serve web pages

    spi_init();
    init_network();
    serve();

    // Catch us if we fall

    while(1);
}
