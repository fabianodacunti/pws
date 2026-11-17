/*******************************************************************************
 * @file   dashboard.h
 * @brief  Terminal dashboard with the latest reading of every sensor.
 *
 * Instead of printing a new block of lines at every round, the dashboard
 * redraws the same screen in place, using ANSI escape codes. These are short
 * sequences, starting with the ESC character, that tell the terminal to move
 * the cursor, clear part of the screen or change the color of the text.
 *
 * @note   Needs a terminal that understands ANSI escape codes, such as the
 *         monitor of the ESP-IDF extension for VS Code. For other terminals,
 *         turn off "Dashboard with ANSI codes" in menuconfig: the readings are
 *         then printed as plain text, one block per round.
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef DASHBOARD_H
#define DASHBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/** @brief Status of a sensor that is disabled in menuconfig, and so is never read. */
#define SENSOR_DISABLED (ESP_ERR_NOT_SUPPORTED)

/** @brief The latest reading of every sensor, and whether each reading is valid. */
typedef struct {
    esp_err_t sht45_status;    /**< ESP_OK, the error of the last reading, or SENSOR_DISABLED. */
    float temperature_c;       /**< Air temperature, in degrees Celsius. */
    float humidity_pct;        /**< Relative humidity, in %. */
    esp_err_t bmp390_status;   /**< ESP_OK, the error of the last reading, or SENSOR_DISABLED. */
    float pressure_hpa;        /**< Atmospheric pressure, in hPa. */
    esp_err_t veml7700_status; /**< ESP_OK, the error of the last reading, or SENSOR_DISABLED. */
    float illuminance_lux;     /**< Illuminance, in lux. */
    esp_err_t ltr390_status;   /**< ESP_OK, the error of the last reading, or SENSOR_DISABLED. */
    float uv_index;            /**< UV index (no unit). */
} readings_t;

/**
 * @brief Clears the terminal. Call once, before the first dashboard_draw().
 */
void dashboard_start(void);

/**
 * @brief Draws the dashboard over the previous one.
 *
 * A reading is shown only when its status is ESP_OK; otherwise the dashboard shows "disabled" or a short message
 * about what went wrong. Does nothing if readings is NULL.
 *
 * @param[in] readings Readings to show. Must not be NULL.
 */
void dashboard_draw(const readings_t *readings);

#ifdef __cplusplus
}
#endif

#endif /* DASHBOARD_H */
