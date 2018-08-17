

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
#include "adc.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

static spi_device_handle_t spi;
#include "spi.h"

//#include "queue.h"
#include "static_queue.h"
#define BLINK_GPIO 5 //esp32 thing gpio_led

#define TRANSMISSION_PERIOD_MS 500
Static_Queue * ADCqueue;
size_t bufferIndex = 0;
static uint64_t dacount = 0;
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

    while(1) {
        /* Blink off (output low) */
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        /* Blink on (output hivalbuffergh) */
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(2 / portTICK_PERIOD_MS);

		//printf("blink_task is running\n");
		//printf("adc reading: %d\n", adc_read());
        //sprintf(valbuffer, "%d\n;", adc_read());
        dacount += 2;
        enqueue(ADCqueue, dacount);
        if(ADCqueue->size == MAX_SIZE-1){
            while(!queueIsEmpty(ADCqueue)) {
                printf("%u, ", dequeue(ADCqueue));
            }
            printf("\nIndex: %d\n", ADCqueue->size);
        }
		data[1]++;
		if(data[1] == 0x8F)
			data[1] = 0;
		rheo_send_data(data);
    }
}

/* @section Periodic Timer Setup
 * 
 * @text The heartbeat handler increases the real counter every second, 
 * and sends a text string with the counter value, as shown in Listing PeriodicCounter. 
 */

static btstack_timer_source_t transmission_timer;

static void bt_transmission_handler(struct btstack_timer_source *ts){
    static int counter = 0;

    if (rfcomm_channel_id){
        sprintf(lineBuffer, "Counter %04u, values:", ++counter);
        while (!queueIsEmpty(ADCqueue)) {
            sprintf(lineBuffer, "%d;", dequeue(ADCqueue));
        }
        sprintf(lineBuffer, "\n");
        printf("%s", lineBuffer);

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
    int rfcomm_channel = rand() % 30 + 1; // select a channel from 1 to 30
    ADCqueue = queueCreate();
    queueClear(ADCqueue);
    printf("Channel set to %d\n", rfcomm_channel);
    one_shot_timer_setup();
    service_setup(rfcomm_channel);

    gap_discoverable_control(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_set_local_name("BT amp 00:00:00:00:00:00");

    adc_init();

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

	xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE*4, NULL, 5, NULL); //minimal stack size(128) is too small for blink task.

    // turn on!
    hci_power_control(HCI_POWER_ON);
    
    return 0;
}
/* EXAMPLE_END */

