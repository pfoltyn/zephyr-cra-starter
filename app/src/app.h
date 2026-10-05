/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_H_
#define APP_H_

#include <stdbool.h>
#include <stdint.h>

struct app_reading {
	bool env_valid;
	int32_t temp_centi_c;    /* 0.01 degC */
	uint32_t humidity_centi; /* 0.01 %RH */
	uint32_t pressure_pa;
	bool accel_valid;
	int32_t accel_milli_g[3];
	uint32_t seq;
};

int sensors_init(void);
void sensors_get_latest(struct app_reading *out);

int app_settings_init(void);
uint32_t app_settings_interval_ms(void);
int app_settings_set_interval_ms(uint32_t ms);

int ble_init(void);
void ble_publish(const struct app_reading *r);

#endif /* APP_H_ */
