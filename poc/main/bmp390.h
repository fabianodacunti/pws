/*******************************************************************************
 * @file   bmp390.h
 * @brief  Bosch BMP390 barometric pressure sensor.
 *
 * Minimal driver for the proof of concept: one measurement per call, over
 * I2C, already compensated with the calibration coefficients stored in the
 * chip.
 *
 * @note   Singleton: the module handles a single sensor. Call bmp390_init()
 *         once before bmp390_read().
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef BMP390_H
#define BMP390_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

/** @brief I2C address when the SDO pin is low. */
#define BMP390_ADDRESS_SDO_LOW (0x76U)

/** @brief I2C address when the SDO pin is high (the usual choice on modules). */
#define BMP390_ADDRESS_SDO_HIGH (0x77U)

/**
 * @brief Adds the sensor to the I2C bus, checks its identity and reads its calibration coefficients.
 * @param[in] bus     I2C master bus the sensor is connected to. Must not be NULL.
 * @param[in] address BMP390_ADDRESS_SDO_LOW or BMP390_ADDRESS_SDO_HIGH.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG for a NULL bus or another address, ESP_ERR_NOT_FOUND if the
 *         device at the address is not a BMP390, or the I2C driver error.
 */
esp_err_t bmp390_init(i2c_master_bus_handle_t bus, uint8_t address);

/**
 * @brief Runs one measurement and returns the compensated values.
 * @param[out] pressure_hpa  Pressure at the sensor, in hPa. Must not be NULL.
 * @param[out] temperature_c Temperature of the chip, in degrees Celsius. Must not be NULL.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if a pointer is NULL, ESP_ERR_INVALID_STATE if bmp390_init() did
 *         not succeed, or the I2C driver error.
 */
esp_err_t bmp390_read(float *pressure_hpa, float *temperature_c);

#ifdef __cplusplus
}
#endif

#endif /* BMP390_H */
