# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(twin_sensor_app LANGUAGES C)

target_sources(app PRIVATE
  src/main.c
  src/sensors.c
  src/app_settings.c
  src/app_shell.c
)
target_sources_ifdef(CONFIG_APP_BLE app PRIVATE src/ble.c)
