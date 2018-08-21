#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spi_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define V_REF 1100
#define ADC1_TEST_CHANNEL (ADC1_CHANNEL_7)

#define PARTITION_NAME "storage"

// #define WRITE_FLASH

//i2s number
#define I2S_NUM (0)

//i2s Sample rate
#define I2S_SAMPLE_RATE (20150)

//i2s data bits
#define I2S_SAMPLE_BITS (16)
//enable display buffer for debug
#define I2S_BUF_DEBUG (0)

//I2S read buffer length [bytes]
#define I2S_BUF_LEN (1000)
//I2S data format
#define I2S_FORMAT (I2S_CHANNEL_FMT_ONLY_RIGHT)
//I2S channel number
#define I2S_CHANNEL_NUM ((I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))
//I2S built-in ADC unit
#define I2S_ADC_UNIT ADC_UNIT_1
//I2S built-in ADC channel
#define I2S_ADC_CHANNEL ADC1_CHANNEL_0 // GPIO_36, SVP pin

//flash record size, for recording 5 seconds' data
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * 5)
#define FLASH_ERASE_SIZE (FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE == 0) ? FLASH_RECORD_SIZE : FLASH_RECORD_SIZE + (FLASH_SECTOR_SIZE - FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE)
//sector size of flash
#define FLASH_SECTOR_SIZE (0x1000)
//flash read / write address
#define FLASH_ADDR (0x200000)


/**
 * @brief I2S ADC/DAC mode init.
 */
void i2s_init()
{
    int i2s_num = I2S_NUM;
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN,
        .sample_rate =  I2S_SAMPLE_RATE/2,
        .bits_per_sample = I2S_SAMPLE_BITS,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .channel_format = I2S_FORMAT,
        .intr_alloc_flags = 0,
        .dma_buf_count = 2,
        .dma_buf_len = I2S_BUF_LEN,
        .use_apll = false
    };
    //install and start i2s driver
    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
	//init DAC pad
	//i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    //init ADC pad
    i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL);
}

/*
 * @brief erase flash for recording
 */
void erase_flash()
{
    printf("Erasing flash \n");
    const esp_partition_t *data_partition = NULL;
    data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);
    if (data_partition != NULL) {
        printf("partiton addr: 0x%08x; size: %d; label: %s\n", data_partition->address, data_partition->size, data_partition->label);
    }
    printf("Erase size: %d Bytes\n", FLASH_ERASE_SIZE);
    ESP_ERROR_CHECK(esp_partition_erase_range(data_partition, 0, FLASH_ERASE_SIZE));
}

/**
 * @brief debug buffer data
 */
void disp_buf(uint8_t* buf, int length)
{
    printf("======\n");
    for (int i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 32 == 0) {
            printf("\n");
        }
    }
    printf("======\n");
}

/**
 * @brief I2S ADC/DAC example
 *        1. Erase flash
 *        2. Record audio from ADC and save in flash
 *        3. Read flash and replay the sound via DAC
 *        4. Play an example audio file(file format: 8bit/8khz/single channel)
 *        5. Loop back to step 3
 */

uint32_t get_usecs() {

    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint32_t ret = (uint32_t) (tv.tv_sec * 1000000LL + (tv.tv_usec));
    return ret;
}

void i2s_adc_sample()
{
    #ifdef WRITE_FLASH
    const esp_partition_t *data_partition = NULL;
    data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);
    if (data_partition != NULL) {
        printf("partiton addr: 0x%08x; size: %d; label: %s\n", data_partition->address, data_partition->size, data_partition->label);
    } else {
        ESP_LOGE(TAG, "Partition error: can't find partition name: %s\n", PARTITION_NAME);
        vTaskDelete(NULL);
    }
    //1. Erase flash
    erase_flash();

    

    uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
    #endif
    int i2s_read_len = I2S_BUF_LEN;size_t bytes_read;
    int flash_wr_size = 0;
    //2. Record audio from ADC and save in flash
    uint8_t* i2s_read_buff = (uint8_t*) calloc(2*i2s_read_len, sizeof(char));
    i2s_adc_enable(I2S_NUM);
    uint32_t usecs1, usecs2;
    while(1){
    // while (flash_wr_size < FLASH_RECORD_SIZE) {
        //read data from I2S bus, in this case, from ADC.
        usecs1 = get_usecs();

        i2s_read(I2S_NUM, (void*) i2s_read_buff, 2*I2S_BUF_LEN, &bytes_read, portMAX_DELAY);
        usecs2 = get_usecs();
        ets_printf("At %u, Bytes read: %u After: %u usecs\n", usecs1, bytes_read, usecs2 - usecs1);
        // disp_buf((uint8_t*) i2s_read_buff, bytes_read);
        

        #ifdef WRITE_FLASH
        //save original data from I2S(ADC) into flash.
        esp_partition_write(data_partition, flash_wr_size, i2s_read_buff, i2s_read_len);
        flash_wr_size += i2s_read_len;
        ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
        #endif
    // }
        vTaskDelay(0);
    }
    i2s_adc_disable(I2S_NUM);
    free(i2s_read_buff);
    i2s_read_buff = NULL;

    #ifdef WRITE_FLASH
    free(flash_write_buff);
    flash_write_buff = NULL;
    #endif

    vTaskDelete(NULL);
}