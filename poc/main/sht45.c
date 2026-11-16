/*******************************************************************************
 * @file   sht45.c
 * @brief  Sensirion SHT45 temperature and relative humidity sensor.
 *
 * Reference: Sensirion SHT4x datasheet, version 7.3.
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "sht45.h"

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/** @brief I2C address of the SHT45 (SHT45-AD1B variant). */
#define SHT45_ADDRESS (0x44U)

/** @brief I2C clock speed used with the sensor, in Hz. */
#define I2C_SPEED_HZ (100000U)

/** @brief Timeout of each I2C transfer, in ms. */
#define I2C_TIMEOUT_MS (50)

/** @brief Command: measure temperature and humidity with high precision. */
#define CMD_MEASURE (0xFDU)

/** @brief Wait after the command, in ms. The measurement takes up to 8.3 ms. */
#define MEASURE_WAIT_MS (20U)

/** @brief Bytes in the answer: temperature (2), CRC (1), humidity (2), CRC (1). */
#define ANSWER_SIZE (6U)

/** @brief CRC-8 polynomial used by the sensor (x^8 + x^5 + x^4 + 1). */
#define CRC_POLYNOMIAL (0x31U)

/** @brief CRC-8 initial value used by the sensor. */
#define CRC_INIT (0xFFU)

/** @brief Lowest relative humidity reported, in %. */
#define HUMIDITY_MIN_PCT (0.0f)

/** @brief Highest relative humidity reported, in %. */
#define HUMIDITY_MAX_PCT (100.0f)

/** @brief Handle of the sensor on the I2C bus. NULL until sht45_init() succeeds. */
static i2c_master_dev_handle_t sht45_dev = NULL;

/**
 * @brief Calculates the CRC-8 checksum the sensor appends to each value.
 * @param[in] data   Bytes to check.
 * @param[in] length Number of bytes in data.
 * @return The checksum of the bytes.
 */
static uint8_t crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = CRC_INIT;

    for (size_t i = 0U; i < length; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            /* When the top bit is set, shifting must be followed by the polynomial. */
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1) ^ CRC_POLYNOMIAL);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

esp_err_t sht45_init(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHT45_ADDRESS,
        .scl_speed_hz = I2C_SPEED_HZ,
    };

    /* This only registers the sensor in the driver; nothing is sent on the bus yet. */
    return i2c_master_bus_add_device(bus, &config, &sht45_dev);
}

esp_err_t sht45_read(float *temperature_c, float *humidity_pct)
{
    if (temperature_c == NULL || humidity_pct == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (sht45_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Start the measurement and give the sensor time to finish it. */
    const uint8_t command = CMD_MEASURE;
    esp_err_t err = i2c_master_transmit(sht45_dev, &command, sizeof(command), I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(MEASURE_WAIT_MS));

    uint8_t rx[ANSWER_SIZE] = {0};
    err = i2c_master_receive(sht45_dev, rx, sizeof(rx), I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    /* Each 2-byte value is followed by its own checksum. */
    if (crc8(&rx[0], 2U) != rx[2] || crc8(&rx[3], 2U) != rx[5]) {
        return ESP_ERR_INVALID_CRC;
    }

    /* Values arrive most significant byte first. */
    const uint16_t t_ticks = (uint16_t)((rx[0] << 8) | rx[1]);
    const uint16_t rh_ticks = (uint16_t)((rx[3] << 8) | rx[4]);

    /* Conversion formulas of the datasheet (section 4.6). */
    *temperature_c = -45.0f + (175.0f * (float)t_ticks / 65535.0f);
    float humidity = -6.0f + (125.0f * (float)rh_ticks / 65535.0f);

    /* The formula can go slightly beyond the physical range, so the datasheet recommends limiting the result. */
    if (humidity > HUMIDITY_MAX_PCT) {
        humidity = HUMIDITY_MAX_PCT;
    } else if (humidity < HUMIDITY_MIN_PCT) {
        humidity = HUMIDITY_MIN_PCT;
    }
    *humidity_pct = humidity;
    return ESP_OK;
}
