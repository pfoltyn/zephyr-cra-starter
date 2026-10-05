/*
 * Periodic sampling of the environment sensor and accelerometer.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "app.h"

LOG_MODULE_REGISTER(sensors, CONFIG_APP_LOG_LEVEL);

static const struct device *const env = DEVICE_DT_GET(DT_ALIAS(env));
static const struct device *const accel = DEVICE_DT_GET(DT_ALIAS(accel));

static struct app_reading latest;
static K_MUTEX_DEFINE(latest_lock);
static void sample_work_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sample_work, sample_work_fn);

static bool read_env(struct app_reading *r)
{
	struct sensor_value t, h, p;

	if (sensor_sample_fetch(env) ||
	    sensor_channel_get(env, SENSOR_CHAN_AMBIENT_TEMP, &t) ||
	    sensor_channel_get(env, SENSOR_CHAN_HUMIDITY, &h) ||
	    sensor_channel_get(env, SENSOR_CHAN_PRESS, &p)) {
		return false;
	}
	r->temp_centi_c = t.val1 * 100 + t.val2 / 10000;
	r->humidity_centi = h.val1 * 100 + h.val2 / 10000;
	/* Zephyr reports pressure in kPa. */
	r->pressure_pa = p.val1 * 1000 + p.val2 / 1000;
	return true;
}

static bool read_accel(struct app_reading *r)
{
	struct sensor_value xyz[3];

	if (sensor_sample_fetch(accel) || sensor_channel_get(accel, SENSOR_CHAN_ACCEL_XYZ, xyz)) {
		return false;
	}
	for (int i = 0; i < 3; i++) {
		/* m/s^2 -> milli-g */
		r->accel_milli_g[i] = (int32_t)(sensor_value_to_micro(&xyz[i]) / 9807);
	}
	return true;
}

static void sample_work_fn(struct k_work *work)
{
	struct app_reading r = {0};

	ARG_UNUSED(work);

	r.env_valid = read_env(&r);
	r.accel_valid = read_accel(&r);

	k_mutex_lock(&latest_lock, K_FOREVER);
	r.seq = latest.seq + 1;
	latest = r;
	k_mutex_unlock(&latest_lock);

	if (r.env_valid) {
		LOG_INF("env: t=%d.%02d C rh=%u.%02u %% p=%u Pa", r.temp_centi_c / 100,
			abs(r.temp_centi_c % 100), r.humidity_centi / 100, r.humidity_centi % 100,
			r.pressure_pa);
	} else {
		LOG_WRN("env: read failed");
	}
	if (r.accel_valid) {
		LOG_INF("accel: x=%d y=%d z=%d mg", r.accel_milli_g[0], r.accel_milli_g[1],
			r.accel_milli_g[2]);
	} else {
		LOG_WRN("accel: read failed");
	}

	if (IS_ENABLED(CONFIG_APP_BLE)) {
		ble_publish(&r);
	}
	k_work_schedule(&sample_work, K_MSEC(app_settings_interval_ms()));
}

int sensors_init(void)
{
	int missing = 0;

	if (!device_is_ready(env)) {
		LOG_ERR("%s not ready", env->name);
		missing++;
	}
	if (!device_is_ready(accel)) {
		LOG_ERR("%s not ready", accel->name);
		missing++;
	}
	k_work_schedule(&sample_work, K_NO_WAIT);
	return missing ? -ENODEV : 0;
}

void sensors_get_latest(struct app_reading *out)
{
	k_mutex_lock(&latest_lock, K_FOREVER);
	*out = latest;
	k_mutex_unlock(&latest_lock);
}
