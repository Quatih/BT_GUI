
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "btstack.h"


static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
 
static btstack_packet_callback_registration_t hci_event_callback_registration;
 
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
 
int btstack_main(int argc, const char * argv[]){
 
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
 
    l2cap_init();
 
    rfcomm_init();
 
    rfcomm_register_service(packet_handler, 3, 0xffff); 
 
    sdp_init();
 
    gap_discoverable_control(1);
 
    hci_power_control(HCI_POWER_ON);
 
    return 0;
}