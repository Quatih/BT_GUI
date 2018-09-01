/*
 * Based on BtStacks ESP32 spp_counter example 
 */

#ifndef _BT_H
#define _BT_H

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "btstack.h"

// for transmission

uint8_t * lineBuffer;
uint32_t lineBufferIndex;

#define SERVICE_UUID {0x0f, 0xd5, 0xca, 0x36, 0x4e, 0x7d, 0x4f, 0x99, 0x82, 0xec, 0x28, 0x68, 0x26, 0x2b, 0xd4, 0xe4}

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

uint16_t rfcomm_channel_id;

// Output from create_custom_sdp_record, intended to reduce RAM footprint

#define SPP_SERVICE_BUFFER_LEN 91

uint8_t * spp_service_buffer;

static btstack_packet_callback_registration_t hci_event_callback_registration;



/* @section SPP Service Setup 
 *s
 * @text To provide an SPP service, the L2CAP, RFCOMM, and SDP protocol layers 
 * are required. After setting up an RFCOMM service with channel nubmer
 * RFCOMM_SERVER_CHANNEL, an SDP record is created and registered with the SDP server.
 * Example code for SPP service setup is
 * provided in Listing SPPSetup. The SDP record created by function
 * spp_create_sdp_record consists of a basic SPP definition that uses the provided
 * RFCOMM channel ID and service name. For more details, please have a look at it
 * in \path{src/sdp_util.c}. 
 * The SDP record is created on the fly in RAM and is deterministic.
 * To preserve valuable RAM, the result could be stored as constant data inside the ROM.   
 */

