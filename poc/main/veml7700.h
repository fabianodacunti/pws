/*******************************************************************************
 * @file   veml7700.h
 * @brief  Vishay VEML7700 ambient light sensor.
 *
 * Minimal driver for the proof of concept: the sensor measures continuously
 * with one fixed gain and integration time, and each call returns the latest
 * illuminance, in lux.
 *
 * The fixed range is chosen by the RANGE macro in veml7700.c, where the
 * available ranges are listed.
 *
 * @note   Singleton: the module handles a single sensor. Call veml7700_init()
 *         once before veml7700_read().
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef VEML7700_H
#define VEML7700_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c_master.h"
#include "esp_err.h"

/**
 * @brief Adds the sensor to the I2C bus, checks its identity and starts the measurements.
 *
 * Waits for the first measurement to finish, which takes one integration time.
 *
 * @param[in] bus I2C master bus the sensor is connected to. Must not be NULL.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if bus is NULL, ESP_ERR_NOT_FOUND if the device at the address is
 *         not a VEML7700, or the I2C driver error.
 */
esp_err_t veml7700_init(i2c_master_bus_handle_t bus);

/**
 * @brief Returns the latest illuminance measured by the sensor.
 * @param[out] illuminance_lux Illuminance, in lux. Must not be NULL.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if the pointer is NULL, ESP_ERR_INVALID_STATE if veml7700_init()
 *         did not succeed, or the I2C driver error.
 */
esp_err_t veml7700_read(float *illuminance_lux);

#ifdef __cplusplus
}
#endif

#endif /* VEML7700_H */
