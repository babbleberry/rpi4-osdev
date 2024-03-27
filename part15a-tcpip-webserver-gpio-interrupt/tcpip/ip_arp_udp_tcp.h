/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright:LGPL V2
 * See http://www.gnu.org/licenses/old-licenses/lgpl-2.0.html
 *
 * IP/ARP/UDP/TCP functions
 *
 *********************************************/
//@{
#ifndef IP_ARP_UDP_TCP_H
#define IP_ARP_UDP_TCP_H 1

#include "ip_config.h"

// set my own mac address:
extern void init_mac(uint8_t *mymac); // not needed if you call init_udp_or_www_server
// -- web server functions --
#if defined (WWW_server) || defined (UDP_server)
// you must call this function once before you use any of the other server functions:
// mymac may be set to NULL in this function if init_mac was used before
// init_ip_arp_udp_tcp is now replaced by init_udp_or_www_server  and the www_server_port function.
extern void init_udp_or_www_server(uint8_t *mymac,uint8_t *myip);
#endif

#if defined (WWW_server)
extern void www_server_port(uint16_t port); // not needed if you want port 80
// send data from the web server to the client:
extern void www_server_reply(uint8_t *buf,uint16_t dlen);
#endif

// for a UDP server:
#if defined (UDP_server)
extern uint8_t eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len);
extern void make_udp_reply_from_request_udpdat_ready(uint8_t *buf,uint16_t datalen,uint16_t port);
extern void make_udp_reply_from_request(uint8_t *buf,char *data,uint8_t datalen,uint16_t port);
#endif
// return 0 to just continue in the packet loop and return the position
// of the tcp data if there is tcp data part:
extern uint16_t packetloop_arp_icmp_tcp(uint8_t *buf,uint16_t plen);
// functions to fill the web pages with data:
extern uint16_t fill_tcp_data_p(uint8_t *buf,uint16_t pos, const uint8_t *progmem_s);
extern uint16_t fill_tcp_data_string(uint8_t *buf,uint16_t pos, char *reply);

extern uint16_t fill_tcp_data(uint8_t *buf,uint16_t pos, const char *s);
// fill a binary string of len data into the tcp packet:
extern uint16_t fill_tcp_data_len(uint8_t *buf,uint16_t pos, const uint8_t *s, uint8_t len);

// -- client only functions --
#if defined (WWW_client) || defined (NTP_client) || defined (UDP_client) || defined (TCP_client) || defined (PING_client)
extern void client_ifconfig(uint8_t *ip,uint8_t *netmask);
// route_via_gw can be used decide if a packed needs to be routed via GW or can be found on the LAN:
extern uint8_t route_via_gw(uint8_t *destip); // returns 1 if destip must be routed via the GW. Returns 0 if destip is on the local LAN
//
// The get_mac_with_arp function can be used to find the MAC address of
// a host that is directly connected to the same LAN. It translates the IP address into
// a MAC address.
// You need to provide a callback function. That function will be executed once the
// MAC address is found. We do this to not block execution in the main loop.
// NOTE: you can only do one MAC address resolution at a time. The reference_number is just
// a number given back to you to make it easier to know what this relates to.
//
// You declare the callback function:
//
//#define TRANS_NUM_GWMAC 12
//void arpresolver_result_callback(uint8_t *ip __attribute__((unused)),uint8_t reference_number,uint8_t *mac){ // the __attribute__((unused)) is a gcc compiler directive to avoid warnings about unsed variables.
//        uint8_t i=0;
//        if (reference_number==TRANS_NUM_GWMAC){
//                // copy mac address over:
//                while(i<6){gwmac[i]=mac[i];i++;}
//        }
//}
//
// and then you can just call get_mac_with_arp like this:
//        get_mac_with_arp(gwip,TRANS_NUM_GWMAC,&arpresolver_result_callback);
// Note: you must have initialized the stack with init_udp_or_www_server or client_ifconfig
// before you can use get_mac_with_arp(). The arp request will automatically be repeated if
// there is no answer.
extern void get_mac_with_arp(uint8_t *ip, uint8_t reference_number,void (*arp_result_callback)(uint8_t *ip,uint8_t reference_number,uint8_t *mac));
uint8_t get_mac_with_arp_wait(void); // checks current ongoing transaction, retuns 0 when the transaction is over
#endif

