/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright:LGPL V2
 * See http://www.gnu.org/licenses/old-licenses/lgpl-2.0.html
 *
 * This file can be used to decide which functionallity of the
 * TCP/IP stack shall be available. By picking the right functions
 * you can significantly reduce the size of the resulting code.
 *
 *********************************************/
//@{
#ifndef IP_CONFIG_H
#define IP_CONFIG_H

//------------- functions in ip_arp_udp_tcp.c --------------
// an NTP client (ntp clock):
#undef NTP_client
// a spontanious sending UDP client (needed as well for DNS and DHCP)
#undef UDP_client
// a server answering to UDP messages
#define UDP_server
// a web server
#undef WWW_server

// to send out a ping:
#undef PING_client
#define PINGPATTERN 0x42

// a UDP wake on lan sender:
#undef WOL_client

// function to send a gratuitous arp
#undef GRATARP

// a "web browser". This can be use to upload data
// to a web server on the internet by encoding the data
// into the url (like a Form action of type GET):
#undef WWW_client
// if you do not need a browser and just a server:
//#undef WWW_client
//
//------------- functions in websrv_help_functions.c --------------
//
// functions to decode cgi-form data:
#undef FROMDECODE_websrv_help

// function to encode a URL (mostly needed for a web client)
#undef URLENCODE_websrv_help

#endif /* IP_CONFIG_H */
//@}