void create_custom_sdp_record(uint8_t *service, uint32_t service_record_handle, int rfcomm_channel, const char *name){
	
	uint8_t* attribute;
	de_create_sequence(service);
    
    // 0x0000 "Service Record Handle"
	de_add_number(service, DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_SERVICE_RECORD_HANDLE);
	de_add_number(service, DE_UINT, DE_SIZE_32, service_record_handle);
    
	// 0x0001 "Service Class ID List"
	de_add_number(service,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_SERVICE_CLASS_ID_LIST);
    uint8_t buff[] = SERVICE_UUID;
    attribute = de_push_sequence(service);
    {
        de_add_uuid128(attribute, buff);
        //de_add_number(attribute,  DE_UUID, DE_SIZE_16, BLUETOOTH_SERVICE_CLASS_SERIAL_PORT );
    }
    de_pop_sequence(service, attribute);
    // de_add_uuid128(service, SERVICE_UUID );
	
	// 0x0004 "Protocol Descriptor List"
	de_add_number(service,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_PROTOCOL_DESCRIPTOR_LIST);
	attribute = de_push_sequence(service);
	{
		uint8_t* l2cpProtocol = de_push_sequence(attribute);
		{
			de_add_number(l2cpProtocol,  DE_UUID, DE_SIZE_16, BLUETOOTH_PROTOCOL_L2CAP);
		}
		de_pop_sequence(attribute, l2cpProtocol);
		
		uint8_t* rfcomm = de_push_sequence(attribute);
		{
			de_add_number(rfcomm,  DE_UUID, DE_SIZE_16, BLUETOOTH_PROTOCOL_RFCOMM);  // rfcomm_service
			de_add_number(rfcomm,  DE_UINT, DE_SIZE_8,  rfcomm_channel);  // rfcomm channel
		}
		de_pop_sequence(attribute, rfcomm);
	}
	de_pop_sequence(service, attribute);
	
	// 0x0005 "Public Browse Group"
	de_add_number(service,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_BROWSE_GROUP_LIST); // public browse group
	attribute = de_push_sequence(service);
	{
		de_add_number(attribute,  DE_UUID, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_PUBLIC_BROWSE_ROOT );
	}
	de_pop_sequence(service, attribute);
	
	// 0x0006
	de_add_number(service,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	attribute = de_push_sequence(service);
	{
		de_add_number(attribute, DE_UINT, DE_SIZE_16, 0x656e);
		de_add_number(attribute, DE_UINT, DE_SIZE_16, 0x006a);
		de_add_number(attribute, DE_UINT, DE_SIZE_16, 0x0100);
	}
	de_pop_sequence(service, attribute);
	
	// // 0x0009 "Bluetooth Profile Descriptor List"
    // de_add_number(service,  DE_UINT, DE_SIZE_16, BLUETOOTH_ATTRIBUTE_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
    // attribute = de_push_sequence(service);
    // {
	// 	uint8_t *Profile = de_push_sequence(attribute);
	// 	{
    //         de_add_number(Profile,  DE_UUID, DE_SIZE_16, 0xca36);
	// 		// de_add_number(Profile,  DE_UUID, DE_SIZE_16, BLUETOOTH_SERVICE_CLASS_SERIAL_PORT);
	// 		// de_add_number(Profile,  DE_UINT, DE_SIZE_16, BLUETOOTH_SERVICE_CLASS_LAN_ACCESS_USING_PPP);
	// 	}
	// 	de_pop_sequence(attribute, Profile);
    // }
    // de_pop_sequence(service, attribute);

	// 0x0100 "ServiceName"
	de_add_number(service, DE_UINT, DE_SIZE_16, 0x0100);
	de_add_data(service, DE_STRING, strlen(name), (uint8_t *) name);
    // for (int i = 0; i < 91; i++){
    //     printf("0x%0x,",spp_service_buffer[i]);
    // }
}

static void bt_service_setup(int rfcomm_channel){
    // register for HCI events
    spp_service_buffer = (uint8_t*) malloc(sizeof(uint8_t)*SPP_SERVICE_BUFFER_LEN);

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    l2cap_init();
  
    rfcomm_init();
    rfcomm_register_service(packet_handler, rfcomm_channel, 0xffff);  // reserved channel, mtu limited by l2cap

    // init SDP, create record for SPP and register with SDP
    sdp_init();
    create_custom_sdp_record(spp_service_buffer, 0x10001, rfcomm_channel, "BT_Sense");

   // printf("Before dump: %u\n\r", de_get_len((uint8_t*)spp_service_buffer));
    //de_dump_data_element((uint8_t*)spp_service_buffer);
    //printf("After dump: %u\n\r", de_get_len((uint8_t*)spp_service_buffer));

    sdp_register_service(spp_service_buffer);
    //sdp_client_query_rfcomm_register_callback(handle_query_rfcomm_event , NULL);

    printf("SDP service record size: %u\n", de_get_len(spp_service_buffer));
    // free(spp_service_buffer);
}

/* @section Bluetooth Logic 
 * @text The Bluetooth logic is implemented within the 
 * packet handler, see Listing SppServerPacketHandler. In this example, 
 * the following events are passed sequentially: 
 * - BTSTACK_EVENT_STATE,
 * - HCI_EVENT_PIN_CODE_REQUEST (Standard pairing) or 
 * - HCI_EVENT_USER_CONFIRMATION_REQUEST (Secure Simple Pairing),
 * - RFCOMM_EVENT_INCOMING_CONNECTION,
 * - RFCOMM_EVENT_CHANNEL_OPENED, 
* - RFCOMM_EVETN_CAN_SEND_NOW, and
 * - RFCOMM_EVENT_CHANNEL_CLOSED
 */

/* @text Upon receiving HCI_EVENT_PIN_CODE_REQUEST event, we need to handle
 * authentication. Here, we use a fixed PIN code "0000".
 *
 * When HCI_EVENT_USER_CONFIRMATION_REQUEST is received, the user will be 
 * asked to accept the pairing request. If the IO capability is set to 
 * SSP_IO_CAPABILITY_DISPLAY_YES_NO, the request will be automatically accepted.
 *
 * The RFCOMM_EVENT_INCOMING_CONNECTION event indicates an incoming connection.
 * Here, the connection is accepted. More logic is need, if you want to handle connections
 * from multiple clients. The incoming RFCOMM connection event contains the RFCOMM
 * channel number used during the SPP setup phase and the newly assigned RFCOMM
 * channel ID that is used by all BTstack commands and events.
 *
 * If RFCOMM_EVENT_CHANNEL_OPENED event returns status greater then 0,
 * then the channel establishment has failed (rare case, e.g., client crashes).
 * On successful connection, the RFCOMM channel ID and MTU for this
 * channel are made available to the heartbeat counter. After opening the RFCOMM channel, 
 * the communication between client and the application
 * takes place. In this example, the timer handler increases the real counter every
 * second. 
 *
 * RFCOMM_EVENT_CAN_SEND_NOW indicates that it's possible to send an RFCOMM packet
 * on the rfcomm_cid that is include

 */ 

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);

    bd_addr_t event_addr;
    uint8_t   rfcomm_channel_nr;
    uint16_t  mtu;
    int i;

    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
                case HCI_EVENT_PIN_CODE_REQUEST:
                    // inform about pin code request
                    printf("Pin code request - using '0000'\n");
                    hci_event_pin_code_request_get_bd_addr(packet, event_addr);
                    gap_pin_code_response(event_addr, "0000");
                    break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    // ssp: inform about user confirmation request
                    printf("SSP User Confirmation Request with numeric value '%06"PRIu32"'\n", little_endian_read_32(packet, 8));
                    printf("SSP User Confirmation Auto accept\n");
                    break;

                case RFCOMM_EVENT_INCOMING_CONNECTION:
                    // data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
                    rfcomm_event_incoming_connection_get_bd_addr(packet, event_addr); 
                    rfcomm_channel_nr = rfcomm_event_incoming_connection_get_server_channel(packet);
                    rfcomm_channel_id = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);
                    printf("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
                    rfcomm_accept_connection(rfcomm_channel_id);
                    break;
               
                case RFCOMM_EVENT_CHANNEL_OPENED:
                    // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
                    if (rfcomm_event_channel_opened_get_status(packet)) {
                        printf("RFCOMM channel open failed, status %u\n", rfcomm_event_channel_opened_get_status(packet));
                    } else {
                        rfcomm_channel_id = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
                        mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
                        printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_channel_id, mtu);
                    }
                    break;
                case RFCOMM_EVENT_CAN_SEND_NOW:
                    rfcomm_send(rfcomm_channel_id, (uint8_t*) lineBuffer, lineBufferIndex);  
                    break;
              
                case RFCOMM_EVENT_CHANNEL_CLOSED:
                    printf("RFCOMM channel closed\n");
                    rfcomm_channel_id = 0;
                    break;
                
                default:
                    break;
            }
            break;

        case RFCOMM_DATA_PACKET:
            printf("RCV: '");
            for (i=0;i<size;i++){
                putchar(packet[i]);
            }
            printf("'\n");
            break;

        default:
            break;
    }
}
#endif