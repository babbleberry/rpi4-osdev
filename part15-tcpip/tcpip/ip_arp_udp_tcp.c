/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 *
 * Author: Guido Socher
 * Copyright:LGPL V2
 * See http://www.gnu.org/licenses/old-licenses/lgpl-2.0.html
 *
 * IP, Arp, UDP and TCP functions.
 *
 * The TCP implementation uses some size optimisations which are valid
 * only if all data can be sent in one single packet. This is however
 * not a big limitation for a microcontroller as you will anyhow use
 * small web-pages. The web server must send the entire web page in one
 * packet. The client "web browser" as implemented here can also receive
 * large pages.
 *
 *********************************************/

#include "ip_config.h"
#include "ip_arp_udp_tcp.h"

// I use them to debug stuff:
#define LEDOFF PORTB|=(1<<PORTB1)
#define LEDON PORTB&=~(1<<PORTB1)
//
static uint8_t macaddr[6];
static uint8_t ipaddr[4]={0,0,0,0};
//static uint8_t seqnum=0xa; // my initial tcp sequence number
static void (*icmp_callback)(uint8_t *ip);

//
#if defined (NTP_client) || defined (UDP_client) || defined (TCP_client) || defined (PING_client)
#define ARP_MAC_resolver_client 1
#define ALL_clients 1
#endif
#if defined (WWW_client) || defined (TCP_client)
// just lower byte, the upper byte is TCPCLIENT_SRC_PORT_H:
static uint8_t tcpclient_src_port_l=1;
static uint8_t tcp_fd=0; // a file descriptor, will be encoded into the port
static uint8_t tcp_otherside_ip[4];
static uint8_t tcp_dst_mac[6]; // normally the gateway via which we want to send
static uint8_t tcp_client_state=0;
static uint16_t tcp_client_port=0;
// This function will be called if we ever get a result back from the
// TCP connection to the sever:
// close_connection= your_client_tcp_result_callback(uint8_t fd, uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data){...your code}
// statuscode=0 means the buffer has valid data
static uint8_t (*client_tcp_result_callback)(uint8_t,uint8_t,uint16_t,uint16_t);
// len_of_data_filled_in=your_client_tcp_datafill_callback(uint8_t fd){...your code}
static uint16_t (*client_tcp_datafill_callback)(uint8_t);
#endif

#define TCPCLIENT_SRC_PORT_H 11

#if defined (WWW_client)
// WWW_client uses TCP_client
#define TCP_client 1
static uint8_t www_fd=0;
static uint8_t browsertype=0; // 0 = get, 1 = post
static void (*client_browser_callback)(uint16_t,uint16_t,uint16_t); // the fields are: uint16_t webstatuscode,uint16_t datapos,uint16_t len; datapos is start of http data and len the the length of that data
static const prog_char *client_additionalheaderline;
static char *client_postval;
static const prog_char *client_urlbuf;
static const char *client_urlbuf_var;
static const char *client_hoststr;
static uint8_t *bufptr=0; // ugly workaround for backward compatibility
#endif


#ifdef ARP_MAC_resolver_client
// This function will be called if we ever get a result back from the
// the arp request we sent out.
void (*client_arp_result_callback)(uint8_t*,uint8_t,uint8_t*);
static int16_t arp_delaycnt=1;
static uint8_t arpip[4];  // IP to find via arp
static uint8_t arpip_state=0; // 0 at poweron, 1=req sent no answer yet, 2=have mac, 8=ready to accept an arp reply
static uint8_t arp_reference_number=0;
#define WGW_INITIAL_ARP 1
#define WGW_HAVE_MAC 2
#define WGW_ACCEPT_ARP_REPLY 8
#endif

#ifdef WWW_server
static uint8_t wwwport_l=80; // server port
static uint8_t wwwport_h=0;  // Note: never use same as TCPCLIENT_SRC_PORT_H
static uint16_t info_data_len=0;
#endif

#if defined (ALL_clients)
static uint8_t ipnetmask[4]={255,255,255,255};
static uint8_t ipid=0x2; // IP-identification, it works as well if you do not change it but it is better to fill the field, we count this number up and wrap.
const char iphdr[] ={0x45,0,0,0x82,0,0,0x40,0,0x20}; // 0x82 is the total len on ip, 0x20 is ttl (time to live), the second 0,0 is IP-identification and may be changed.
#endif

#define CLIENTMSS 750
#define TCP_DATA_START ((uint16_t)TCP_SRC_PORT_H_P+(buf[TCP_HEADER_LEN_P]>>4)*4)
const char arpreqhdr[] ={0,1,8,0,6,4,0,1};
#ifdef NTP_client
const char ntpreqhdr[] ={0xe3,0,4,0xfa,0,1,0,0,0,1};
#endif

// The Ip checksum is calculated over the ip header only starting
// with the header length field and a total length of 20 bytes
// unitl ip.dst
// You must set the IP checksum field to zero before you start
// the calculation.
// len for ip is 20.
//
// For UDP/TCP we do not make up the required pseudo header. Instead we
// use the ip.src and ip.dst fields of the real packet:
// The udp checksum calculation starts with the ip.src field
// Ip.src=4bytes,Ip.dst=4 bytes,Udp header=8bytes + data length=16+len
// In other words the len here is 8 + length over which you actually
// want to calculate the checksum.
// You must set the checksum field to zero before you start
// the calculation.
// The same algorithm is also used for udp and tcp checksums.
// len for udp is: 8 + 8 + data length
// len for tcp is: 4+4 + 20 + option len + data length
//
// For more information on how this algorithm works see:
// http://www.netfor2.com/checksum.html
// http://www.msc.uky.edu/ken/cs471/notes/chap3.htm
// The RFC has also a C code example: http://www.faqs.org/rfcs/rfc1071.html
uint16_t checksum(uint8_t *buf, uint16_t len,uint8_t type){
        // type 0=ip , icmp
        //      1=udp
        //      2=tcp
        uint32_t sum = 0;

        //if(type==0){
        //        // do not add anything, standard IP checksum as described above
        //        // Usable for ICMP and IP header
        //}
        if(type==1){
                sum+=IP_PROTO_UDP_V; // protocol udp
                // the length here is the length of udp (data+header len)
                // =length given to this function - (IP.scr+IP.dst length)
                sum+=len-8; // = real udp len
        }
        if(type==2){
                sum+=IP_PROTO_TCP_V;
                // the length here is the length of tcp (data+header len)
                // =length given to this function - (IP.scr+IP.dst length)
                sum+=len-8; // = real tcp len
        }
        // build the sum of 16bit words
        while(len >1){
                sum += 0xFFFF & (((uint32_t)*buf<<8)|*(buf+1));
                buf+=2;
                len-=2;
        }
        // if there is a byte left then add it (padded with zero)
        if (len){
                sum += ((uint32_t)(0xFF & *buf))<<8;
        }
        // now calculate the sum over the bytes in the sum
        // until the result is only 16bit long
        while (sum>>16){
                sum = (sum & 0xFFFF)+(sum >> 16);
        }
        // build 1's complement:
        return( (uint16_t) sum ^ 0xFFFF);
}

void init_mac(uint8_t *mymac){
        if (mymac){
                memcpy(macaddr,mymac,6);
        }
}

#if defined (ALL_clients)
void client_ifconfig(uint8_t *ip,uint8_t *netmask)
{
        uint8_t i;
        if (ip){
                i=0;while(i<4){ipaddr[i]=ip[i];i++;}
        }
        if (netmask){
                i=0;while(i<4){ipnetmask[i]=netmask[i];i++;}
        }
}

