/*******************************************************************************
 * @file   veml7700.c
 * @brief  Vishay VEML7700 ambient light sensor.
 *
 * References: Vishay VEML7700 datasheet (document 84286, rev. 1.8) and
 * application note "Designing the VEML7700 Into an Application" (document
 * 84323, 06-Mar-2025).
 *
 * The sensor covers from darkness to full sunlight by combining a gain with
 * an integration time. This driver uses one fixed combination, called a
 * range. To try another one, change only the RANGE macro below; the table of
 * ranges keeps the gain, the integration time and the resolution consistent.
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "veml7700.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/** @brief I2C address of the VEML7700 (fixed). */
#define VEML7700_ADDRESS (0x10U)

/** @brief I2C clock speed used with the sensor, in Hz. */
#define I2C_SPEED_HZ (100000U)

/** @brief Timeout of each I2C transfer, in ms. */
#define I2C_TIMEOUT_MS (50)

/** @brief Register: configuration (gain, integration time, shutdown). */
#define REG_ALS_CONF (0x00U)

/** @brief Register: latest ambient light measurement, in counts. */
#define REG_ALS (0x04U)

/** @brief Register: device identification. */
#define REG_ID (0x07U)

/** @brief Value of the low byte of the identification register in a VEML7700. */
#define DEVICE_ID (0x81U)

/** @brief Position of the gain field in the configuration register (bits 12:11). */
#define CONF_GAIN_SHIFT (11U)

/** @brief Position of the integration time field in the configuration register (bits 9:6). */
#define CONF_IT_SHIFT (6U)

/** @brief Gain field: gain 1/8. */
#define GAIN_1_8 (0x2U)

/** @brief Gain field: gain 1/4. */
#define GAIN_1_4 (0x3U)

/** @brief Gain field: gain 1. */
#define GAIN_1 (0x0U)

/** @brief Gain field: gain 2. */
#define GAIN_2 (0x1U)

/** @brief Integration time field: 25 ms. */
#define IT_25MS (0xCU)

/** @brief Integration time field: 100 ms. */
#define IT_100MS (0x0U)

/** @brief Integration time field: 800 ms. */
#define IT_800MS (0x3U)

/** @brief Range in use: one of the veml7700_range_id_t values. Change only this line to try another range. */
#define RANGE (RANGE_OUTDOOR)

/** @brief Extra wait after starting the sensor, in ms, over the 2.5 ms it needs to power on. */
#define STARTUP_MARGIN_MS (50U)

/** @brief Ranges that can be selected with the RANGE macro. */
typedef enum {
    RANGE_SUNLIGHT = 0, /**< Gain 1/8, 25 ms: 2.1504 lx per count, counter full at 140 000 lx. */
    RANGE_OUTDOOR,      /**< Gain 1/8, 100 ms: 0.5376 lx per count, counter full at 35 000 lx. */
    RANGE_SHADE,        /**< Gain 1/4, 100 ms: 0.2688 lx per count, counter full at 17 600 lx. */
    RANGE_INDOOR,       /**< Gain 1, 100 ms: 0.0672 lx per count, counter full at 4 400 lx. */
    RANGE_DARK,         /**< Gain 2, 800 ms: 0.0042 lx per count, counter full at 275 lx. */
    RANGE_COUNT,        /**< Number of ranges. Not a range itself. */
} veml7700_range_id_t;

/** @brief Everything that belongs to one range. */
typedef struct {
    uint16_t gain;             /**< Gain field of the configuration register. */
    uint16_t integration_time; /**< Integration time field of the configuration register. */
    uint32_t integration_ms;   /**< Integration time, in ms. */
    float lux_per_count;       /**< Lux represented by one count (application note, resolution table). */
    bool needs_correction;     /**< True for gains 1/8 and 1/4, which are not linear in bright light. */
} veml7700_range_t;

