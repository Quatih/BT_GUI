
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "btstack.h"

#define HEARTBEAT_PERIOD_MS 1000

static uint16_t rfcomm_channel_id;

static int RFCOMM_SERVER_CHANNEL;

static btstack_timer_source_t heartbeat;
static char lineBuffer[30];

static void  heartbeat_handler(struct btstack_timer_source *ts){
    static int counter = 0;

    if (rfcomm_channel_id){
        sprintf(lineBuffer, "BTstack counter %04u\n", ++counter);
        printf("%s", lineBuffer);

        rfcomm_request_can_send_now_event(rfcomm_channel_id);
    }

    btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
} 

static void one_shot_timer_setup(void){
    // set one-shot timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(&heartbeat);
}

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
 
static btstack_packet_callback_registration_t hci_event_callback_registration;
/*
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
 
   uint16_t rfcomm_channel_id;
 
   if (packet_type == HCI_EVENT_PACKET
       && hci_event_packet_get_type(packet) == RFCOMM_EVENT_INCOMING_CONNECTION){
 
       rfcomm_channel_id = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);
       rfcomm_accept_connection(rfcomm_channel_id);
 
     }else if (packet_type == RFCOMM_DATA_PACKET){
 
        printf("Received data: '");
 
        for (int i=0;i<size;i++){
             putchar(packet[i]);
         }
 
          printf("\n----------------\n");
    }
}
*/

/* LISTING_START(SppServerPacketHandler): SPP Server - Heartbeat Counter over RFCOMM */
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);

/* LISTING_PAUSE */ 
    bd_addr_t event_addr;
    uint8_t   rfcomm_channel_nr;
    uint16_t  mtu;
    int i;

    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
/* LISTING_RESUME */ 
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
                    rfcomm_send(rfcomm_channel_id, (uint8_t*) lineBuffer, strlen(lineBuffer));  
                    break;

/* LISTING_PAUSE */                 
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
/* LISTING_RESUME */ 
}
/* LISTING_END */
 
int btstack_main(int argc, const char * argv[]){
 
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
 
    l2cap_init();
 
    rfcomm_init();
    RFCOMM_SERVER_CHANNEL = rand() % 30 + 1; // select a channel from 1 to 30
    one_shot_timer_setup();
    
    rfcomm_register_service(packet_handler, RFCOMM_SERVER_CHANNEL, 0xffff); 
 
    sdp_init();
 
    gap_discoverable_control(1);
    uint32_t handle = sdp_create_service_record_handle();

    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_set_local_name("RFCOMM Test 00:00:00:00:00:00");

    hci_power_control(HCI_POWER_ON);
 
    return 0;
}