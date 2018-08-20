

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "soc/gpio_struct.h"
#include "driver/spi_master.h"

#include "bt.h"
#include <btstack.h>
#include "i2s_adc.h"

#include <time.h>
#include <sys/time.h>

static spi_device_handle_t spi;
#include "spi.h"
//#include "queue.h"
#include "static_queue.h"
#define BLINK_GPIO 5 //esp32 thing gpio_led
#define LINEBUFFER_BYTES 8*125
#define TRANSMISSION_PERIOD_MS 1
Static_Queue * ADCqueue;
static uint64_t dacount = 0;

uint32_t get_msecs() {

    struct timeval tv;

    gettimeofday(&tv, NULL);
    uint32_t ret = (uint32_t) (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
    return ret;
}

int insert_int_in_buffer(uint8_t * buffer, uint32_t insert){
    // buffer[0] = insert & 0xff;
    // buffer[1] = (insert >> 8)  & 0xff;
    // buffer[2] = (insert >> 16) & 0xff;
    // buffer[3] = (insert >> 24) & 0xff;

    buffer[3] = insert & 0xff;
    buffer[2] = (insert >> 8)  & 0xff;
    buffer[1] = (insert >> 16) & 0xff;
    buffer[0] = (insert >> 24) & 0xff;
    
    return 4;
    // printf("[%x, %x, %x, %x], %u ", buffer[0], buffer[1], buffer[2], buffer[3], insert);
}

int put_measurements(uint8_t* buffer, int index){
            //for (int i = 0; i< ADCqueue->size; i++){
        ///    printf("%d, ", ADCqueue->queue[i]);
        //}
        uint8_t * start = buffer;
        val_tuple deq;
        int i = 0;
        int indexStart = index;
        
        // while (!queueIsEmpty(ADCqueue) || i < 4) {
        //     i++;
        //     deq = dequeue(ADCqueue);
        //     length += sprintf(lineBuffer + length, "%d;", deq.data);
        // }
        // unsigned char data[sizeof(QueueType)+1];
        // unsigned char time[sizeof(QueueTime)+1];
        
        
        while (!queueIsEmpty(ADCqueue) || index < (LINEBUFFER_BYTES - 8)) {
            i++;
            deq = dequeue(ADCqueue);
            index += insert_int_in_buffer(lineBuffer + index, deq.data);
            index += insert_int_in_buffer(lineBuffer + index, deq.time);
            // length += sprintf(lineBuffer+ length, "%s", &data[0]);
            // length += sprintf(lineBuffer+ length, "%s", &time[0]);
            // printf("%s%s\n", data, time);
        }
        // printf("len %d, i: %d \n", lineBufferIndex, i*8 );
        buffer[0] = '\0';
        i = 0;
        // while(start[i]!= '\0'){
        // while(i < index - indexStart){
        //     printf("%02x", start[i++]);
        // }
        return index;
        // sprintf(ref, "%s", temp);
}


void blink_task(void *pvParameter)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    uint8_t data[3] = {0x00, 0x00, '\0'};
    printf("port, %d\n", portTICK_PERIOD_MS);
    while(1) {
        /* Blink off (output low) */
        // gpio_set_level(BLINK_GPIO, 0);
        // vTaskDelay(50 / portTICK_PERIOD_MS);
        // /* Blink on (output hivalbuffergh) */
        // gpio_set_level(BLINK_GPIO, 1);
         vTaskDelay(1 / portTICK_PERIOD_MS);

		//printf("blink_task is running\n");
		//printf("adc reading: %d\n", adc_read());
        //sprintf(valbuffer, "%d\n;", adc_read());
        dacount += 1;
        enqueue(ADCqueue, (val_tuple) {dacount, get_msecs()});
        if (dacount == 15){
            // lineBufferIndex = sprintf(lineBuffer, "Blaa:");
            // put_measurements(lineBuffer + lineBufferIndex);
            dacount = 0;
            // printf("%s\n", lineBuffer);
        }
        // if(ADCqueue->size == MAX_SIZE-1){
        //     while(!queueIsEmpty(ADCqueue)) {
        //         printf("%u, ", dequeue(ADCqueue));
        //     }
        //     printf("\nIndex: %d\n", ADCqueue->size);
        // }
		// data[1]++;
		// if(data[1] == 0x8F)
		// 	data[1] = 0;
		// rheo_send_data(data);
    }
}

/* @section Periodic Timer Setup
 * 
 * @text The heartbeat handler increases the real counter every second, 
 * and sends a text string with the counter value, as shown in Listing PeriodicCounter. 
 */

static btstack_timer_source_t transmission_timer;

static void bt_transmission_handler(struct btstack_timer_source *ts){
    // static int counter = 0;

    if (rfcomm_channel_id){
        // lineBufferIndex = sprintf(lineBuffer, "%04u:", ++counter);
        // printf("line: %02x, buffer: %02x, len: %d \n",&lineBuffer[0], &lineBuffer[lineBufferIndex], lineBufferIndex );
        // lineBufferIndex = put_measurements(lineBuffer, 0);

        rfcomm_request_can_send_now_event(rfcomm_channel_id);
    }

    btstack_run_loop_set_timer(ts, TRANSMISSION_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
} 

static void one_shot_timer_setup(void){
    // set one-shot timer
    transmission_timer.process = &bt_transmission_handler;
    btstack_run_loop_set_timer(&transmission_timer, TRANSMISSION_PERIOD_MS);
    btstack_run_loop_add_timer(&transmission_timer);
}




int btstack_main(int argc, const char * argv[]);
int btstack_main(int argc, const char * argv[]){
    (void)argc;
    (void)argv;
    int rfcomm_channel = esp_random() % 30 + 1; // select a channel from 1 to 30
    ADCqueue = queueCreate();
    printf("Channel set to %d\n", rfcomm_channel);
    one_shot_timer_setup();
    service_setup(rfcomm_channel);

    gap_discoverable_control(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_set_local_name("BT amp 00:00:00:00:00:00");

    // adc_init();

    i2s_init();
	//spi_init(&spi);
	spi_init();
	uint8_t data[3] = {0x00, 0x00, '\0'};
	//rheo_send_data(&spi, data);
	rheo_send_data(data);
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    xTaskCreatePinnedToCore(&i2s_adc_sample, "i2s_adc_sample", 1024 * 2, NULL, 1, NULL, 1);


	xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE*4, NULL, 5, NULL); //minimal stack size(128) is too small for blink task.
    lineBufferIndex = LINEBUFFER_BYTES;
    // turn on!
    hci_power_control(HCI_POWER_ON);
    lineBuffer = (uint8_t*) malloc(sizeof (uint8_t) * LINEBUFFER_BYTES); // allocate space for samples
    memset(lineBuffer, 0, LINEBUFFER_BYTES);
    return 0;
}
/* EXAMPLE_END */