/** @brief Settings of each range, indexed by veml7700_range_id_t. */
static const veml7700_range_t ranges[RANGE_COUNT] = {
    [RANGE_SUNLIGHT] = {GAIN_1_8, IT_25MS, 25U, 2.1504f, true},
    [RANGE_OUTDOOR] = {GAIN_1_8, IT_100MS, 100U, 0.5376f, true},
    [RANGE_SHADE] = {GAIN_1_4, IT_100MS, 100U, 0.2688f, true},
    [RANGE_INDOOR] = {GAIN_1, IT_100MS, 100U, 0.0672f, false},
    [RANGE_DARK] = {GAIN_2, IT_800MS, 800U, 0.0042f, false},
};

/** @brief Handle of the sensor on the I2C bus. NULL until veml7700_init() succeeds. */
static i2c_master_dev_handle_t veml7700_dev = NULL;

/**
 * @brief Reads one 16-bit register.
 * @param[in]  reg   Address of the register.
 * @param[out] value Value read.
 * @return ESP_OK on success or the I2C driver error.
 */
static esp_err_t read_register(uint8_t reg, uint16_t *value)
{
    uint8_t rx[2] = {0};

    esp_err_t err = i2c_master_transmit_receive(veml7700_dev, &reg, sizeof(reg), rx, sizeof(rx), I2C_TIMEOUT_MS);
    if (err == ESP_OK) {
        /* Registers travel least significant byte first. */
        *value = (uint16_t)((rx[1] << 8) | rx[0]);
    }
    return err;
}

/**
 * @brief Writes one 16-bit register.
 * @param[in] reg   Address of the register.
 * @param[in] value Value to write.
 * @return ESP_OK on success or the I2C driver error.
 */
static esp_err_t write_register(uint8_t reg, uint16_t value)
{
    const uint8_t tx[3] = {reg, (uint8_t)(value & 0xFFU), (uint8_t)(value >> 8)};

    return i2c_master_transmit(veml7700_dev, tx, sizeof(tx), I2C_TIMEOUT_MS);
}

/**
 * @brief Corrects the small non-linearity of gains 1/8 and 1/4 in bright light.
 *
 * Polynomial of the application note. For low values the result is practically the input.
 *
 * @param[in] lux Illuminance calculated from the counts, in lux.
 * @return Corrected illuminance, in lux.
 */
static float correct_lux(float lux)
{
    const float lux2 = lux * lux;

    return (6.0135e-13f * lux2 * lux2) - (9.3924e-9f * lux2 * lux) + (8.1488e-5f * lux2) + (1.0023f * lux);
}

esp_err_t veml7700_init(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VEML7700_ADDRESS,
        .scl_speed_hz = I2C_SPEED_HZ,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &config, &veml7700_dev);
    if (err != ESP_OK) {
        return err;
    }

    /* Make sure the device at this address really is a VEML7700. */
    uint16_t id = 0U;
    err = read_register(REG_ID, &id);
    if (err == ESP_OK && (id & 0xFFU) != DEVICE_ID) {
        err = ESP_ERR_NOT_FOUND;
    }

    /* Writing the range with the shutdown bit (bit 0) at zero also turns the sensor on. */
    if (err == ESP_OK) {
        const veml7700_range_t *range = &ranges[RANGE];
        const uint16_t conf = (uint16_t)((range->gain << CONF_GAIN_SHIFT) | (range->integration_time << CONF_IT_SHIFT));
        err = write_register(REG_ALS_CONF, conf);
    }

    if (err == ESP_OK) {
        /* From here on the sensor measures by itself; wait for the first result. */
        vTaskDelay(pdMS_TO_TICKS(ranges[RANGE].integration_ms + STARTUP_MARGIN_MS));
    } else {
        /* On any failure, forget the device so that veml7700_read() reports the module as not ready. */
        (void)i2c_master_bus_rm_device(veml7700_dev);
        veml7700_dev = NULL;
    }
    return err;
}

esp_err_t veml7700_read(float *illuminance_lux)
{
    if (illuminance_lux == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (veml7700_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t counts = 0U;
    esp_err_t err = read_register(REG_ALS, &counts);
    if (err != ESP_OK) {
        return err;
    }

    const veml7700_range_t *range = &ranges[RANGE];
    float lux = (float)counts * range->lux_per_count;

    /* The correction only applies to the two lowest gains. */
    if (range->needs_correction) {
        lux = correct_lux(lux);
    }
    *illuminance_lux = lux;
    return ESP_OK;
}
