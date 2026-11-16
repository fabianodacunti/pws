/*******************************************************************************
 * @file   ltr390.h
 * @brief  Lite-On LTR390 ultraviolet light sensor.
 *
 * Minimal driver for the proof of concept: the sensor measures UV light
 * continuously, and each call returns the latest UV index.
 *
 * The sensor does not deliver the UV index itself. It counts UV light, and
 * the index is calculated from the sensitivity given in the datasheet.
 *
 * @note   Singleton: the module handles a single sensor. Call ltr390_init()
 *         once before ltr390_read().
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef LTR390_H
#define LTR390_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c_master.h"
#include "esp_err.h"

/**
 * @brief Adds the sensor to the I2C bus, checks its identity and starts the UV measurements.
 *
 * Waits for the first measurement to finish, which takes about half a second.
 *
 * @param[in] bus I2C master bus the sensor is connected to. Must not be NULL.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if bus is NULL, ESP_ERR_NOT_FOUND if the device at the address is
 *         not an LTR390, or the I2C driver error.
 */
esp_err_t ltr390_init(i2c_master_bus_handle_t bus);

/**
 * @brief Returns the UV index calculated from the latest measurement.
 * @param[out] uv_index UV index (no unit). Must not be NULL.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if the pointer is NULL, ESP_ERR_INVALID_STATE if ltr390_init() did
 *         not succeed, or the I2C driver error.
 */
esp_err_t ltr390_read(float *uv_index);

#ifdef __cplusplus
}
#endif

#endif /* LTR390_H */