// returns 1 if destip must be routed via the GW. Returns 0 if destip is on the local LAN
uint8_t route_via_gw(uint8_t *destip)
{
    uint8_t i=0;
    while(i<4){
        if ((destip[i] & ipnetmask[i]) != (ipaddr[i] & ipnetmask[i])){
            return(1);
        }
        i++;
    }
    return(0);
}
#endif


uint8_t check_ip_message_is_from(uint8_t *buf,uint8_t *ip)
{
        uint8_t i=0;
        while(i<4){
                if(buf[IP_SRC_P+i]!=ip[i]){
                        return(0);
                }
                i++;
        }
        return(1);
}

uint8_t eth_type_is_arp_and_my_ip(uint8_t *buf,uint16_t len){
        uint8_t i=0;
        //
        if (len<41){
                return(0);
        }
        if(buf[ETH_TYPE_H_P] != ETHTYPE_ARP_H_V ||
           buf[ETH_TYPE_L_P] != ETHTYPE_ARP_L_V){
                return(0);
        }
        while(i<4){
                if(buf[ETH_ARP_DST_IP_P+i] != ipaddr[i]){
                        return(0);
                }
                i++;
        }
        return(1);
}

uint8_t eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len){
        uint8_t i=0;
        //eth+ip+udp header is 42
        if (len<42){
                return(0);
        }
        if(buf[ETH_TYPE_H_P]!=ETHTYPE_IP_H_V ||
           buf[ETH_TYPE_L_P]!=ETHTYPE_IP_L_V){
                return(0);
        }
        if (buf[IP_HEADER_LEN_VER_P]!=0x45){
                // must be IP V4 and 20 byte header
                return(0);
        }
        while(i<4){
                if(buf[IP_DST_P+i]!=ipaddr[i]){
                        return(0);
                }
                i++;
        }
        return(1);
}

// make a return eth header from a received eth packet
void make_eth(uint8_t *buf)
{
        uint8_t i=0;
        //
        //copy the destination mac from the source and fill my mac into src
        while(i<6){
                buf[ETH_DST_MAC +i]=buf[ETH_SRC_MAC +i];
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
}
void fill_ip_hdr_checksum(uint8_t *buf)
{
        uint16_t ck;
        // clear the 2 byte checksum
        buf[IP_CHECKSUM_P]=0;
        buf[IP_CHECKSUM_P+1]=0;
        buf[IP_FLAGS_P]=0x40; // don't fragment
        buf[IP_FLAGS_P+1]=0;  // fragement offset
        buf[IP_TTL_P]=64; // ttl
        // calculate the checksum:
        ck=checksum(&buf[IP_P], IP_HEADER_LEN,0);
        buf[IP_CHECKSUM_P]=ck>>8;
        buf[IP_CHECKSUM_P+1]=ck& 0xff;
}

// make a return ip header from a received ip packet
void make_ip(uint8_t *buf)
{
        uint8_t i=0;
        while(i<4){
                buf[IP_DST_P+i]=buf[IP_SRC_P+i];
                buf[IP_SRC_P+i]=ipaddr[i];
                i++;
        }
        fill_ip_hdr_checksum(buf);
}

// swap seq and ack number and count ack number up
void step_seq(uint8_t *buf,uint16_t rel_ack_num,uint8_t cp_seq)
{
        uint8_t i;
        uint8_t tseq;
        i=4;
        // sequence numbers:
        // add the rel ack num to SEQACK
        while(i>0){
                rel_ack_num=buf[TCP_SEQ_H_P+i-1]+rel_ack_num;
                tseq=buf[TCP_SEQACK_H_P+i-1];
                buf[TCP_SEQACK_H_P+i-1]=0xff&rel_ack_num;
                if (cp_seq){
                        // copy the acknum sent to us into the sequence number
                        buf[TCP_SEQ_H_P+i-1]=tseq;
                }else{
                        buf[TCP_SEQ_H_P+i-1]= 0; // some preset value
                }
                rel_ack_num=rel_ack_num>>8;
                i--;
        }
}

// make a return tcp header from a received tcp packet
// rel_ack_num is how much we must step the seq number received from the
// other side. We do not send more than 765 bytes of text (=data) in the tcp packet.
// No mss is included here.
//
// After calling this function you can fill in the first data byte at TCP_OPTIONS_P+4
// If cp_seq=0 then an initial sequence number is used (should be use in synack)
// otherwise it is copied from the packet we received
void make_tcphead(uint8_t *buf,uint16_t rel_ack_num,uint8_t cp_seq)
{
        uint8_t i;
        // copy ports:
        i=buf[TCP_DST_PORT_H_P];
        buf[TCP_DST_PORT_H_P]=buf[TCP_SRC_PORT_H_P];
        buf[TCP_SRC_PORT_H_P]=i;
        //
        i=buf[TCP_DST_PORT_L_P];
        buf[TCP_DST_PORT_L_P]=buf[TCP_SRC_PORT_L_P];
        buf[TCP_SRC_PORT_L_P]=i;
        step_seq(buf,rel_ack_num,cp_seq);
        // zero the checksum
        buf[TCP_CHECKSUM_H_P]=0;
        buf[TCP_CHECKSUM_L_P]=0;
        // no options:
        // 20 bytes:
        // The tcp header length is only a 4 bit field (the upper 4 bits).
        // It is calculated in units of 4 bytes.
        // E.g 20 bytes: 20/4=6 => 0x50=header len field
        buf[TCP_HEADER_LEN_P]=0x50;
}

void make_arp_answer_from_request(uint8_t *buf)
{
        uint8_t i=0;
        //
        make_eth(buf);
        buf[ETH_ARP_OPCODE_H_P]=ETH_ARP_OPCODE_REPLY_H_V;
        buf[ETH_ARP_OPCODE_L_P]=ETH_ARP_OPCODE_REPLY_L_V;
        // fill the mac addresses:
        while(i<6){
                buf[ETH_ARP_DST_MAC_P+i]=buf[ETH_ARP_SRC_MAC_P+i];
                buf[ETH_ARP_SRC_MAC_P+i]=macaddr[i];
                i++;
        }
        i=0;
        while(i<4){
                buf[ETH_ARP_DST_IP_P+i]=buf[ETH_ARP_SRC_IP_P+i];
                buf[ETH_ARP_SRC_IP_P+i]=ipaddr[i];
                i++;
        }
        // eth+arp is 42 bytes:
        enc28j60PacketSend(42,buf);
}

void make_echo_reply_from_request(uint8_t *buf,uint16_t len)
{
        make_eth(buf);
        make_ip(buf);
        buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREPLY_V;
        // we changed only the icmp.type field from request(=8) to reply(=0).
        // we can therefore easily correct the checksum:
        if (buf[ICMP_CHECKSUM_P] > (0xff-0x08)){
                buf[ICMP_CHECKSUM_P+1]++;
        }
        buf[ICMP_CHECKSUM_P]+=0x08;
        //
        enc28j60PacketSend(len,buf);
}

// do some basic length calculations
uint16_t get_tcp_data_len(uint8_t *buf)
{
        int16_t i;
        i=(((int16_t)buf[IP_TOTLEN_H_P])<<8)|(buf[IP_TOTLEN_L_P]&0xff);
        i-=IP_HEADER_LEN;
        i-=(buf[TCP_HEADER_LEN_P]>>4)*4; // generate len in bytes;
        if (i<=0){
                i=0;
        }
        return((uint16_t)i);
}


// fill in tcp data at position pos. pos=0 means start of
// tcp data. Returns the position at which the string after
// this string could be filled.
uint16_t fill_tcp_data_p(uint8_t *buf,uint16_t pos, const uint8_t *progmem_s)
{
        char c;
        // fill in tcp data at position pos
        //
        // with no options the data starts after the checksum + 2 more bytes (urgent ptr)
        while ((c = *(progmem_s++))) {
                buf[TCP_CHECKSUM_L_P+3+pos]=c;
                pos++;
        }
        return(pos);
}

uint16_t fill_tcp_data_string(uint8_t *buf,uint16_t pos, char *reply)
{
//        char c;
        // fill in tcp data at position pos
        //
        // with no options the data starts after the checksum + 2 more bytes (urgent ptr)
        while ( !(*reply == '\n') ) {
                buf[TCP_CHECKSUM_L_P+3+pos] = *reply;
                reply++;
                pos++;
        }
        return(pos);
}

// fill a binary string of len data into the tcp packet
uint16_t fill_tcp_data_len(uint8_t *buf,uint16_t pos, const uint8_t *s, uint8_t len)
{
        // fill in tcp data at position pos
        //
        // with no options the data starts after the checksum + 2 more bytes (urgent ptr)
        while (len) {
                buf[TCP_CHECKSUM_L_P+3+pos]=*s;
                pos++;
                s++;
                len--;
        }
        return(pos);
}

// fill in tcp data at position pos. pos=0 means start of
// tcp data. Returns the position at which the string after
// this string could be filled.
uint16_t fill_tcp_data(uint8_t *buf,uint16_t pos, const char *s)
{
        return(fill_tcp_data_len(buf,pos,(uint8_t*)s,strlen(s)));
}

// Make just an ack packet with no tcp data inside
// This will modify the eth/ip/tcp header
void make_tcp_ack_from_any(uint8_t *buf,int16_t datlentoack,uint8_t addflags)
{
        uint16_t j;
        make_eth(buf);
        // fill the header:
        buf[TCP_FLAGS_P]=TCP_FLAGS_ACK_V|addflags;
        if (addflags==TCP_FLAGS_RST_V){
                make_tcphead(buf,datlentoack,1);
        }else{
                if (datlentoack==0){
                        // if there is no data then we must still acknoledge one packet
                        datlentoack=1;
                }
                // normal case, ack the data:
                make_tcphead(buf,datlentoack,1); // no options
        }
        // total length field in the IP header must be set:
        // 20 bytes IP + 20 bytes tcp (when no options)
        j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN;
        buf[IP_TOTLEN_H_P]=j>>8;
        buf[IP_TOTLEN_L_P]=j& 0xff;
        make_ip(buf);
        // use a low window size otherwise we have to have
        // timers and can not just react on every packet.
        buf[TCP_WIN_SIZE]=0x4; // 1024=0x400, 1280=0x500 2048=0x800 768=0x300
        buf[TCP_WIN_SIZE+1]=0;
        // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
        j=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN,2);
        buf[TCP_CHECKSUM_H_P]=j>>8;
        buf[TCP_CHECKSUM_L_P]=j& 0xff;
        enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+ETH_HEADER_LEN,buf);
}


