/*
* E131.c
*
* Project: E131 - E.131 (sACN) library for Arduino
* Copyright (c) 2015 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#include "E131.h"
#include <string.h>
#include "lwip\netif.h"


/* E1.17 ACN Packet Identifier */


/* Constructor */
void E131_init()
{

    memset(pbuff1.raw, 0, sizeof(pbuff1.raw));
    memset(pbuff2.raw, 0, sizeof(pbuff2.raw));
    packet = &pbuff1;
    pwbuff = &pbuff2;

    sequence = 0;
    stats.num_packets = 0;
    stats.sequence_errors = 0;
    stats.packet_errors = 0;

}

void initUnicast() {
    //delay(100);
	conn = netconn_new(NETCONN_UDP);
	err = netconn_bind(conn, IP_ADDR_ANY, 5568);

	if(err != ERR_OK)
	{
		PRINTF("NETCONN BIND FAIL\r\n");
		return;
	}

    PRINTF("- Unicast port: %d\r\n", E131_DEFAULT_PORT);

}

void initMulticast(uint16_t universe, uint8_t n) {
    //delay(100);
	ip4_addr_t address;
	IP4_ADDR(&address, 239, 255, ((universe >> 8) & 0xff), ((universe >> 0) & 0xff));

	ip4_addr_t *ifaddr;
	ip4_addr_t multicast_addr;

    netconn_getaddr(conn, ifaddr, NULL, 1);	// Save Local IP of the Netconn "conn" to ifaddr

    for (uint8_t i = 1; i < n; i++) {
    	IP4_ADDR(&multicast_addr, 239, 255, (((universe + i) >> 8) & 0xff), (((universe + i) >> 0) & 0xff));

        igmp_joingroup(ifaddr, &multicast_addr);
    }

    //TODO udp.beginMulticast(WiFi.localIP(), address, E131_DEFAULT_PORT);


    PRINTF("- Universe: ");
    PRINTF("%s\r\n", universe);
    PRINTF("- Multicast address: ");
    PRINTF(" %u.%u.%u.%u\r\n", ((u8_t *)&address)[0], ((u8_t *)&address)[1],
           ((u8_t *)&address)[2], ((u8_t *)&address)[3]);

}

void E131_begin(e131_listen_t type, uint16_t universe, uint8_t n) {
    if (type == E131_UNICAST)
        initUnicast();
    if (type == E131_MULTICAST)
        initMulticast(universe, n);
}


void dumpError(e131_error_t error) {
    switch (error) {
    	case ERROR_NONE:
    		break;
        case ERROR_ACN_ID:
        	PRINTF("INVALID PACKET ID: ");
            for (uint8_t i = 0; i < sizeof(ACN_ID); i++)
            	PRINTF("%02X", pwbuff->acn_id[i]);
            PRINTF("\r\n");
            break;
        case ERROR_PACKET_SIZE:
        	PRINTF("INVALID PACKET SIZE: \r\n");
            break;
        case ERROR_VECTOR_ROOT:
        	PRINTF("INVALID ROOT VECTOR: 0x \r\n");
            //Serial.println(htonl(pwbuff->root_vector), HEX);
            break;
        case ERROR_VECTOR_FRAME:
        	PRINTF("INVALID FRAME VECTOR: 0x");
            //Serial.println(htonl(pwbuff->frame_vector), HEX);
            break;
        case ERROR_VECTOR_DMP:
        	PRINTF("INVALID DMP VECTOR: 0x");
        	PRINTF("%02X\r\n", pwbuff->dmp_vector);
    }
}

uint16_t E131_parsePacket()
{
    e131_error_t error;
    uint16_t retval = 0;
    int size = 0;

    err = netconn_recv(conn, &buf);


    if(netbuf_copy(buf, pwbuff->raw, sizeof(pwbuff->raw)) != buf->p->tot_len) {
    	LWIP_DEBUGF(LWIP_DBG_ON, ("netbuf_copy failed\n"));
    	PRINTF("Fail\r\n");
    }
    else
    {
    	//pwbuff[buf->p->tot_len] = '\0';
    	size = buf->p->tot_len;

    	/* PRINTF("Size of Buffer: %d\r\n", buf->p->tot_len);
    	       for(int i = 0; i < buf->p->tot_len; i++)
    	        	PRINTF("%02X",buffer->raw[i]);
    	PRINTF("\r\n");
    	*/
    }


    if (size)
    {
    	//pwbuff->raw = buffer;
    	error = validate();
        if (!error)
        {
            e131_packet_t *swap = packet;
            packet = pwbuff;
            pwbuff = swap;

            universe = htons(packet->universe);
            data = packet->property_values + 1;
            retval = htons(packet->property_value_count) - 1;
            if (packet->sequence_number != sequence++)
            {
                stats.sequence_errors++;
                sequence = packet->sequence_number + 1;
            }
            stats.num_packets++;

        }
        else
        {
            //dumpError(error);
            stats.packet_errors++;
        }

        PRINTF("PR: %d   PE: %d     SE: %d\r", stats.num_packets, stats.packet_errors, stats.sequence_errors);

    }

    netbuf_delete(buf);

    return retval;
}

e131_error_t validate()
{
	if (memcmp(pwbuff->acn_id, ACN_ID, sizeof(pwbuff->acn_id)))
		return ERROR_ACN_ID;
	if (htonl(pwbuff->root_vector) != VECTOR_ROOT)
		return ERROR_VECTOR_ROOT;
	if (htonl(pwbuff->frame_vector) != VECTOR_FRAME)
		return ERROR_VECTOR_FRAME;
	if (pwbuff->dmp_vector != VECTOR_DMP)
		return ERROR_VECTOR_DMP;
	return ERROR_NONE;
}

