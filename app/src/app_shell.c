/*
 * "app" shell commands: status, interval.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include <app_version.h>
#include "app.h"

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	struct app_reading r;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	sensors_get_latest(&r);
	shell_print(sh, "version: %s", APP_VERSION_STRING);
	shell_print(sh, "interval: %u ms", app_settings_interval_ms());
	shell_print(sh, "samples: %u", r.seq);
	if (r.env_valid) {
		shell_print(sh, "temperature: %d.%02d C", r.temp_centi_c / 100, abs(r.temp_centi_c % 100));
		shell_print(sh, "humidity: %u.%02u %%", r.humidity_centi / 100, r.humidity_centi % 100);
		shell_print(sh, "pressure: %u Pa", r.pressure_pa);
	}
	if (r.accel_valid) {
		shell_print(sh, "accel: %d %d %d mg", r.accel_milli_g[0], r.accel_milli_g[1],
			    r.accel_milli_g[2]);
	}
	return 0;
}

static int cmd_interval(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_print(sh, "interval: %u ms", app_settings_interval_ms());
		return 0;
	}
	char *end;
	unsigned long ms = strtoul(argv[1], &end, 10);

	if (*end != '\0' || app_settings_set_interval_ms(ms) != 0) {
		shell_error(sh, "invalid interval (min 100 ms) or storage error");
		return -EINVAL;
	}
	shell_print(sh, "interval set to %lu ms (saved)", ms);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(app_cmds,
	SHELL_CMD(status, NULL, "Show latest readings and configuration", cmd_status),
	SHELL_CMD_ARG(interval, NULL, "Get/set sampling interval in ms (persisted)", cmd_interval,
		      1, 1),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(app, &app_cmds, "Twin Sensor application commands", NULL);
