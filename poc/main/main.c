/*
 * PWS proof of concept.
 *
 * Step 1: start the I2C bus and scan it, printing the address of every
 * device that answers. The sensor modules used in the proof of concept
 * should show up at their datasheet addresses.
 */

#include <stdio.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_SDA_GPIO        CONFIG_POC_I2C_SDA_GPIO
#define I2C_SCL_GPIO        CONFIG_POC_I2C_SCL_GPIO
#define I2C_PROBE_TIMEOUT_MS 50

/* 7-bit addresses outside this range are reserved by the I2C specification. */
#define I2C_FIRST_ADDRESS   0x08
#define I2C_LAST_ADDRESS    0x77

static i2c_master_bus_handle_t i2c_bus_start(void)
{
    const i2c_master_bus_config_t config = {
        .i2c_port = -1,                     /* let the driver pick a free port */
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, /* the modules have their own pull-ups; this is a fallback */
    };

    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&config, &bus));
    return bus;
}

/* Prints a table like the Linux i2cdetect tool: one row per 16 addresses. */
static void i2c_bus_scan(i2c_master_bus_handle_t bus)
{
    int found = 0;
    int timeouts = 0;

    printf("\nI2C scan (SDA = GPIO%d, SCL = GPIO%d)\n\n", I2C_SDA_GPIO, I2C_SCL_GPIO);
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");

    for (int row = 0x00; row <= 0x70; row += 0x10) {
        printf("%02x: ", row);
        for (int col = 0; col < 0x10; col++) {
            int address = row + col;

            if (address < I2C_FIRST_ADDRESS || address > I2C_LAST_ADDRESS) {
                printf("   ");
                continue;
            }

            esp_err_t err = i2c_master_probe(bus, address, I2C_PROBE_TIMEOUT_MS);
            if (err == ESP_OK) {
                printf("%02x ", address);
                found++;
            } else {
                printf("-- ");
                if (err == ESP_ERR_TIMEOUT) {
                    timeouts++;
                }
            }
        }
        printf("\n");
    }

    printf("\n%d device(s) found\n", found);
    if (timeouts > 0) {
        printf("Warning: %d address(es) timed out; check the wiring and the pull-up resistors\n", timeouts);
    }
}

void app_main(void)
{
    i2c_master_bus_handle_t bus = i2c_bus_start();
    i2c_bus_scan(bus);
}
