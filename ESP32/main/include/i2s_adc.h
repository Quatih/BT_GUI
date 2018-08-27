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
#include "audio_example_file.h"
#define V_REF 1100

#define PARTITION_NAME "storage"

// #define WRITE_FLASH
//i2s number
#define I2S_NUM (0)
//i2s Sample rate
#define I2S_SAMPLE_RATE (20480)

//i2s data bits
#define I2S_SAMPLE_BITS (16)
#define I2S_DAC_CHANNEL DAC1_CHANNEL_1 // GPIO_36, SVP pin

//I2S read buffer length [bytes]
#define I2S_BUF_LEN (256)
//I2S data format
#define I2S_FORMAT (I2S_CHANNEL_FMT_ONLY_RIGHT)
//I2S channel number
#define I2S_CHANNEL_NUM ((I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))
//I2S built-in ADC unit
#define I2S_ADC_UNIT ADC_UNIT_1
//I2S built-in ADC channel
#define I2S_ADC_CHANNEL ADC1_CHANNEL_0 // GPIO_36, SVP pin

#ifdef WRITE_FLASH
//flash record size, for recording 5 seconds' data
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * 5)
#define FLASH_ERASE_SIZE (FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE == 0) ? FLASH_RECORD_SIZE : FLASH_RECORD_SIZE + (FLASH_SECTOR_SIZE - FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE)
//sector size of flash
#define FLASH_SECTOR_SIZE (0x1000)
//flash read / write address
#define FLASH_ADDR (0x200000)
#endif

/**
 * @brief I2S ADC mode init.
 */
void i2s_init()
{
    i2s_config_t i2s_config_adc = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN,
        .sample_rate =  I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_SAMPLE_BITS,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .channel_format = I2S_FORMAT,
        .intr_alloc_flags = 0,
        .dma_buf_count = 2,
        .dma_buf_len = I2S_BUF_LEN,
        .use_apll = false
    };

    //install and start i2s driver
    i2s_driver_install(I2S_NUM, &i2s_config_adc, 0, NULL);
    //init ADC pad
    i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL);
}

/*
 * @brief erase flash for recording
 */
void erase_flash()
{
    #ifdef WRITE_FLASH
    printf("Erasing flash \n");
    const esp_partition_t *data_partition = NULL;
    data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);
    if (data_partition != NULL) {
        printf("partiton addr: 0x%08x; size: %d; label: %s\n", data_partition->address, data_partition->size, data_partition->label);
    }
    printf("Erase size: %d Bytes\n", FLASH_ERASE_SIZE);
    ESP_ERROR_CHECK(esp_partition_erase_range(data_partition, 0, FLASH_ERASE_SIZE));
    #endif
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