#ifdef TCP_client
// To use the tcp client you need to:
//
// Declare a callback function to get the result (tcp data from the server):
//
// uint8_t your_client_tcp_result_callback(uint8_t fd, uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data){
//      ...your code;
//      return(close_tcp_session);
//      }
//
// statuscode=0 means the buffer has valid data, otherwise len and pos_in_buf
// are invalid. That is: do to use data_start_pos_in_buf and len_of_data
// if statuscode!=0.
//
// This callback gives you access to the TCP data of the first
// packet returned from the server. You should aim to minimize the server
// output such that this will be the only packet.
//
// close_tcp_session=1 means close the session now. close_tcp_session=0
// read all data and leave it to the other side to close it.
// If you connect to a web server then you want close_tcp_session=0.
// If you connect to a modbus/tcp equipment then you want close_tcp_session=1
//
// Declare a callback function to be called in order to fill in the
//
// request (tcp data sent to the server):
// uint16_t your_client_tcp_datafill_callback(uint8_t fd){...your code;return(len_of_data_filled_in);}
//
// Now call:
// fd=client_tcp_req(&your_client_tcp_result_callback,&your_client_tcp_datafill_callback,portnumber);
//
// fd is a file descriptor like number that you get back in the fill and result
// function so you know to which call of client_tcp_req this callback belongs.
//
// You can not start different clients (e.g modbus and web) at the
// same time but you can start them one after each other. That is
// when the request has timed out or when the result_callback was
// executed then you can start a new one. The fd makes it still possible to
// distinguish in the callback code the different types you started.
//
// Note that you might never get called back if the other side does
// not answer. A timer would be needed to recongnize such a condition.
//
// We use callback functions because that is the best implementation
// given the fact that we have very little RAM memory.
//
extern uint8_t client_tcp_req(uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),uint16_t (*datafill_callback)(uint8_t fd),uint16_t port,uint8_t *dstip,uint8_t *dstmac);
#endif

#ifdef WWW_client
// ----- http get
// The string buffers to which urlbuf_varpart and hoststr are pointing
// must not be changed until the callback is executed.
extern void client_browse_url(const prog_char *urlbuf, char *urlbuf_varpart, const char *hoststr,void (*callback)(uint16_t,uint16_t,uint16_t),uint8_t *dstip,uint8_t *dstmac);
// The callback is a reference to a function which must look like this:
// void browserresult_callback(uint16_t webstatuscode,uint16_t datapos,uint16_t len)
// webstatuscode is zero if there was no proper reply from the server (garbage message total communication failure, this is rare).
// webstatuscode is otherwise the http status code (e.g webstatuscode=200 for 200 OK);
// For possible status codes look at http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
// Basically 2xx is success and any 5xx, 4xx is a failure.
// ----- http post
// client web browser using http POST operation:
// additionalheaderline must be set to NULL if not used.
// The string buffers to which urlbuf_varpart and hoststr are pointing
// must not be changed until the callback is executed.
// postval is a string buffer which can only be de-allocated by the caller
// when the post operation was really done (e.g when callback was executed).
// postval must be urlencoded.
extern void client_http_post(const prog_char *urlbuf, char *urlbuf_varpart,const char *hoststr, const prog_char *additionalheaderline,char *postval,void (*callback)(uint16_t,uint16_t,uint16_t),uint8_t *dstip,uint8_t *dstmac);
// The callback is a reference to a function which must look like this:
// void browserresult_callback(uint16_t webstatuscode,uint16_t datapos,uint16_t len)
// webstatuscode is zero if there was no proper reply from the server (garbage message total communication failure, this is rare).
// webstatuscode is otherwise the http status code (e.g webstatuscode=200 for 200 OK);
#endif

#ifdef NTP_client
// be careful to not mix client_ntp_request with situations where you are filling
// a web-page. Normally you will be using the same packet buffer and
// client_ntp_request writes immediately to buf. You might need to
// set a marker and call client_ntp_request when your main loop is idle.
extern void client_ntp_request(uint8_t *buf,uint8_t *ntpip,uint8_t srcport,uint8_t *dstmac);
extern uint8_t client_ntp_process_answer(uint8_t *buf,uint32_t *time,uint8_t dstport_l);
#endif

#ifdef UDP_client
// There are two ways of using this UDP client:
//
// 1) you call send_udp_prepare, you fill the data yourself into buf starting at buf[UDP_DATA_P],
// you send the packet by calling send_udp_transmit
//
// 2) You just allocate a large enough buffer for you data and you call send_udp and nothing else
// needs to be done.
//
extern void send_udp_prepare(uint8_t *buf,uint16_t sport, const uint8_t *dip, uint16_t dport,const uint8_t *dstmac);
extern void send_udp_transmit(uint8_t *buf,uint16_t datalen);

// send_udp sends via gwip, you must call client_set_gwip at startup, datalen must be less than 220 bytes
extern void send_udp(uint8_t *buf,char *data,uint8_t datalen,uint16_t sport, const uint8_t *dip, uint16_t dport,const uint8_t *dstmac);
#endif

// you can find out who ping-ed you if you want:
extern void register_ping_rec_callback(void (*callback)(uint8_t *srcip));

#ifdef PING_client
extern void client_icmp_request(uint8_t *buf,uint8_t *destip,uint8_t *dstmac);
// you must loop over this function to check if there was a ping reply:
extern uint8_t packetloop_icmp_checkreply(uint8_t *buf,uint8_t *ip_monitoredhost);
#endif // PING_client

#ifdef WOL_client
extern void send_wol(uint8_t *buf,uint8_t *wolmac);
#endif // WOL_client

#if defined GRATARP
// send a Gratuitous arp, this is to refresh the arp
// cash of routers and switches. It can improve the response
// time in wifi networks as some wifi equipment expects the initial
// communication to not start from the network side. That is wrong
// but some consumer devices are made like this.
extern uint8_t gratutious_arp(uint8_t *buf);
#endif // GRATARP

#endif /* IP_ARP_UDP_TCP_H */
//@}
