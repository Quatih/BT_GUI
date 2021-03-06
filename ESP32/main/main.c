

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

#include "bt_btstack.h"
#include "i2s_adc.h"
#include "adc.h"
#include <time.h>
#include <sys/time.h>

#define LINEBUFFER_BYTES 2*I2S_BUF_LEN+8 // 8 extra for timestamp

uint64_t get_msecs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

uint64_t get_usecs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000LL + (tv.tv_usec));
}

// puts the usec timestamp of the last sample
void put_time(uint8_t * buffer, uint64_t t){
    buffer[0] = (t >> 56) & 0xff;
    buffer[1] = (t >> 48) & 0xff;
    buffer[2] = (t >> 40) & 0xff;  
    buffer[3] = (t >> 32) & 0xff;
    buffer[4] = (t >> 24) & 0xff;
    buffer[5] = (t >> 16) & 0xff;
    buffer[6] = (t >> 8) & 0xff;
    buffer[7] = t & 0xff;

    // for (int i = 0; i<8; i++){
    //     printf("%02x", buffer[i]);
    // }
    // printf("\n");
}

void i2s_adc_sample()
{
    // 2* length because there are two DMA buffers
    int i2s_read_len = 2*I2S_BUF_LEN;

    size_t bytes_read;
    
    uint8_t* i2s_read_buff = (uint8_t*) calloc(i2s_read_len, sizeof(uint8_t));
    i2s_adc_enable(I2S_NUM);
    uint32_t usecs1, usecs2;
    while(1){
        //read data from I2S bus
        usecs1 = get_usecs();
        
        i2s_read(I2S_NUM, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        usecs2 = get_usecs();
        // usec variables fairly useless, since the i2s_read function is blocking.
        ets_printf("At %u, samples read: %u After: %u usecs\n", usecs2, bytes_read/2, usecs2 - usecs1);

        // disp_buf((uint8_t*) i2s_read_buff, bytes_read);

        memcpy(lineBuffer+8, i2s_read_buff, bytes_read);
        put_time(lineBuffer, usecs2);

        // rfcomm_channel_id is only valid when a rfcomm connection is open.
        if(rfcomm_channel_id)
            rfcomm_request_can_send_now_event(rfcomm_channel_id);

        // vTaskDelay(10);
    }
    i2s_adc_disable(I2S_NUM);
    free(i2s_read_buff);
    i2s_read_buff = NULL;

    vTaskDelete(NULL);
}

int btstack_main(int argc, const char * argv[]);
int btstack_main(int argc, const char * argv[]){
    (void)argc;
    (void)argv;
    int rfcomm_channel = esp_random() % 30 + 1; // select a channel from 1 to 30
    printf("Channel set to %d\n", rfcomm_channel);
    // one_shot_timer_setup();
    bt_service_setup(rfcomm_channel);

    gap_discoverable_control(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_set_local_name("BT amp 00:00:00:00:00:00");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    adc_init();
    i2s_init();

    lineBuffer = (uint8_t*) calloc(LINEBUFFER_BYTES, sizeof (uint8_t)); // allocate space for samples

    xTaskCreatePinnedToCore(&i2s_adc_sample, "i2s_adc_sample", 1024 * 4, NULL, 1, NULL, 1);

	// xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE*4, NULL, 5, NULL); //minimal stack size(128) is too small for blink task.
    lineBufferIndex = LINEBUFFER_BYTES;
    // turn on!
    hci_power_control(HCI_POWER_ON);
    return 0;
}

