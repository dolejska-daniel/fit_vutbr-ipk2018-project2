// dhcp.c
// IPK-PROJ2, 07.04.2018
// Author: Daniel Dolejska, FIT

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include "dhcp.h"
#include "general.h"
#include "network.h"

#ifdef DEBUG_PRINT_ENABLED
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif // DEBUG_PRINT_ENABLED

#ifdef DEBUG_LOG_ENABLED
#define DEBUG_LOG(...) do{ fprintf( stderr, "[%s]     %s\n", __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_LOG(...) do{ } while ( 0 )
#endif // DEBUG_LOG_ENABLED

#ifdef DEBUG_ERR_ENABLED
#define DEBUG_ERR(...) do{ fprintf( stderr, "[%s] ERR %s\n", __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_ERR(...) do{ } while ( 0 )
#endif // DEBUG_ERR_ENABLED

#define ERR(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#define OUTPUT(...) do{ fprintf( stdout, __VA_ARGS__ ); } while( 0 )

struct dhcp_packet *get_dhcp_data( uint8_t *packet )
{
    return (struct dhcp_packet *) (packet + get_header_sizes());
}

size_t get_dhcp_data_size()
{
    return sizeof(struct dhcp_packet);
}

void print_dhcp_data( const struct dhcp_packet *dhcp_data )
{
#ifdef DEBUG_PRINT_ENABLED
    fprintf(
            stderr,
            "DHCP_DATA : {\n\top\t%d\n\thtype\t%d\n\thlen\t%d\n\thops\t%d\n\txid\t%#x\n\tsecs\t%d\n\tflags\t%#x\n\tciaddr\t%#x\n\tyiaddr\t%#x\n\tsiaddr\t%#x\n\tgiaddr\t%#x\n\tchaddr\t%X:%X:%X:%X:%X:%X,\n\tmagic_cookie\t%#x\n}\n",
            dhcp_data->op,
            dhcp_data->htype,
            dhcp_data->hlen,
            dhcp_data->hops,
            dhcp_data->xid,
            dhcp_data->secs,
            dhcp_data->flags,
            dhcp_data->ciaddr,
            dhcp_data->yiaddr,
            dhcp_data->siaddr,
            dhcp_data->giaddr,
            dhcp_data->chaddr[0],
            dhcp_data->chaddr[1],
            dhcp_data->chaddr[2],
            dhcp_data->chaddr[3],
            dhcp_data->chaddr[4],
            dhcp_data->chaddr[5],
            dhcp_data->magic_cookie
    );
#endif
}

void dhcp_encaps( uint8_t *packet, uint16_t *packet_len, const uint8_t *mac, uint8_t dhcp_op, uint32_t dhcp_xid )
{
    DEBUG_LOG("DHCP-ENCAPS", "Creating DHCP contents...");
    struct dhcp_packet *dhcp_data = get_dhcp_data(packet);

    dhcp_data->op     = dhcp_op;
    dhcp_data->htype  = HTYPE_ETHER;
    dhcp_data->hlen   = 6;
    dhcp_data->hops   = 0;
    dhcp_data->xid    = dhcp_xid ? dhcp_xid : (uint32_t) create_random_number(UINT32_MAX);
    dhcp_data->secs   = 0;
    dhcp_data->flags  = BOOTP_BROADCAST;
    dhcp_data->ciaddr = 0;
    dhcp_data->yiaddr = 0;
    dhcp_data->siaddr = 0;
    dhcp_data->giaddr = 0;

    dhcp_data->chaddr[0] = mac[0];
    dhcp_data->chaddr[1] = mac[1];
    dhcp_data->chaddr[2] = mac[2];
    dhcp_data->chaddr[3] = mac[3];
    dhcp_data->chaddr[4] = mac[4];
    dhcp_data->chaddr[5] = mac[5];

    //dhcp_data->sname;
    //dhcp_data->file;
    dhcp_data->magic_cookie = (uint32_t) DHCP_OPTIONS_COOKIE; // 0x63825363

    *packet_len+= get_dhcp_data_size();

    DEBUG_LOG("DHCP-ENCAPS", "DHCP contents created...");
    DEBUG_PRINT("\tpacket_length (local): %hu (added %d)\n", (unsigned short) *packet_len, get_dhcp_data_size());
    print_dhcp_data(dhcp_data);
}

