/*******************************************************************************
 * @file   sht45.h
 * @brief  Sensirion SHT45 temperature and relative humidity sensor.
 *
 * Minimal driver for the proof of concept: one high precision measurement
 * per call, over I2C, with the sensor at its default address.
 *
 * @note   Singleton: the module handles a single sensor. Call sht45_init()
 *         once before sht45_read().
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef SHT45_H
#define SHT45_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c_master.h"
#include "esp_err.h"

/**
 * @brief Adds the sensor to the I2C bus.
 * @param[in] bus I2C master bus the sensor is connected to. Must not be NULL.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if bus is NULL, or the I2C driver error.
 */
esp_err_t sht45_init(i2c_master_bus_handle_t bus);

/**
 * @brief Runs one measurement and returns the converted values.
 * @param[out] temperature_c Temperature, in degrees Celsius. Must not be NULL.
 * @param[out] humidity_pct  Relative humidity, in %, limited to 0 to 100. Must not be NULL.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if a pointer is NULL, ESP_ERR_INVALID_STATE if sht45_init() was
 *         not called, ESP_ERR_INVALID_CRC if the data arrived corrupted, or the I2C driver error.
 */
esp_err_t sht45_read(float *temperature_c, float *humidity_pct);

#ifdef __cplusplus
}
#endif

#endif /* SHT45_H */
