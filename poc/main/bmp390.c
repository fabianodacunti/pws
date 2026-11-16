/*******************************************************************************
 * @file   bmp390.c
 * @brief  Bosch BMP390 barometric pressure sensor.
 *
 * Reference: Bosch BMP390 datasheet, BST-BMP390-DS002, revision 1.7.
 *
 * The sensor delivers raw 24-bit numbers. They only become pressure and
 * temperature after the compensation formulas of the datasheet, which use
 * coefficients measured at the factory and stored in each chip.
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "bmp390.h"

#include <math.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/** @brief I2C clock speed used with the sensor, in Hz. */
#define I2C_SPEED_HZ (100000U)

/** @brief Timeout of each I2C transfer, in ms. */
#define I2C_TIMEOUT_MS (50)

/** @brief Register: chip identification. */
#define REG_CHIP_ID (0x00U)

/** @brief Register: first of 6 data bytes (3 of pressure, then 3 of temperature). */
#define REG_DATA (0x04U)

/** @brief Register: power control (sensor enables and measurement mode). */
#define REG_PWR_CTRL (0x1BU)

/** @brief Register: oversampling of pressure and temperature. */
#define REG_OSR (0x1CU)

/** @brief Register: first of the calibration coefficient bytes (0x31 to 0x45). */
#define REG_CALIB (0x31U)

/** @brief Value of the chip identification register in a BMP390. */
#define CHIP_ID (0x60U)

/** @brief Bytes of calibration coefficients stored in the chip. */
#define CALIB_SIZE (21U)

/** @brief Bytes of one measurement: pressure (3) and temperature (3). */
#define DATA_SIZE (6U)

/** @brief Oversampling setting: pressure x8 (bits 2:0 = 011), temperature x1 (bits 5:3 = 000). */
#define OSR_PRESS_X8_TEMP_X1 (0x03U)

/** @brief Power control: pressure on (bit 0), temperature on (bit 1), forced mode (bits 5:4 = 01). */
#define PWR_CTRL_FORCED (0x13U)

/** @brief Wait for one forced measurement, in ms. With this oversampling it takes about 19 ms. */
#define MEASURE_WAIT_MS (40U)

/** @brief Pascals in one hectopascal. */
#define PA_PER_HPA (100.0f)

/** @brief Calibration coefficients, already converted to floating point. */
typedef struct {
    float par_t1;  /**< Temperature coefficient 1. */
    float par_t2;  /**< Temperature coefficient 2. */
    float par_t3;  /**< Temperature coefficient 3. */
    float par_p1;  /**< Pressure coefficient 1. */
    float par_p2;  /**< Pressure coefficient 2. */
    float par_p3;  /**< Pressure coefficient 3. */
    float par_p4;  /**< Pressure coefficient 4. */
    float par_p5;  /**< Pressure coefficient 5. */
    float par_p6;  /**< Pressure coefficient 6. */
    float par_p7;  /**< Pressure coefficient 7. */
    float par_p8;  /**< Pressure coefficient 8. */
    float par_p9;  /**< Pressure coefficient 9. */
    float par_p10; /**< Pressure coefficient 10. */
    float par_p11; /**< Pressure coefficient 11. */
} bmp390_calib_t;

/** @brief Handle of the sensor on the I2C bus. NULL until bmp390_init() succeeds. */
static i2c_master_dev_handle_t bmp390_dev = NULL;

/** @brief Calibration coefficients read from the chip in bmp390_init(). */
static bmp390_calib_t calib = {0};

/**
 * @brief Reads consecutive registers, starting at the given one.
 * @param[in]  reg    Address of the first register.
 * @param[out] data   Buffer for the bytes read.
 * @param[in]  length Number of bytes to read.
 * @return ESP_OK on success or the I2C driver error.
 */
static esp_err_t read_registers(uint8_t reg, uint8_t *data, size_t length)
{
    return i2c_master_transmit_receive(bmp390_dev, &reg, sizeof(reg), data, length, I2C_TIMEOUT_MS);
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

    return i2c_master_transmit(bmp390_dev, tx, sizeof(tx), I2C_TIMEOUT_MS);
}

/**
 * @brief Converts the calibration bytes stored in the chip to floating point coefficients.
 *
 * Follows datasheet section 8.4: each stored integer is divided by a power of two. ldexpf(x, n) calculates
 * x * 2^n, so dividing by 2^30 is written as ldexpf(x, -30).
 *
 * @param[in] raw CALIB_SIZE bytes read from REG_CALIB, least significant byte first.
 */
