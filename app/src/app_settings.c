/*
 * Persistent application settings ("app/..." in the settings subsystem).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include "app.h"

LOG_MODULE_REGISTER(app_settings, CONFIG_APP_LOG_LEVEL);

static uint32_t interval_ms = CONFIG_APP_SAMPLE_INTERVAL_MS;

static int app_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	if (settings_name_steq(name, "interval", NULL) && len == sizeof(interval_ms)) {
		uint32_t v;

		if (read_cb(cb_arg, &v, sizeof(v)) == sizeof(v) && v >= 100) {
			interval_ms = v;
		}
		return 0;
	}
	return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(app, "app", NULL, app_set, NULL, NULL);

int app_settings_init(void)
{
	int err = settings_subsys_init();

	if (err) {
		return err;
	}
	err = settings_load();
	LOG_INF("sample interval %u ms", interval_ms);
	return err;
}

uint32_t app_settings_interval_ms(void)
{
	return interval_ms;
}

int app_settings_set_interval_ms(uint32_t ms)
{
	if (ms < 100) {
		return -EINVAL;
	}
	interval_ms = ms;
	return settings_save_one("app/interval", &interval_ms, sizeof(interval_ms));
}