// dlen is the amount of tcp data (http data) we send in this packet
// You can use this function only immediately after make_tcp_ack_from_any
// This is because this function will NOT modify the eth/ip/tcp header except for
// length and checksum
// You must set TCP_FLAGS before calling this
void make_tcp_ack_with_data_noflags(uint8_t *buf,uint16_t dlen)
{
        uint16_t j;
        // total length field in the IP header must be set:
        // 20 bytes IP + 20 bytes tcp (when no options) + len of data
        j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlen;
        buf[IP_TOTLEN_H_P]=j>>8;
        buf[IP_TOTLEN_L_P]=j& 0xff;
        fill_ip_hdr_checksum(buf);
        // zero the checksum
        buf[TCP_CHECKSUM_H_P]=0;
        buf[TCP_CHECKSUM_L_P]=0;
        // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
        j=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+dlen,2);
        buf[TCP_CHECKSUM_H_P]=j>>8;
        buf[TCP_CHECKSUM_L_P]=j& 0xff;
        enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlen+ETH_HEADER_LEN,buf);
}

#if defined (UDP_server)
// a udp server
void make_udp_reply_from_request_udpdat_ready(uint8_t *buf,uint16_t datalen,uint16_t port)
{
        uint16_t j;
        make_eth(buf);
        if (datalen>220){
                datalen=220;
        }
        // total length field in the IP header must be set:
        j=IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
        buf[IP_TOTLEN_H_P]=j>>8;
        buf[IP_TOTLEN_L_P]=j& 0xff;
        make_ip(buf);
        // send to port:
        //buf[UDP_DST_PORT_H_P]=port>>8;
        //buf[UDP_DST_PORT_L_P]=port & 0xff;
        // sent to port of sender and use "port" as own source:
        buf[UDP_DST_PORT_H_P]=buf[UDP_SRC_PORT_H_P];
        buf[UDP_DST_PORT_L_P]= buf[UDP_SRC_PORT_L_P];
        buf[UDP_SRC_PORT_H_P]=port>>8;
        buf[UDP_SRC_PORT_L_P]=port & 0xff;
        // calculte the udp length:
        j=UDP_HEADER_LEN+datalen;
        buf[UDP_LEN_H_P]=j>>8;
        buf[UDP_LEN_L_P]=j& 0xff;
        // zero the checksum
        buf[UDP_CHECKSUM_H_P]=0;
        buf[UDP_CHECKSUM_L_P]=0;
        j=checksum(&buf[IP_SRC_P], 16 + datalen,1);
        buf[UDP_CHECKSUM_H_P]=j>>8;
        buf[UDP_CHECKSUM_L_P]=j& 0xff;
        enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

// you can send a max of 220 bytes of data because we use only one
// byte for the data but udp messages are normally small.
void make_udp_reply_from_request(uint8_t *buf,char *data,uint8_t datalen,uint16_t port)
{
        uint8_t i=0;
        // copy the data:
        while(i<datalen){
                buf[UDP_DATA_P+i]=data[i];
                i++;
        }
        make_udp_reply_from_request_udpdat_ready(buf,datalen,port);
}

#endif // UDP_server

#if defined (UDP_server) || defined (WWW_server)
// This initializes server
// you must call this function once before you use any of the other functions:
// mymac may be NULL and can be used if you did already call init_mac
void init_udp_or_www_server(uint8_t *mymac,uint8_t *myip){
        uint8_t i=0;
        if (myip){
                while(i<4){
                        ipaddr[i]=myip[i];
                        i++;
                }
        }
        if (mymac) init_mac(mymac);
}
#endif // UDP_server || WWW_server

#ifdef WWW_server
// not needed if you want port 80 (the default is port 80):
void www_server_port(uint16_t port){
        wwwport_h=(port>>8)&0xff;
        wwwport_l=(port&0xff);
}

// this is for the server not the client:
void make_tcp_synack_from_syn(uint8_t *buf)
{
        uint16_t ck;
        make_eth(buf);
        // total length field in the IP header must be set:
        // 20 bytes IP + 24 bytes (20tcp+4tcp options)
        buf[IP_TOTLEN_H_P]=0;
        buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4;
        make_ip(buf);
        buf[TCP_FLAGS_P]=TCP_FLAGS_SYNACK_V;
        make_tcphead(buf,1,0);
        // put an inital seq number
        buf[TCP_SEQ_H_P+0]= 0;
        buf[TCP_SEQ_H_P+1]= 0;
        // we step only the second byte, this allows us to send packts
        // with 255 bytes, 512  or 765 (step by 3) without generating
        // overlapping numbers.
        buf[TCP_SEQ_H_P+2]= seqnum;
        buf[TCP_SEQ_H_P+3]= 0;
        // step the inititial seq num by something we will not use
        // during this tcp session:
        seqnum+=3;
        // add an mss options field with MSS to 1280:
        // 1280 in hex is 0x500
        buf[TCP_OPTIONS_P]=2;
        buf[TCP_OPTIONS_P+1]=4;
        buf[TCP_OPTIONS_P+2]=0x05;
        buf[TCP_OPTIONS_P+3]=0x0;
        // The tcp header length is only a 4 bit field (the upper 4 bits).
        // It is calculated in units of 4 bytes.
        // E.g 24 bytes: 24/4=6 => 0x60=header len field
        buf[TCP_HEADER_LEN_P]=0x60;
        // here we must just be sure that the web browser contacting us
        // will send only one get packet
        buf[TCP_WIN_SIZE]=0x0a; // was 1400=0x578, 2560=0xa00 suggested by Andras Tucsni to be able to receive bigger packets
        buf[TCP_WIN_SIZE+1]=0; //
        // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + 4 (one option: mss)
        ck=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+4,2);
        buf[TCP_CHECKSUM_H_P]=ck>>8;
        buf[TCP_CHECKSUM_L_P]=ck& 0xff;
        // add 4 for option mss:
        enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4+ETH_HEADER_LEN,buf);
}