void dhcp_add_option( uint8_t *packet, uint16_t *packet_len, uint8_t option_type, uint8_t *option_data, uint8_t option_size )
{
    DEBUG_LOG("REQ-ADDOPT", "Adding DHCP option:");
    DEBUG_PRINT("\ttype:\t%d\n\tdata:\t%#x\n\tsize:\t%d\n", option_type, *((char *) option_data), option_size);

    if (option_type != 0)
    {
        memcpy(&packet[*packet_len], &option_type, sizeof(option_type));
        *packet_len+= sizeof(option_type);

        memcpy(&packet[*packet_len], &option_size, sizeof(option_size));
        *packet_len+= sizeof(option_size);
    }

    memcpy(&packet[*packet_len], option_data, option_size);
    *packet_len+= option_size;

    DEBUG_LOG("REQ-ADDOPT", "DHCP option added...");
    DEBUG_PRINT("\tpacket_length (local): %hu\n", (unsigned short) *packet_len);
}

void dhcp_get_option( const uint8_t *packet, uint16_t packet_len, uint8_t option_type, uint8_t **option_data, uint8_t *option_size )
{
    DEBUG_LOG("REQ-GETOPT", "Getting DHCP option:");
    DEBUG_PRINT("\ttype:\t%d\n", option_type);

    uint8_t *options = (uint8_t *) (packet + get_header_sizes() + get_dhcp_data_size());
    DEBUG_PRINT("\topts:\t%p\n\tpacket:\t%p\n\tpacket_len:\t%d\n", (void *) options, (void *) packet, packet_len);

    while (options < packet + packet_len)
    {
        uint8_t opt_type = (uint8_t) *options;
        uint8_t opt_len = (uint8_t) *(options + 1);

        DEBUG_LOG("REQ-GETOPT", "Got:");
        DEBUG_PRINT("\ttype:\t%d\n\tlength:\t%d\n", opt_type, opt_len);

        if (opt_type == option_type)
        {
            DEBUG_LOG("REQ-GETOPT", "Option found!");

            *option_data = malloc(sizeof(uint8_t) * opt_len);
            memcpy(*option_data, options + 2, opt_len);

            DEBUG_PRINT("\tdata:\t%#x (%d)\n", (uint32_t) *option_data, (uint32_t) **option_data);

            if (option_size)
                *option_size = opt_len;

            return;
        }
        else if (opt_type == DHO_END)
        {
            break;
        }
        else
        {
            DEBUG_LOG("REQ-GETOPT", "Not found yet, continuting from:");
            DEBUG_PRINT("\topts:\t%p\n", (void *) options);

            options+= opt_len + 2;
        }
    }
    DEBUG_LOG("REQ-GETOPT", "Search completed.");

    *option_data = NULL;
    if (option_size != NULL)
        *option_size = 0;

    DEBUG_LOG("REQ-GETOPT", "Option not found!");
}

void dhcp_copy_option( uint8_t *packet, uint16_t *packet_len, uint8_t *prev_packet, uint16_t prev_packet_len, uint8_t option_type )
{
    DEBUG_LOG("REQ-COPYOPT", "Copying DHCP option:");
    DEBUG_PRINT("\ttype:\t%d\n", option_type);
    DEBUG_PRINT("\tfrom:\t%p (size: %d)\n\tto:\t%p (size: %d)\n", (void *) packet, *packet_len, (void *) prev_packet, prev_packet_len);

    uint8_t *opt_data;
    uint8_t opt_size;

    dhcp_get_option(prev_packet, prev_packet_len, option_type, &opt_data, &opt_size);
    dhcp_add_option(packet, packet_len, option_type, opt_data, opt_size);

    dhcp_destroy_option(opt_data);

    DEBUG_LOG("REQ-COPYOPT", "DHCP option copied!");
}

