/*******************************************************************************
 * @file   ltr390.c
 * @brief  Lite-On LTR390 ultraviolet light sensor.
 *
 * Reference: Lite-On LTR-390UV-01 datasheet, version 1.1.
 *
 * The sensor is used with gain 18 and 20-bit resolution, the only setting
 * for which the datasheet gives the UV sensitivity.
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "ltr390.h"

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/** @brief I2C address of the LTR390 (fixed). */
#define LTR390_ADDRESS (0x53U)

/** @brief I2C clock speed used with the sensor, in Hz. */
#define I2C_SPEED_HZ (100000U)

/** @brief Timeout of each I2C transfer, in ms. */
#define I2C_TIMEOUT_MS (50)

/** @brief Register: operating mode (standby or active, ambient light or UV). */
#define REG_MAIN_CTRL (0x00U)

/** @brief Register: resolution and measurement rate. */
#define REG_MEAS_RATE (0x04U)

/** @brief Register: gain. */
#define REG_GAIN (0x05U)

/** @brief Register: part number (high nibble) and revision (low nibble). */
#define REG_PART_ID (0x06U)

/** @brief Register: first of the 3 bytes of the latest UV measurement. */
#define REG_UVS_DATA (0x10U)

/** @brief Part number of the LTR390, in the high nibble of the part ID register. */
#define PART_NUMBER (0xBU)

/** @brief Main control: sensor active (bit 1) in UV mode (bit 3). */
#define MAIN_CTRL_UVS_ACTIVE (0x0AU)

/** @brief Resolution and rate: 20 bits (bits 6:4 = 000), one measurement every 500 ms (bits 2:0 = 100). */
#define MEAS_RATE_20BIT_500MS (0x04U)

/** @brief Gain: 18 (bits 2:0 = 100). */
#define GAIN_18 (0x04U)

/** @brief Bytes of one UV measurement. */
#define DATA_SIZE (3U)

/** @brief Valid bits of the most significant data byte: the measurement has 20 bits. */
#define DATA_MSB_MASK (0x0FU)

/** @brief Wait for the first measurement after starting the sensor, in ms (one measurement period plus a margin). */
#define FIRST_MEASURE_WAIT_MS (600U)

/**
 * @brief Counts that correspond to one unit of UV index, with gain 18 and 20 bits.
 *
 * Copies of the datasheet version 1.1 disagree on this value: some give 2300 and others 1810. To be confirmed at
 * the bench, against the official UV index of the day.
 */
#define UV_SENSITIVITY (2300.0f)

/** @brief Handle of the sensor on the I2C bus. NULL until ltr390_init() succeeds. */
static i2c_master_dev_handle_t ltr390_dev = NULL;

/**
 * @brief Reads consecutive registers, starting at the given one.
 * @param[in]  reg    Address of the first register.
 * @param[out] data   Buffer for the bytes read.
 * @param[in]  length Number of bytes to read.
 * @return ESP_OK on success or the I2C driver error.
 */
static esp_err_t read_registers(uint8_t reg, uint8_t *data, size_t length)
{
    return i2c_master_transmit_receive(ltr390_dev, &reg, sizeof(reg), data, length, I2C_TIMEOUT_MS);
}

/**
 * @brief Writes one register.
 * @param[in] reg   Address of the register.
 * @param[in] value Value to write.
 * @return ESP_OK on success or the I2C driver error.
 */
static esp_err_t write_register(uint8_t reg, uint8_t value)
{
    const uint8_t tx[2] = {reg, value};

    return i2c_master_transmit(ltr390_dev, tx, sizeof(tx), I2C_TIMEOUT_MS);
}

esp_err_t ltr390_init(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LTR390_ADDRESS,
        .scl_speed_hz = I2C_SPEED_HZ,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &config, &ltr390_dev);
    if (err != ESP_OK) {
        return err;
    }

    /* Make sure the device at this address really is an LTR390. */
    uint8_t part_id = 0U;
    err = read_registers(REG_PART_ID, &part_id, sizeof(part_id));
    if (err == ESP_OK && (part_id >> 4) != PART_NUMBER) {
        err = ESP_ERR_NOT_FOUND;
    }

    /* Resolution and gain first, then the mode, which takes the sensor out of standby. */
    if (err == ESP_OK) {
        err = write_register(REG_MEAS_RATE, MEAS_RATE_20BIT_500MS);
    }
    if (err == ESP_OK) {
        err = write_register(REG_GAIN, GAIN_18);
    }
    if (err == ESP_OK) {
        err = write_register(REG_MAIN_CTRL, MAIN_CTRL_UVS_ACTIVE);
    }

    if (err == ESP_OK) {
        /* From here on the sensor measures by itself; wait for the first result. */
        vTaskDelay(pdMS_TO_TICKS(FIRST_MEASURE_WAIT_MS));
    } else {
        /* On any failure, forget the device so that ltr390_read() reports the module as not ready. */
        (void)i2c_master_bus_rm_device(ltr390_dev);
        ltr390_dev = NULL;
    }
    return err;
}

esp_err_t ltr390_read(float *uv_index)
{
    if (uv_index == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ltr390_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t data[DATA_SIZE] = {0};
    esp_err_t err = read_registers(REG_UVS_DATA, data, sizeof(data));
    if (err != ESP_OK) {
        return err;
    }

    /* The 20-bit value arrives least significant byte first. */
    const uint32_t counts = ((uint32_t)(data[2] & DATA_MSB_MASK) << 16) | ((uint32_t)data[1] << 8) | (uint32_t)data[0];

    *uv_index = (float)counts / UV_SENSITIVITY;
    return ESP_OK;
}