// you must have initialized info_data_len at some time before calling this function
//
// This info_data_len initialisation is done automatically if you call
// packetloop_icmp_tcp(buf,enc28j60PacketReceive(BUFFER_SIZE, buf));
// and test the return value for non zero.
//
// dlen is the amount of tcp data (http data) we send in this packet
// You can use this function only immediately after make_tcp_ack_from_any
// This is because this function will NOT modify the eth/ip/tcp header except for
// length and checksum
void www_server_reply(uint8_t *buf,uint16_t dlen)
{
        make_tcp_ack_from_any(buf,info_data_len,0); // send ack for http get
        // fill the header:
        // This code requires that we send only one data packet
        // because we keep no state information. We must therefore set
        // the fin here:
        buf[TCP_FLAGS_P]=TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V;//|TCP_FLAGS_FIN_V;
        make_tcp_ack_with_data_noflags(buf,dlen); // send data
}

#endif // WWW_server

#if defined (ALL_clients)
// fill buffer with a prog-mem string
void fill_buf_p(uint8_t *buf,uint16_t len, const char *progmem_s)
{
        uint8_t i=0;

        while (len){
                *buf= progmem_s[i];
                buf++;
                len--; i++;
        }
}
#endif

#ifdef PING_client
// icmp echo, matchpat is a pattern that has to be sent back by the
// host answering the ping.
// The ping is sent to destip  and mac dstmac
void client_icmp_request(uint8_t *buf,uint8_t *destip,uint8_t *dstmac)
{
        uint8_t i=0;
        uint16_t ck;
        //
        while(i<6){
                buf[ETH_DST_MAC +i]=dstmac[i]; // gw mac in local lan or host mac
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
        buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
        buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
        fill_buf_p(&buf[IP_P],9,iphdr);
        buf[IP_ID_L_P]=ipid; ipid++;
        buf[IP_TOTLEN_L_P]=0x54;
        buf[IP_PROTO_P]=IP_PROTO_ICMP_V;
        i=0;
        while(i<4){
                buf[IP_DST_P+i]=destip[i];
                buf[IP_SRC_P+i]=ipaddr[i];
                i++;
        }
        fill_ip_hdr_checksum(buf);
        buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREQUEST_V;
        buf[ICMP_TYPE_P+1]=0; // code
        // zero the checksum
        buf[ICMP_CHECKSUM_H_P]=0;
        buf[ICMP_CHECKSUM_L_P]=0;
        // a possibly unique id of this host:
        buf[ICMP_IDENT_H_P]=5; // some number
        buf[ICMP_IDENT_L_P]=ipaddr[3]; // last byte of my IP
        //
        buf[ICMP_IDENT_L_P+1]=0; // seq number, high byte
        buf[ICMP_IDENT_L_P+2]=1; // seq number, low byte, we send only 1 ping at a time
        // copy the data:
        i=0;
        while(i<56){
                buf[ICMP_DATA_P+i]=PINGPATTERN;
                i++;
        }
        //
        ck=checksum(&buf[ICMP_TYPE_P], 56+8,0);
        buf[ICMP_CHECKSUM_H_P]=ck>>8;
        buf[ICMP_CHECKSUM_L_P]=ck& 0xff;
        enc28j60PacketSend(98,buf);
}
#endif // PING_client


#ifdef NTP_client
// ntp udp packet
// See http://tools.ietf.org/html/rfc958 for details
//
void client_ntp_request(uint8_t *buf,uint8_t *ntpip,uint8_t srcport,uint8_t *dstmac)
{
        uint8_t i=0;
        uint16_t ck;
        //
        while(i<6){
                buf[ETH_DST_MAC +i]=dstmac[i]; // gw mac in local lan or host mac
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
        buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
        buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
        fill_buf_p(&buf[IP_P],9,iphdr);
        buf[IP_ID_L_P]=ipid; ipid++;
        buf[IP_TOTLEN_L_P]=0x4c;
        buf[IP_PROTO_P]=IP_PROTO_UDP_V;
        i=0;
        while(i<4){
                buf[IP_DST_P+i]=ntpip[i];
                buf[IP_SRC_P+i]=ipaddr[i];
                i++;
        }
        fill_ip_hdr_checksum(buf);
        buf[UDP_DST_PORT_H_P]=0;
        buf[UDP_DST_PORT_L_P]=0x7b; // ntp=123
        buf[UDP_SRC_PORT_H_P]=10;
        buf[UDP_SRC_PORT_L_P]=srcport; // lower 8 bit of src port
        buf[UDP_LEN_H_P]=0;
        buf[UDP_LEN_L_P]=56; // fixed len
        // zero the checksum
        buf[UDP_CHECKSUM_H_P]=0;
        buf[UDP_CHECKSUM_L_P]=0;
        // copy the data:
        i=0;
        // most fields are zero, here we zero everything and fill later
        while(i<48){
                buf[UDP_DATA_P+i]=0;
                i++;
        }
        fill_buf_p(&buf[UDP_DATA_P],10,ntpreqhdr);
        //
        ck=checksum(&buf[IP_SRC_P], 16 + 48,1);
        buf[UDP_CHECKSUM_H_P]=ck>>8;
        buf[UDP_CHECKSUM_L_P]=ck& 0xff;
        enc28j60PacketSend(90,buf);
}
// process the answer from the ntp server:
// if dstport==0 then accept any port otherwise only answers going to dstport
// return 1 on sucessful processing of answer
uint8_t client_ntp_process_answer(uint8_t *buf,uint32_t *time,uint8_t dstport_l){
        if (dstport_l){
                if (buf[UDP_DST_PORT_L_P]!=dstport_l){
                        return(0);
                }
        }
        if (buf[UDP_LEN_H_P]!=0 || buf[UDP_LEN_L_P]!=56 || buf[UDP_SRC_PORT_L_P]!=0x7b){
                // not ntp
                return(0);
        }
        // copy time from the transmit time stamp field:
        *time=((uint32_t)buf[0x52]<<24)|((uint32_t)buf[0x53]<<16)|((uint32_t)buf[0x54]<<8)|((uint32_t)buf[0x55]);
        return(1);
}
#endif

#ifdef UDP_client
// -------------------- send a spontanious UDP packet to a server
// There are two ways of using this:
// 1) you call send_udp_prepare, you fill the data yourself into buf starting at buf[UDP_DATA_P],
// you send the packet by calling send_udp_transmit
//
// 2) You just allocate a large enough buffer for you data and you call send_udp and nothing else
// needs to be done.
//
void send_udp_prepare(uint8_t *buf,uint16_t sport, const uint8_t *dip, uint16_t dport,const uint8_t *dstmac)
{
        uint8_t i=0;
        //
        while(i<6){
                buf[ETH_DST_MAC +i]=dstmac[i]; // gw mac in local lan or host mac
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
        buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
        buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
        fill_buf_p(&buf[IP_P],9,iphdr);
        buf[IP_ID_L_P]=ipid; ipid++;
        // total length field in the IP header must be set:
        buf[IP_TOTLEN_H_P]=0;
        // done in transmit: buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
        buf[IP_PROTO_P]=IP_PROTO_UDP_V;
        i=0;
        while(i<4){
                buf[IP_DST_P+i]=dip[i];
                buf[IP_SRC_P+i]=ipaddr[i];
                i++;
        }
        // done in transmit: fill_ip_hdr_checksum(buf);
        buf[UDP_DST_PORT_H_P]=(dport>>8);
        buf[UDP_DST_PORT_L_P]=0xff&dport;
        buf[UDP_SRC_PORT_H_P]=(sport>>8);
        buf[UDP_SRC_PORT_L_P]=sport&0xff;
        buf[UDP_LEN_H_P]=0;
        // done in transmit: buf[UDP_LEN_L_P]=UDP_HEADER_LEN+datalen;
        // zero the checksum
        buf[UDP_CHECKSUM_H_P]=0;
        buf[UDP_CHECKSUM_L_P]=0;
        // copy the data:
        // now starting with the first byte at buf[UDP_DATA_P]
        //
}

void send_udp_transmit(uint8_t *buf,uint16_t datalen)
{
        uint16_t tmp16;
        tmp16=IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
        buf[IP_TOTLEN_L_P]=tmp16& 0xff;
        buf[IP_TOTLEN_H_P]=tmp16>>8;
        fill_ip_hdr_checksum(buf);
        tmp16=UDP_HEADER_LEN+datalen;
        buf[UDP_LEN_L_P]=tmp16& 0xff;
        buf[UDP_LEN_H_P]=tmp16>>8;
        //
        tmp16=checksum(&buf[IP_SRC_P], 16 + datalen,1);
        buf[UDP_CHECKSUM_L_P]=tmp16& 0xff;
        buf[UDP_CHECKSUM_H_P]=tmp16>>8;
        enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

void send_udp(uint8_t *buf,char *data,uint8_t datalen,uint16_t sport, const uint8_t *dip, uint16_t dport,const uint8_t *dstmac)
{
        send_udp_prepare(buf,sport, dip, dport,dstmac);
        uint8_t i=0;
        // limit the length:
        if (datalen>220){
                datalen=220;
        }
        // copy the data:
        i=0;
        while(i<datalen){
                buf[UDP_DATA_P+i]=data[i];
                i++;
        }
        //
        send_udp_transmit(buf,datalen);
}
#endif // UDP_client

#ifdef WOL_client
// -------------------- special code to make a WOL packet

// A WOL (Wake on Lan) packet is a UDP packet to the broadcast
// address and UDP port 9. The data part contains 6x FF followed by
// 16 times the mac address of the host to wake-up
//
void send_wol(uint8_t *buf,uint8_t *wolmac)
{
        uint8_t i=0;
        uint8_t m=0;
        uint8_t pos=0;
        uint16_t ck;
        //
        while(i<6){
                buf[ETH_DST_MAC +i]=0xff;
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
        buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
        buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
        fill_buf_p(&buf[IP_P],9,iphdr);
        buf[IP_ID_L_P]=ipid; ipid++;
        buf[IP_TOTLEN_L_P]=0x82; //  fixed len
        buf[IP_PROTO_P]=IP_PROTO_UDP_V; // wol uses udp
        i=0;
        while(i<4){
                buf[IP_SRC_P+i]=ipaddr[i];
                buf[IP_DST_P+i]=0xff;
                i++;
        }
        fill_ip_hdr_checksum(buf);
        buf[UDP_DST_PORT_H_P]=0;
        buf[UDP_DST_PORT_L_P]=0x9; // wol=normally 9
        buf[UDP_SRC_PORT_H_P]=10;
        buf[UDP_SRC_PORT_L_P]=0x42; // source port does not matter
        buf[UDP_LEN_H_P]=0;
        buf[UDP_LEN_L_P]=110; // fixed len
        // zero the checksum
        buf[UDP_CHECKSUM_H_P]=0;
        buf[UDP_CHECKSUM_L_P]=0;
        // copy the data (102 bytes):
        i=0;
        while(i<6){
                buf[UDP_DATA_P+i]=0xff;
                i++;
        }
        m=0;
        pos=UDP_DATA_P+6;
        while (m<16){
                i=0;
                while(i<6){
                        buf[pos]=wolmac[i];
                        i++;
                        pos++;
                }
                m++;
        }
        //
        ck=checksum(&buf[IP_SRC_P], 16+ 102,1);
        buf[UDP_CHECKSUM_H_P]=ck>>8;
        buf[UDP_CHECKSUM_L_P]=ck& 0xff;
        enc28j60PacketSend(pos,buf);
}
#endif // WOL_client

#if defined GRATARP
// Send a Gratuitous arp, this is to refresh the arp
// cash of routers and switches. It can improve the response
// time in wifi networks as some wifi equipment expects the initial
// communication to not start from the network side. That is wrong
// but some consumer devices are made like this.
//
// A Gratuitous ARP can be a request or a reply.
// A request frame is as well called Unsolicited ARP
uint8_t gratutious_arp(uint8_t *buf)
{
        uint8_t i=0;
        if (!enc28j60linkup()){
                return(0);
        }
        //
        while(i<6){
                buf[ETH_DST_MAC +i]=0xff;
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
        buf[ETH_TYPE_H_P] = ETHTYPE_ARP_H_V;
        buf[ETH_TYPE_L_P] = ETHTYPE_ARP_L_V;
        // arp request and reply are the same execept for
        // the opcode:
        fill_buf_p(&buf[ETH_ARP_P],8,arpreqhdr);
        //buf[ETH_ARP_OPCODE_L_P]=ETH_ARP_OPCODE_REPLY_L_V; // reply
        i=0;
        while(i<6){
                buf[ETH_ARP_SRC_MAC_P +i]=macaddr[i];
                buf[ETH_ARP_DST_MAC_P+i]=0xff;
                i++;
        }
        i=0;
        while(i<4){
                buf[ETH_ARP_DST_IP_P+i]=ipaddr[i];
                buf[ETH_ARP_SRC_IP_P+i]=ipaddr[i];
                i++;
        }
        // 0x2a=42=len of packet
        enc28j60PacketSend(0x2a,buf);
        return(1);
}
#endif // GRATARP

#if ARP_MAC_resolver_client
// make a arp request
// Note: you must have initialized the stack with
// init_udp_or_www_server or client_ifconfig
// before you can use this function
void client_arp_whohas(uint8_t *buf,uint8_t *ip_we_search)
{
        uint8_t i=0;
        if (ipaddr[0]==0) return; // error ipaddr not set
        //
        while(i<6){
                buf[ETH_DST_MAC +i]=0xff;
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
        buf[ETH_TYPE_H_P] = ETHTYPE_ARP_H_V;
        buf[ETH_TYPE_L_P] = ETHTYPE_ARP_L_V;
        fill_buf_p(&buf[ETH_ARP_P],8,arpreqhdr);
        i=0;
        while(i<6){
                buf[ETH_ARP_SRC_MAC_P +i]=macaddr[i];
                buf[ETH_ARP_DST_MAC_P+i]=0;
                i++;
        }
        i=0;
        while(i<4){
                buf[ETH_ARP_DST_IP_P+i]=*(ip_we_search +i);
                buf[ETH_ARP_SRC_IP_P+i]=ipaddr[i];
                i++;
        }
        // 0x2a=42=len of packet
        enc28j60PacketSend(0x2a,buf);
}

// return zero when current transaction is finished
uint8_t get_mac_with_arp_wait(void)
{
        if (arpip_state == WGW_HAVE_MAC){
                return(0);
        }
        return(1);
}

// reference_number is something that is just returned in the callback
// to make matching and waiting for a given ip/mac address pair easier
// Note: you must have initialized the stack with
// init_udp_or_www_server or client_ifconfig
// before you can use this function
void get_mac_with_arp(uint8_t *ip, uint8_t reference_number,void (*arp_result_callback)(uint8_t *ip,uint8_t reference_number,uint8_t *mac))
{
        uint8_t i=0;
        client_arp_result_callback=arp_result_callback;
        arpip_state=WGW_INITIAL_ARP; // causes an arp request in the packet loop
        arp_reference_number=reference_number;
        while(i<4){
                arpip[i]=ip[i];
                i++;
        }
}
#endif

#if defined (TCP_client)
// Make a tcp syn packet
void tcp_client_syn(uint8_t *buf,uint8_t srcport,uint16_t dstport)
{
        uint16_t ck;
        uint8_t i=0;
        // -- make the main part of the eth/IP/tcp header:
        while(i<6){
                buf[ETH_DST_MAC +i]=tcp_dst_mac[i]; // gw mac in local lan or host mac
                buf[ETH_SRC_MAC +i]=macaddr[i];
                i++;
        }
        buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
        buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
        fill_buf_p(&buf[IP_P],9,iphdr);
        buf[IP_TOTLEN_L_P]=44; // good for syn
        buf[IP_ID_L_P]=ipid; ipid++;
        buf[IP_PROTO_P]=IP_PROTO_TCP_V;
        i=0;
        while(i<4){
                buf[IP_DST_P+i]=tcp_otherside_ip[i];
                buf[IP_SRC_P+i]=ipaddr[i];
                i++;
        }
        fill_ip_hdr_checksum(buf);
        buf[TCP_DST_PORT_H_P]=(dstport>>8)&0xff;
        buf[TCP_DST_PORT_L_P]=(dstport&0xff);
        buf[TCP_SRC_PORT_H_P]=TCPCLIENT_SRC_PORT_H;
        buf[TCP_SRC_PORT_L_P]=srcport; // lower 8 bit of src port
        i=0;
        // zero out sequence number and acknowledgement number
        while(i<8){
                buf[TCP_SEQ_H_P+i]=0;
                i++;
        }
        // -- header ready
        // put inital seq number
        // we step only the second byte, this allows us to send packts
        // with 255 bytes 512 (if we step the initial seqnum by 2)
        // or 765 (step by 3)
        buf[TCP_SEQ_H_P+2]= seqnum;
        // step the inititial seq num by something we will not use
        // during this tcp session:
        seqnum+=3;
        buf[TCP_HEADER_LEN_P]=0x60; // 0x60=24 len: (0x60>>4) * 4
        buf[TCP_FLAGS_P]=TCP_FLAGS_SYN_V;
        // use a low window size otherwise we have to have
        // timers and can not just react on every packet.
        buf[TCP_WIN_SIZE]=0x3; // 1024=0x400 768=0x300, initial window
        buf[TCP_WIN_SIZE+1]=0x0;
        // zero the checksum
        buf[TCP_CHECKSUM_H_P]=0;
        buf[TCP_CHECKSUM_L_P]=0;
        // urgent pointer
        buf[TCP_CHECKSUM_L_P+1]=0;
        buf[TCP_CHECKSUM_L_P+2]=0;
        // MSS= max IP len that we want to have:
        buf[TCP_OPTIONS_P]=2;
        buf[TCP_OPTIONS_P+1]=4;
        buf[TCP_OPTIONS_P+2]=(CLIENTMSS>>8);
        buf[TCP_OPTIONS_P+3]=CLIENTMSS & 0xff;
        ck=checksum(&buf[IP_SRC_P], 8 +TCP_HEADER_LEN_PLAIN+4,2);
        buf[TCP_CHECKSUM_H_P]=ck>>8;
        buf[TCP_CHECKSUM_L_P]=ck& 0xff;
        // 4 is the tcp mss option:
        enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+ETH_HEADER_LEN+4,buf);
}
#endif // TCP_client

#if defined (TCP_client)
// This is how to use the tcp client:
//
// Declare a callback function to get the result (tcp data from the server):
//
// uint8_t your_client_tcp_result_callback(uint8_t fd, uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data){...your code;return(close_tcp_session);}
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
// We use callback functions because that saves memory and a uC is very
// limited in memory
//
uint8_t client_tcp_req(uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),uint16_t (*datafill_callback)(uint8_t fd),uint16_t port,uint8_t *dstip,uint8_t *dstmac)
{
        uint8_t i=0;
        client_tcp_result_callback=result_callback;
        client_tcp_datafill_callback=datafill_callback;
        while(i<4){tcp_otherside_ip[i]=dstip[i];i++;}
        i=0;
        while(i<6){tcp_dst_mac[i]=dstmac[i];i++;}
        tcp_client_port=port;
        tcp_client_state=1; // send a syn
        tcp_fd++;
        if (tcp_fd>7){
                tcp_fd=0;
        }
        return(tcp_fd);
}
#endif //  TCP_client

#if defined (WWW_client)
uint16_t www_client_internal_datafill_callback(uint8_t fd){
        char strbuf[5];
        uint16_t len=0;
        if (fd==www_fd){
                if (browsertype==0){
                        // GET
                        len=fill_tcp_data_p(bufptr,0,PSTR("GET "));
                        len=fill_tcp_data_p(bufptr,len,client_urlbuf);
                        len=fill_tcp_data(bufptr,len,client_urlbuf_var);
                        // I would prefer http/1.0 but there is a funny
                        // bug in some apache webservers which causes
                        // them to send two packets (fragmented PDU)
                        // if we don't use HTTP/1.1 + Connection: close
                        len=fill_tcp_data_p(bufptr,len,PSTR(" HTTP/1.1\r\nHost: "));
                        len=fill_tcp_data(bufptr,len,client_hoststr);
                        len=fill_tcp_data_p(bufptr,len,PSTR("\r\nUser-Agent: tgr/1.1\r\nAccept: text/html\r\nConnection: close\r\n\r\n"));
                }else{
                        // POST
                        len=fill_tcp_data_p(bufptr,0,PSTR("POST "));
                        len=fill_tcp_data_p(bufptr,len,client_urlbuf);
                        len=fill_tcp_data(bufptr,len,client_urlbuf_var);
                        len=fill_tcp_data_p(bufptr,len,PSTR(" HTTP/1.1\r\nHost: "));
                        len=fill_tcp_data(bufptr,len,client_hoststr);
                        if (client_additionalheaderline){
                                len=fill_tcp_data_p(bufptr,len,PSTR("\r\n"));
                                len=fill_tcp_data_p(bufptr,len,client_additionalheaderline);
                        }
                        len=fill_tcp_data_p(bufptr,len,PSTR("\r\nUser-Agent: tgr/1.1\r\nAccept: */*\r\nConnection: close\r\n"));
                        len=fill_tcp_data_p(bufptr,len,PSTR("Content-Length: "));
                        itoa(strlen(client_postval),strbuf,10);
                        len=fill_tcp_data(bufptr,len,strbuf);
                        len=fill_tcp_data_p(bufptr,len,PSTR("\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n"));
                        len=fill_tcp_data(bufptr,len,client_postval);
                }
                return(len);
        }
        return(0);
}

uint8_t www_client_internal_result_callback(uint8_t fd, uint8_t statuscode, uint16_t datapos, uint16_t len_of_data){
        uint16_t web_statuscode=0; // tcp status is OK but we need to check http layer too
        uint8_t i=0;
        if (fd!=www_fd){
                (*client_browser_callback)(500,0,0);
                return(0);
        }
        if (statuscode==0 && len_of_data>12){
                // we might have a http status code
                // http status codes are 3digit numbers as ascii text. See http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
                // The buffer would look like this: HTTP/1.1 200 OK\r\n
                // web_statuscode=0 means we got a corrupted answer
                if (client_browser_callback){
                        if (isblank(bufptr[datapos+8]) && isdigit(bufptr[datapos+9])&& isdigit(bufptr[datapos+11])){ // e.g 200 OK, a status code has 3 digits from datapos+9 to datapos+11, copy over the web/http status code to web_statuscode:
                                while(i<2){
                                        web_statuscode+=bufptr[datapos+9+i]-'0';
                                        web_statuscode*=10;
                                        i++;
                                }
                                web_statuscode+=bufptr[datapos+11]-'0';
                        }
                        //(*client_browser_callback)(web_statuscode,((uint16_t)TCP_SRC_PORT_H_P+(bufptr[TCP_HEADER_LEN_P]>>4)*4),len_of_data);
                        (*client_browser_callback)(web_statuscode,datapos,len_of_data);
                }
        }
        return(0);
}

// call this function externally like this:
//
// Declare a callback function: void browserresult(uint8_t webstatuscode,uint16_t datapos,uint16_t len){...your code}
// The variable datapos is the index in the packet buffer.
// Now call client_browser_url:
// client_browser_url(PSTR("/cgi-bin/checkip"),NULL,"tuxgraphics.org",&browserresult,other_side_ip,gwmac);
// urlbuf_varpart is a pointer to a string buffer that contains the second
// non constant part of the url. You must keep this buffer allocated until the
// callback function is executed or until you can be sure that the server side
// has timed out.
// hoststr is the name of the host. This is needed because many sites host several
// sites on the same physical machine with only one IP address. The web server needs
// to know to which site you want to go.
// webstatuscode is zero if there was no proper reply from the server (garbage message total communication failure, this is rare).
// webstatuscode is the http status code (e.g webstatuscode=200 for 200 OK);
// webstatuscode is zero if there was a garbage answer received from the server.
// For possible status codes look at http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
// Basically 2xx is success and any 5xx, 4xx is a failure.
// The string buffers to which urlbuf_varpart and hoststr are pointing
// must not be changed until the callback is executed.
//
void client_browse_url(const prog_char *urlbuf,const char *urlbuf_varpart,const char *hoststr,void (*callback)(uint16_t,uint16_t,uint16_t),uint8_t *dstip,uint8_t *dstmac)
{
        if (!enc28j60linkup())return;
        client_urlbuf=urlbuf;
        client_urlbuf_var=urlbuf_varpart;
        client_hoststr=hoststr;
        browsertype=0;
        client_browser_callback=callback;
        www_fd=client_tcp_req(&www_client_internal_result_callback,&www_client_internal_datafill_callback,80,dstip,dstmac);
}

// client web browser using http POST operation:
// additionalheaderline must be set to NULL if not used.
// The string buffers to which urlbuf_varpart and hoststr are pointing
// must not be changed until the callback is executed.
// postval is a string buffer which can only be de-allocated by the caller
// when the post operation was really done (e.g when callback was executed).
// postval must be urlencoded.
void client_http_post(const prog_char *urlbuf, const char *urlbuf_varpart,const char *hoststr, const prog_char *additionalheaderline,char *postval,void (*callback)(uint16_t,uint16_t,uint16_t),uint8_t *dstip,uint8_t *dstmac)
{
        if (!enc28j60linkup())return;
        client_urlbuf=urlbuf;
        client_hoststr=hoststr;
        client_urlbuf_var=urlbuf_varpart;
        client_additionalheaderline=additionalheaderline;
        client_postval=postval;
        browsertype=1;
        client_browser_callback=callback;
        www_fd=client_tcp_req(&www_client_internal_result_callback,&www_client_internal_datafill_callback,80,dstip,dstmac);
}
#endif // WWW_client

void register_ping_rec_callback(void (*callback)(uint8_t *srcip))
{
        icmp_callback=callback;
}

#ifdef PING_client
// loop over this to check if we get a ping reply:
uint8_t packetloop_icmp_checkreply(uint8_t *buf,uint8_t *ip_monitoredhost)
{
        if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREPLY_V){
                if (buf[ICMP_DATA_P]== PINGPATTERN){
                        if (check_ip_message_is_from(buf,ip_monitoredhost)){
                                return(1);
                                // ping reply is from monitored host and ping was from us
                        }
                }
        }
        return(0);
}
#endif // PING_client


// return 0 to just continue in the packet loop and return the position
// of the tcp data if there is tcp data part
uint16_t packetloop_arp_icmp_tcp(uint8_t *buf,uint16_t plen)
{
//        uint16_t len;
#if defined (TCP_client)
        uint8_t send_fin=0;
        uint16_t tcpstart;
        uint16_t save_len;
#endif
#ifdef ARP_MAC_resolver_client
        //plen will be unequal to zero if there is a valid
        // packet (without crc error):
        if(plen==0){
                if (arpip_state == (WGW_ACCEPT_ARP_REPLY|WGW_INITIAL_ARP) && arp_delaycnt==0 ){
                        // arp_delaycnt has wrapped no arp reply yet
                        if (enc28j60linkup()) client_arp_whohas(buf,arpip);
                }
                if (arpip_state == WGW_INITIAL_ARP && enc28j60linkup()){
                        client_arp_whohas(buf,arpip);
                        arpip_state|=WGW_ACCEPT_ARP_REPLY; // WGW_INITIAL_ARP and WGW_ACCEPT_ARP_REPLY set
                        arp_delaycnt=0; // this is like a timer, not so precise but good enough, it wraps in about 2 sec
                }
                arp_delaycnt++;
#if defined (TCP_client)
                if (tcp_client_state==1 && enc28j60linkup()){ // send a syn
                        tcp_client_state=2;
                        tcpclient_src_port_l++; // allocate a new port
                        // we encode our 3 bit fd into the src port this
                        // way we get it back in every message that comes
                        // from the server:
                        tcp_client_syn(buf,((tcp_fd<<5) | (0x1f & tcpclient_src_port_l)),tcp_client_port);
                }
#endif
                return(0);
        }
#endif // ARP_MAC_resolver_client
        // arp is broadcast if unknown but a host may also
        // verify the mac address by sending it to
        // a unicast address.
        if(eth_type_is_arp_and_my_ip(buf,plen)){
                if (buf[ETH_ARP_OPCODE_L_P]==ETH_ARP_OPCODE_REQ_L_V){
                        // is it an arp request
                        make_arp_answer_from_request(buf);
                }
#ifdef ARP_MAC_resolver_client
                if ((arpip_state & WGW_ACCEPT_ARP_REPLY) && (buf[ETH_ARP_OPCODE_L_P]==ETH_ARP_OPCODE_REPLY_L_V)){
                        // is it an arp reply
                        if (memcmp(&buf[ETH_ARP_SRC_IP_P],arpip,4)!=0) return(0); // not an arp reply for the IP we were searching
                        (*client_arp_result_callback)(arpip,arp_reference_number,buf+ETH_ARP_SRC_MAC_P);
                        arpip_state=WGW_HAVE_MAC;
                }
#endif // ARP_MAC_resolver_client
                return(0);

        }
        // check if ip packets are for us:
        if(eth_type_is_ip_and_my_ip(buf,plen)==0){
                return(0);
        }
        if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V){
                if (icmp_callback){
                        (*icmp_callback)(&(buf[IP_SRC_P]));
                }
                // a ping packet, let's send pong
                debugstr("Sending ping reply...");
                debugcrlf();
                make_echo_reply_from_request(buf,plen);
                return(0);
        }
        if (plen<54 && buf[IP_PROTO_P]!=IP_PROTO_TCP_V ){
                // smaller than the smallest TCP packet and not tcp port
                return(0);
        }
#if defined (TCP_client)
        // a message for the tcp client, tcp_client_state is zero if client was never used
        if ( buf[TCP_DST_PORT_H_P]==TCPCLIENT_SRC_PORT_H){
#if defined (WWW_client)
                // workaround to pass pointer to www_client_internal..
                bufptr=buf;
#endif // WWW_client
                if (check_ip_message_is_from(buf,tcp_otherside_ip)==0){
                        return(0);
                }
                // if we get a reset:
                if (buf[TCP_FLAGS_P] & TCP_FLAGS_RST_V){
                        if (client_tcp_result_callback){
                                // parameters in client_tcp_result_callback: fd, status, buf_start, len
                                (*client_tcp_result_callback)((buf[TCP_DST_PORT_L_P]>>5)&0x7,3,0,0);
                        }
                        tcp_client_state=5;
                        return(0);
                }
                len=get_tcp_data_len(buf);
                if (tcp_client_state==2){
                        if ((buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V) && (buf[TCP_FLAGS_P] &TCP_FLAGS_ACK_V)){
                                // synack, answer with ack
                                make_tcp_ack_from_any(buf,0,0);
                                buf[TCP_FLAGS_P]=TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V;

                                // Make a tcp message with data. When calling this function we must
                                // still have a valid tcp-ack in the buffer. In other words
                                // you have just called make_tcp_ack_from_any(buf,0).
                                if (client_tcp_datafill_callback){
                                        // in this case it is src port because the above
                                        // make_tcp_ack_from_any swaps the dst and src port:
                                        len=(*client_tcp_datafill_callback)((buf[TCP_SRC_PORT_L_P]>>5)&0x7);
                                }else{
                                        // this is just to prevent a crash
                                        len=0;
                                }
                                tcp_client_state=3;
                                make_tcp_ack_with_data_noflags(buf,len);
                                return(0);
                        }else{
                                // reset only if we have sent a syn and don't get syn-ack back.
                                // If we connect to a non listen port then we get a RST
                                // which will be handeled above. In other words there is
                                // normally no danger for an endless loop.
                                tcp_client_state=1; // retry
                                // do not inform application layer as we retry.
                                len++;
                                if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V){
                                        // if packet was an ack then do not step the ack number
                                        len=0;
                                }
                                // refuse and reset the connection
                                make_tcp_ack_from_any(buf,len,TCP_FLAGS_RST_V);
                                return(0);
                        }
                }
                // in tcp_client_state==3 we will normally first get an empty
                // ack-packet and then a ack-packet with data.
                if (tcp_client_state==3 && len>0){
                        // our first real data packet
                        tcp_client_state=4;
                        // return the data we received
                        if (client_tcp_result_callback){
                                tcpstart=TCP_DATA_START; // TCP_DATA_START is a formula
                                // out of buffer bounds check, needed in case of fragmented IP packets
                                if (tcpstart>plen-8){
                                        tcpstart=plen-8; // dummy but save
                                }
                                save_len=len;
                                if (tcpstart+len>plen){
                                        save_len=plen-tcpstart;
                                }
                                send_fin=(*client_tcp_result_callback)((buf[TCP_DST_PORT_L_P]>>5)&0x7,0,tcpstart,save_len);
                        }
                        if (send_fin){
                                make_tcp_ack_from_any(buf,len,TCP_FLAGS_PUSH_V|TCP_FLAGS_FIN_V);
                                tcp_client_state=5;
                                return(0);
                        }
                }
                if(tcp_client_state==5){
                        // no more ack
                        return(0);
                }
                if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V){
                        make_tcp_ack_from_any(buf,len+1,TCP_FLAGS_PUSH_V|TCP_FLAGS_FIN_V);
                        tcp_client_state=5; // connection terminated
                        return(0);
                }
                // ack all data (the web page may be long):
                // if we just get a fragment then len will be zero
                // and we ack only once we have the full packet
                if (len>0){
                        make_tcp_ack_from_any(buf,len,0);
                }
                return(0);
        }
#endif // TCP_client
        //
#ifdef WWW_server
        // tcp port web server start
        if (buf[IP_PROTO_P]==IP_PROTO_TCP_V && buf[TCP_DST_PORT_H_P]==wwwport_h && buf[TCP_DST_PORT_L_P]==wwwport_l){
                if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V){
                        make_tcp_synack_from_syn(buf);
                        // make_tcp_synack_from_syn does already send the syn,ack
                        return(0);
                }
                if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V){
                        info_data_len=get_tcp_data_len(buf);
                        // we can possibly have no data, just ack:
                        // Here we misuse plen for something else to save a variable.
                        // plen is now the position of start of the tcp user data.
                        if (info_data_len==0){
                                if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V){
                                        // finack, answer with ack
                                        make_tcp_ack_from_any(buf,0,0);
                                }
                                // just an ack with no data, wait for next packet
                                return(0);
                        }
                        // Here we misuse len for something else to save a variable
                        len=TCP_DATA_START; // TCP_DATA_START is a formula
                        // check for data corruption
                        if (len>plen-8){
                                return(0);
                        }
                        return(len);
                }
        }
#endif // WWW_server
        return(0);
}
/* end of ip_arp_udp.c */