void dhcp_destroy_option( void *option_data )
{
    if (option_data != NULL)
        free(option_data);
}

void dhcp_request_create( uint8_t *packet, uint16_t *packet_len, uint8_t *mac, uint8_t dhcp_op, uint8_t *prev_packet, uint16_t prev_packet_len )
{
    DEBUG_LOG("REQ-CREATE", "Creating DHCP data packet...");
    eth_encaps(packet, packet_len, mac);
    ip_encaps(packet, packet_len);
    udp_encaps(packet, packet_len);
    dhcp_encaps(packet, packet_len, mac, BOOTREQUEST, prev_packet != NULL ? get_dhcp_data(prev_packet)->xid : 0);

    DEBUG_LOG("REQ-CREATE", "Adding message type option...");
    uint8_t opt = dhcp_op;
    dhcp_add_option(packet, packet_len, DHO_DHCP_MESSAGE_TYPE, &opt, sizeof(opt));

    char *opt_char = "DHCP-HUNGER";
    dhcp_add_option(packet, packet_len, DHO_HOST_NAME, (uint8_t *) opt_char, (uint8_t) strlen(opt_char));

    if (dhcp_op == DHCPDISCOVER)
    {
        DEBUG_LOG("REQ-CREATE", "Packet type is DHCPDISCOVER, adding options...");

        uint8_t request_list[4] = {
                DHO_SUBNET_MASK, // Request Subnet Mask
                DHO_ROUTERS, // Router
                DHO_DOMAIN_NAME, // Domain Name
                DHO_DOMAIN_NAME_SERVERS //  Domain Name Server
        };
        dhcp_add_option(packet, packet_len, DHO_DHCP_PARAMETER_REQUEST_LIST, request_list, sizeof(request_list));
    }
    else if (dhcp_op == DHCPREQUEST)
    {
        if (prev_packet == NULL || prev_packet_len == 0)
        {
            ERR("Cannot create DHCPREQUEST packet without previous one.\n");
            exit(1);
        }

        DEBUG_LOG("REQ-CREATE", "Packet type is DHCPREQUEST, adding options...");

        get_dhcp_data(packet)->secs = get_dhcp_data(prev_packet)->secs;

        dhcp_copy_option(packet, packet_len, prev_packet, prev_packet_len, DHO_DHCP_SERVER_IDENTIFIER);

        dhcp_add_option(packet, packet_len, DHO_DHCP_REQUESTED_ADDRESS, (uint8_t *) &get_dhcp_data(prev_packet)->yiaddr, sizeof(get_dhcp_data(prev_packet)->yiaddr));
    }
    else
    {
        DEBUG_LOG("REQ-CREATE", "Packet type is unknown:");
        DEBUG_PRINT("\ttype: %d\n", dhcp_op);
    }

    DEBUG_LOG("REQ-CREATE", "DHCP data packet created...");
}

void dhcp_request_complete( uint8_t *packet, uint16_t *packet_len )
{
    DEBUG_LOG("REQ-COMPLETE", "Finalizing DHCP data packet...");
    uint8_t opt = DHO_END;
    dhcp_add_option(packet, packet_len, 0, &opt, sizeof(opt));

    DEBUG_PRINT("\tpacket_length (local): %hu\n\tpacket_length (UDP): %hu\n\tpacket_length (IP): %hu\n",
                (unsigned short) *packet_len,
                (unsigned short) *packet_len - sizeof(struct ethhdr) - sizeof(struct iphdr),
                (unsigned short) *packet_len - sizeof(struct ethhdr));
    //  Length of UDP payload and header
    get_udp_header(packet)->len = htons(*packet_len - sizeof(struct ethhdr) - sizeof(struct iphdr));

    //  Length of IP payload and header
    get_ip_header(packet)->tot_len = htons(*packet_len - sizeof(struct ethhdr));

    //  Calculate IP checksum on completed header
    get_ip_header(packet)->check = check_sum((unsigned short *) (packet + sizeof(struct ethhdr)),
                                             sizeof(struct iphdr) / 2);

    DEBUG_LOG("REQ-COMPLETE", "DHCP data packet finalized...");
}

