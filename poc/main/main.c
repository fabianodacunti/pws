/*******************************************************************************
 * @file   main.c
 * @brief  PWS proof of concept: application entry point.
 *
 * Starts the I2C bus and scans it, printing the address of every device
 * that answers. Then reads the sensors in a loop and prints the readings.
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#include <stdbool.h>
#include <stdio.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bmp390.h"
#include "ltr390.h"
#include "sht45.h"
#include "veml7700.h"

/** @brief GPIO of the I2C SDA line, chosen in menuconfig. */
#define I2C_SDA_GPIO (CONFIG_POC_I2C_SDA_GPIO)

/** @brief GPIO of the I2C SCL line, chosen in menuconfig. */
#define I2C_SCL_GPIO (CONFIG_POC_I2C_SCL_GPIO)

/** @brief Time to wait for an answer from each address during the scan, in ms. */
#define I2C_PROBE_TIMEOUT_MS (50)

/** @brief First address scanned. Lower 7-bit addresses are reserved by the I2C specification. */
#define I2C_FIRST_ADDRESS (0x08)

/** @brief Last address scanned. Higher 7-bit addresses are reserved by the I2C specification. */
#define I2C_LAST_ADDRESS (0x77)

/** @brief Interval between two rounds of readings, in ms. */
#define READ_INTERVAL_MS (2000U)

/**
 * @brief Creates the I2C master bus on the configured pins.
 * @return Handle of the bus. The program stops if the bus cannot be created.
 */
static i2c_master_bus_handle_t i2c_bus_start(void)
{
    const i2c_master_bus_config_t config = {
        .i2c_port = -1, /* let the driver pick a free port */
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, /* the modules have their own pull-ups; this is a fallback */
    };

    i2c_master_bus_handle_t bus = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&config, &bus));
    return bus;
}

/**
 * @brief Probes every valid address and prints a table of the devices found.
 *
 * The table has the same layout as the Linux i2cdetect tool: one row per 16 addresses.
 *
 * @param[in] bus I2C master bus to scan.
 */
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

            esp_err_t err = i2c_master_probe(bus, (uint16_t)address, I2C_PROBE_TIMEOUT_MS);
            if (err == ESP_OK) {
                printf("%02x ", address);
                found++;
            } else {
                printf("-- ");
                /* A timeout, instead of a plain "no answer", points to a wiring problem. */
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

/**
 * @brief Application entry point, called by ESP-IDF after startup.
 */
void app_main(void)
{
    i2c_master_bus_handle_t bus = i2c_bus_start();
    i2c_bus_scan(bus);

    /* Initialize the SHT45 (temperature and relative humidity sensor). */
    esp_err_t err = sht45_init(bus);
    if (err != ESP_OK) {
        printf("SHT45: init error (%s)\n", esp_err_to_name(err));
    }

    /* Initialize the BMP390 (barometric pressure sensor). */
    err = bmp390_init(bus, BMP390_ADDRESS_SDO_HIGH);
    if (err != ESP_OK) {
        printf("BMP390: init error (%s)\n", esp_err_to_name(err));
    }

    /* Initialize the VEML7700 (ambient light sensor). */
    err = veml7700_init(bus);
    if (err != ESP_OK) {
        printf("VEML7700: init error (%s)\n", esp_err_to_name(err));
    }

    /* Initialize the LTR390 (ultraviolet light sensor). */
    err = ltr390_init(bus);
    if (err != ESP_OK) {
        printf("LTR390: init error (%s)\n", esp_err_to_name(err));
    }

    while (true) {
        /* Read the SHT45 (temperature and relative humidity). */
        float temperature = 0.0f;
        float humidity = 0.0f;

        err = sht45_read(&temperature, &humidity);
        if (err == ESP_OK) {
            printf("SHT45: %.2f C, %.1f %%RH\n", temperature, humidity);
        } else {
            printf("SHT45: read error (%s)\n", esp_err_to_name(err));
        }

        /* Read the BMP390 (barometric pressure). */
        float pressure = 0.0f;
        float chip_temperature = 0.0f;

        err = bmp390_read(&pressure, &chip_temperature);
        if (err == ESP_OK) {
            printf("BMP390: %.2f hPa (chip at %.2f C)\n", pressure, chip_temperature);
        } else {
            printf("BMP390: read error (%s)\n", esp_err_to_name(err));
        }

        /* Read the VEML7700 (illuminance). */
        float illuminance = 0.0f;

        err = veml7700_read(&illuminance);
        if (err == ESP_OK) {
            printf("VEML7700: %.1f lx\n", illuminance);
        } else {
            printf("VEML7700: read error (%s)\n", esp_err_to_name(err));
        }

        /* Read the LTR390 (UV index). */
        float uv_index = 0.0f;

        err = ltr390_read(&uv_index);
        if (err == ESP_OK) {
            printf("LTR390: UV index %.1f\n", uv_index);
        } else {
            printf("LTR390: read error (%s)\n", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
