/*
 * Twin Sensor Node application entry point.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/logging/log.h>

#include <app_version.h>
#include "app.h"

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

#define WDT_TIMEOUT_MS 8000
#define HEARTBEAT_MS   1000

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct gpio_callback button_cb;
static int wdt_channel = -1;

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	LOG_INF("button: pressed");
}

static int io_init(void)
{
	int err;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&button)) {
		return -ENODEV;
	}
	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}
	err = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (err) {
		return err;
	}
	err = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		return err;
	}
	gpio_init_callback(&button_cb, button_pressed, BIT(button.pin));
	return gpio_add_callback_dt(&button, &button_cb);
}

static int watchdog_init(void)
{
	struct wdt_timeout_cfg cfg = {
		.window = {.min = 0, .max = WDT_TIMEOUT_MS},
		.flags = WDT_FLAG_RESET_SOC,
	};

	if (!device_is_ready(wdt)) {
		return -ENODEV;
	}
	wdt_channel = wdt_install_timeout(wdt, &cfg);
	if (wdt_channel < 0) {
		return wdt_channel;
	}
	return wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
}

/*
 * Confirm the running image only after the product's critical functions came
 * up. If an update is broken, MCUboot reverts to the previous image on the
 * next reset.
 */
static void confirm_image(bool healthy)
{
	if (boot_is_img_confirmed()) {
		return;
	}
	if (!healthy) {
		LOG_ERR("self-test failed, image left unconfirmed (will revert on reset)");
		return;
	}
	if (boot_write_img_confirmed() == 0) {
		LOG_INF("image confirmed");
	}
}

int main(void)
{
	bool healthy = true;
	int err;

	LOG_INF("twin_sensor v%s (%s) starting", APP_VERSION_STRING, CONFIG_BOARD_TARGET);

	err = io_init();
	if (err) {
		LOG_ERR("io init failed (%d)", err);
		healthy = false;
	}
	err = app_settings_init();
	if (err) {
		LOG_ERR("settings init failed (%d)", err);
		healthy = false;
	}
	err = sensors_init();
	if (err) {
		LOG_ERR("sensor init failed (%d)", err);
		healthy = false;
	}
	if (IS_ENABLED(CONFIG_APP_BLE)) {
		err = ble_init();
		if (err) {
			LOG_ERR("bluetooth init failed (%d)", err);
			healthy = false;
		}
	}
	err = watchdog_init();
	if (err) {
		LOG_WRN("watchdog not started (%d)", err);
	}

	confirm_image(healthy);
	LOG_INF("self-test %s", healthy ? "passed" : "FAILED");

	while (true) {
		gpio_pin_toggle_dt(&led);
		if (wdt_channel >= 0) {
			wdt_feed(wdt, wdt_channel);
		}
		k_msleep(HEARTBEAT_MS);
	}
	return 0;
}
