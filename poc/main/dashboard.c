/*******************************************************************************
 * @file   dashboard.c
 * @brief  Terminal dashboard with the latest reading of every sensor.
 *
 * Every dashboard_draw() sends the cursor back to the top-left corner and
 * writes the screen again, line by line. Each line ends by erasing whatever
 * was left over from the previous, possibly longer, text.
 *
 * With the ANSI option turned off in menuconfig, the escape codes are replaced
 * by empty text and every round is printed as a new block of lines.
 *
 * @author Fabiano Bozzi D'Acunti
 * @copyright SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "dashboard.h"

#include <stddef.h>
#include <stdio.h>

#if CONFIG_POC_DASHBOARD_ANSI

/** @brief ANSI: clear the whole screen. */
#define ANSI_CLEAR_SCREEN ("\033[2J")

/** @brief ANSI: move the cursor to the top-left corner. */
#define ANSI_CURSOR_HOME ("\033[H")

/** @brief ANSI: erase from the cursor to the end of the line. */
#define ANSI_CLEAR_LINE ("\033[K")

/** @brief ANSI: back to the normal text style. */
#define ANSI_RESET ("\033[0m")

/** @brief ANSI: bold text. */
#define ANSI_BOLD ("\033[1m")

/** @brief ANSI: dim text. */
#define ANSI_DIM ("\033[2m")

/** @brief ANSI: red text. */
#define ANSI_RED ("\033[31m")

#else

/* Plain text mode: no escape codes, and every round starts after a blank line instead of going back to the top. */
#define ANSI_CLEAR_SCREEN ("")
#define ANSI_CURSOR_HOME  ("\n")
#define ANSI_CLEAR_LINE   ("")
#define ANSI_RESET        ("")
#define ANSI_BOLD         ("")
#define ANSI_DIM          ("")
#define ANSI_RED          ("")

#endif

/** @brief Horizontal line that separates the title and the readings. */
#define SEPARATOR ("------------------------------------------------------------")

/**
 * @brief Translates the error of a reading into a short message for the person looking at the dashboard.
 * @param[in] status Error returned by the sensor module.
 * @return The message to show.
 */
static const char *error_text(esp_err_t status)
{
    switch (status) {
    case ESP_ERR_INVALID_STATE:
        /* The sensor did not start, so it was never read. */
        return "not found, check the wiring and the address";
    case ESP_ERR_TIMEOUT:
        return "not responding, check the wiring";
    case ESP_ERR_INVALID_CRC:
        return "corrupted data, check the wiring";
    default:
        return "reading failed";
    }
}

/**
 * @brief Prints one line of the dashboard: the label and then the value, "disabled" or the error.
 * @param[in] label    Name of the quantity and of the sensor that measures it.
 * @param[in] status   ESP_OK to show the value, SENSOR_DISABLED to show "disabled", any other error to explain it.
 * @param[in] value    Value to show when the status is ESP_OK.
 * @param[in] decimals Digits after the decimal point.
 * @param[in] unit     Unit of the value.
 */
static void draw_line(const char *label, esp_err_t status, float value, int decimals, const char *unit)
{
    printf(" %-24s ", label);

    if (status == ESP_OK) {
        printf("%s%9.*f%s %s", ANSI_BOLD, decimals, (double)value, ANSI_RESET, unit);
    } else if (status == SENSOR_DISABLED) {
        printf("%sdisabled%s", ANSI_DIM, ANSI_RESET);
    } else {
        printf("%s%s%s", ANSI_RED, error_text(status), ANSI_RESET);
    }

    printf("%s\n", ANSI_CLEAR_LINE);
}

void dashboard_start(void)
{
    printf("%s", ANSI_CLEAR_SCREEN);
}

void dashboard_draw(const readings_t *readings)
{
    if (readings == NULL) {
        return;
    }

    printf("%s %sPWS proof of concept%s%s\n", ANSI_CURSOR_HOME, ANSI_BOLD, ANSI_RESET, ANSI_CLEAR_LINE);
    printf(" %s%s\n", SEPARATOR, ANSI_CLEAR_LINE);

    draw_line("Temperature (SHT45)", readings->sht45_status, readings->temperature_c, 2, "C");
    draw_line("Humidity (SHT45)", readings->sht45_status, readings->humidity_pct, 1, "%");
    draw_line("Pressure (BMP390)", readings->bmp390_status, readings->pressure_hpa, 2, "hPa");
    draw_line("Illuminance (VEML7700)", readings->veml7700_status, readings->illuminance_lux, 1, "lx");
    draw_line("UV index (LTR390)", readings->ltr390_status, readings->uv_index, 1, "");

    printf(" %s%s\n", SEPARATOR, ANSI_CLEAR_LINE);
}