int dhcp_request_receive( uint8_t *reply_packet, uint16_t reply_packet_len, uint8_t *req_packet/*, uint16_t req_packet_len*/, uint8_t dhcp_op, uint8_t dhcp_message_type )
{
    DEBUG_LOG("REQ-RECV", "Validating transaction_id: ");
    if (get_dhcp_data(reply_packet)->xid != get_dhcp_data(req_packet)->xid)
    {
        DEBUG_PRINT("invalid\n");
        DEBUG_PRINT("\t%#x != %#x\n", get_dhcp_data(reply_packet)->xid, get_dhcp_data(req_packet)->xid);
        return 0;
    }
    DEBUG_PRINT("VALID!\n");

    DEBUG_LOG("REQ-RECV", "Validating DHCP chaddr: ");
    if (get_dhcp_data(reply_packet)->chaddr[0] != get_dhcp_data(req_packet)->chaddr[0]
        || get_dhcp_data(reply_packet)->chaddr[1] != get_dhcp_data(req_packet)->chaddr[1]
        || get_dhcp_data(reply_packet)->chaddr[2] != get_dhcp_data(req_packet)->chaddr[2]
        || get_dhcp_data(reply_packet)->chaddr[3] != get_dhcp_data(req_packet)->chaddr[3]
        || get_dhcp_data(reply_packet)->chaddr[4] != get_dhcp_data(req_packet)->chaddr[4]
        || get_dhcp_data(reply_packet)->chaddr[5] != get_dhcp_data(req_packet)->chaddr[5])
    {
        DEBUG_PRINT("invalid ");
        DEBUG_PRINT("(%x:%x:%x:%x:%x:%x", get_dhcp_data(reply_packet)->chaddr[0], get_dhcp_data(reply_packet)->chaddr[1], get_dhcp_data(reply_packet)->chaddr[2], get_dhcp_data(reply_packet)->chaddr[3], get_dhcp_data(reply_packet)->chaddr[4], get_dhcp_data(reply_packet)->chaddr[5]);
        DEBUG_PRINT(", expected %x:%x:%x:%x:%x:%x)\n", get_dhcp_data(req_packet)->chaddr[0], get_dhcp_data(req_packet)->chaddr[1], get_dhcp_data(req_packet)->chaddr[2], get_dhcp_data(req_packet)->chaddr[3], get_dhcp_data(req_packet)->chaddr[4], get_dhcp_data(req_packet)->chaddr[5]);
        return 0;
    }
    DEBUG_PRINT("VALID!\n");

    DEBUG_LOG("REQ-RECV", "Validating DHCP message_type: ");
    uint8_t *message_type;
    dhcp_get_option(reply_packet, (uint16_t) reply_packet_len, DHO_DHCP_MESSAGE_TYPE, &message_type, NULL);
    if (get_dhcp_data(reply_packet)->op != dhcp_op || message_type == NULL || *message_type != dhcp_message_type)
    {
        DEBUG_PRINT("invalid\n");
        DEBUG_PRINT("\tmessage_type: %d (expected %d)\n", message_type == NULL ? -1 : *message_type, dhcp_message_type);
        DEBUG_PRINT("\top: %d (expected %d)\n", get_dhcp_data(reply_packet)->op, dhcp_op);
        dhcp_destroy_option(message_type);
        return 0;
    }
    dhcp_destroy_option(message_type);
    DEBUG_PRINT("VALID!\n");

    return 1;
}