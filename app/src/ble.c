/*
 * Bluetooth LE: connectable advertising + Environmental Sensing Service with
 * temperature and humidity characteristics (read + notify).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include "app.h"

LOG_MODULE_REGISTER(ble, CONFIG_APP_LOG_LEVEL);

static int16_t ess_temp;    /* 0.01 degC, per GATT Temperature characteristic */
static uint16_t ess_humid;  /* 0.01 %RH, per GATT Humidity characteristic */

static ssize_t read_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(uint16_t));
}

static void ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	LOG_INF("notifications %s", value == BT_GATT_CCC_NOTIFY ? "on" : "off");
}

BT_GATT_SERVICE_DEFINE(ess_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_u16, NULL, &ess_temp),
	BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_u16, NULL, &ess_humid),
	BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ESS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);
	if (err) {
		LOG_WRN("connection failed (0x%02x)", err);
	} else {
		LOG_INF("connected");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	LOG_INF("disconnected (0x%02x)", reason);
}

static void recycled(void)
{
	/* Connection object freed: resume advertising. */
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err && err != -EALREADY) {
		LOG_ERR("advertising restart failed (%d)", err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

int ble_init(void)
{
	int err = bt_enable(NULL);

	if (err) {
		return err;
	}
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		settings_load_subtree("bt");
	}
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		return err;
	}
	LOG_INF("advertising as \"%s\"", CONFIG_BT_DEVICE_NAME);
	return 0;
}

void ble_publish(const struct app_reading *r)
{
	if (!r->env_valid) {
		return;
	}
	ess_temp = (int16_t)r->temp_centi_c;
	ess_humid = (uint16_t)r->humidity_centi;
	bt_gatt_notify(NULL, &ess_svc.attrs[2], &ess_temp, sizeof(ess_temp));
	bt_gatt_notify(NULL, &ess_svc.attrs[5], &ess_humid, sizeof(ess_humid));
}