static void parse_calibration(const uint8_t *raw)
{
    /* Stored values: 8 or 16 bits, some signed and some unsigned (datasheet table 24). */
    const uint16_t t1 = (uint16_t)((raw[1] << 8) | raw[0]);
    const uint16_t t2 = (uint16_t)((raw[3] << 8) | raw[2]);
    const int8_t t3 = (int8_t)raw[4];
    const int16_t p1 = (int16_t)((raw[6] << 8) | raw[5]);
    const int16_t p2 = (int16_t)((raw[8] << 8) | raw[7]);
    const int8_t p3 = (int8_t)raw[9];
    const int8_t p4 = (int8_t)raw[10];
    const uint16_t p5 = (uint16_t)((raw[12] << 8) | raw[11]);
    const uint16_t p6 = (uint16_t)((raw[14] << 8) | raw[13]);
    const int8_t p7 = (int8_t)raw[15];
    const int8_t p8 = (int8_t)raw[16];
    const int16_t p9 = (int16_t)((raw[18] << 8) | raw[17]);
    const int8_t p10 = (int8_t)raw[19];
    const int8_t p11 = (int8_t)raw[20];

    calib.par_t1 = ldexpf((float)t1, 8);
    calib.par_t2 = ldexpf((float)t2, -30);
    calib.par_t3 = ldexpf((float)t3, -48);
    calib.par_p1 = ldexpf((float)p1 - 16384.0f, -20);
    calib.par_p2 = ldexpf((float)p2 - 16384.0f, -29);
    calib.par_p3 = ldexpf((float)p3, -32);
    calib.par_p4 = ldexpf((float)p4, -37);
    calib.par_p5 = ldexpf((float)p5, 3);
    calib.par_p6 = ldexpf((float)p6, -6);
    calib.par_p7 = ldexpf((float)p7, -8);
    calib.par_p8 = ldexpf((float)p8, -15);
    calib.par_p9 = ldexpf((float)p9, -48);
    calib.par_p10 = ldexpf((float)p10, -48);
    calib.par_p11 = ldexpf((float)p11, -65);
}

/**
 * @brief Converts a raw temperature reading to degrees Celsius (datasheet appendix).
 * @param[in] raw_temp Raw 24-bit temperature.
 * @return Temperature of the chip, in degrees Celsius.
 */
static float compensate_temperature(uint32_t raw_temp)
{
    const float partial = (float)raw_temp - calib.par_t1;

    return (partial * calib.par_t2) + (partial * partial * calib.par_t3);
}

/**
 * @brief Converts a raw pressure reading to pascals (datasheet appendix).
 * @param[in] raw_press     Raw 24-bit pressure.
 * @param[in] temperature_c Compensated temperature of the same measurement, in degrees Celsius.
 * @return Pressure, in Pa.
 */
static float compensate_pressure(uint32_t raw_press, float temperature_c)
{
    const float p = (float)raw_press;
    const float t = temperature_c;
    const float t2 = t * t;
    const float t3 = t2 * t;

    /* Offset and sensitivity, both corrected by the temperature. */
    const float offset = calib.par_p5 + (calib.par_p6 * t) + (calib.par_p7 * t2) + (calib.par_p8 * t3);
    const float sensitivity = calib.par_p1 + (calib.par_p2 * t) + (calib.par_p3 * t2) + (calib.par_p4 * t3);

    /* Second and third order terms, for the small non-linearity of the sensor. */
    const float second_order = p * p * (calib.par_p9 + (calib.par_p10 * t));
    const float third_order = p * p * p * calib.par_p11;

    return offset + (p * sensitivity) + second_order + third_order;
}

esp_err_t bmp390_init(i2c_master_bus_handle_t bus, uint8_t address)
{
    if (bus == NULL || (address != BMP390_ADDRESS_SDO_LOW && address != BMP390_ADDRESS_SDO_HIGH)) {
        return ESP_ERR_INVALID_ARG;
    }

    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = I2C_SPEED_HZ,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &config, &bmp390_dev);
    if (err != ESP_OK) {
        return err;
    }

    /* Make sure the device at this address really is a BMP390. */
    uint8_t chip_id = 0U;
    err = read_registers(REG_CHIP_ID, &chip_id, sizeof(chip_id));
    if (err == ESP_OK && chip_id != CHIP_ID) {
        err = ESP_ERR_NOT_FOUND;
    }

    /* The coefficients never change, so they are read only once. */
    if (err == ESP_OK) {
        uint8_t raw[CALIB_SIZE] = {0};
        err = read_registers(REG_CALIB, raw, sizeof(raw));
        if (err == ESP_OK) {
            parse_calibration(raw);
        }
    }

    if (err == ESP_OK) {
        err = write_register(REG_OSR, OSR_PRESS_X8_TEMP_X1);
    }

    /* On any failure, forget the device so that bmp390_read() reports the module as not ready. */
    if (err != ESP_OK) {
        (void)i2c_master_bus_rm_device(bmp390_dev);
        bmp390_dev = NULL;
    }
    return err;
}

esp_err_t bmp390_read(float *pressure_hpa, float *temperature_c)
{
    if (pressure_hpa == NULL || temperature_c == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (bmp390_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Forced mode: the sensor runs one measurement and goes back to sleep. */
    esp_err_t err = write_register(REG_PWR_CTRL, PWR_CTRL_FORCED);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(MEASURE_WAIT_MS));

    uint8_t data[DATA_SIZE] = {0};
    err = read_registers(REG_DATA, data, sizeof(data));
    if (err != ESP_OK) {
        return err;
    }

    /* Each value has 24 bits and arrives least significant byte first. */
    const uint32_t raw_press = ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | (uint32_t)data[0];
    const uint32_t raw_temp = ((uint32_t)data[5] << 16) | ((uint32_t)data[4] << 8) | (uint32_t)data[3];

    /* The temperature comes first because the pressure formula depends on it. */
    const float temperature = compensate_temperature(raw_temp);

    *temperature_c = temperature;
    *pressure_hpa = compensate_pressure(raw_press, temperature) / PA_PER_HPA;
    return ESP_OK;
}
