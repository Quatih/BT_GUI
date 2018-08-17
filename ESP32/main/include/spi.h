#ifndef _SPI_H
#define _SPI_H

#include <string.h> //needed for memset
#include "soc/gpio_struct.h"
#include "driver/spi_master.h"
#include <stdint.h>
#define PIN_NUM_MISO 17
#define PIN_NUM_MOSI 19
#define PIN_NUM_CLK  18
#define PIN_NUM_CS 21

#define PARALLEL_LINES 16

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

 //void spi_init(spi_device_handle_t *spi) {
void spi_init() {
	esp_err_t ret;
	//spi_device_handle_t spi; //commenting out because I don't know how to header files
    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=PARALLEL_LINES*320*2+8
};

    spi_device_interface_config_t devcfg={
        .clock_speed_hz=2*1000*1000,           //making SPI clock slower so it's easier to analyze
        //.clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz //MCP4342 is 10MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        //.pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
};
	 //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
}

//void rheo_send_data(spi_device_handle_t *spi, const uint8_t *data)
 void rheo_send_data(const uint8_t *data)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=16;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

#endif