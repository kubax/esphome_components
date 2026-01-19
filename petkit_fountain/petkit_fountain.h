#pragma once

#include "esphome.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble/ble_uuid.h"

#include <esp_gattc_api.h>
#include <cstring>
#include <string>

namespace esphome {
namespace petkit_fountain {

class PetkitFountain : public PollingComponent, public ble_client::BLEClientNode {
 public:
  PetkitFountain(const std::string &service_uuid, const std::string &notify_uuid, const std::string &write_uuid)
      : PollingComponent(60000),
        service_uuid_(esp32_ble::ESPBTUUID::from_raw(service_uuid)),
        notify_uuid_(esp32_ble::ESPBTUUID::from_raw(notify_uuid)),
        write_uuid_(esp32_ble::ESPBTUUID::from_raw(write_uuid)) {}

  // --- setters called from codegen ---
  void set_power_sensor(sensor::Sensor *s) { power_ = s; }
  void set_mode_sensor(sensor::Sensor *s) { mode_ = s; }
  void set_is_night_dnd_sensor(sensor::Sensor *s) { is_night_dnd_ = s; }
  void set_breakdown_warning_sensor(sensor::Sensor *s) { breakdown_warning_ = s; }
  void set_lack_warning_sensor(sensor::Sensor *s) { lack_warning_ = s; }
  void set_filter_warning_sensor(sensor::Sensor *s) { filter_warning_ = s; }

  void set_filter_percent_sensor(sensor::Sensor *s) { filter_percent_ = s; }
  void set_run_status_sensor(sensor::Sensor *s) { run_status_ = s; }

  void set_water_pump_runtime_seconds_sensor(sensor::Sensor *s) { water_pump_runtime_seconds_ = s; }
  void set_today_pump_runtime_seconds_sensor(sensor::Sensor *s) { today_pump_runtime_seconds_ = s; }
  void set_today_purified_water_times_sensor(sensor::Sensor *s) { today_purified_water_times_ = s; }
  void set_today_energy_kwh_sensor(sensor::Sensor *s) { today_energy_kwh_ = s; }

  void set_smart_working_time_sensor(sensor::Sensor *s) { smart_working_time_ = s; }
  void set_smart_sleep_time_sensor(sensor::Sensor *s) { smart_sleep_time_ = s; }
  void set_light_switch_sensor(sensor::Sensor *s) { light_switch_ = s; }
  void set_light_brightness_sensor(sensor::Sensor *s) { light_brightness_ = s; }
  void set_light_schedule_start_min_sensor(sensor::Sensor *s) { light_schedule_start_min_ = s; }
  void set_light_schedule_end_min_sensor(sensor::Sensor *s) { light_schedule_end_min_ = s; }
  void set_dnd_switch_sensor(sensor::Sensor *s) { dnd_switch_ = s; }
  void set_dnd_start_min_sensor(sensor::Sensor *s) { dnd_start_min_ = s; }
  void set_dnd_end_min_sensor(sensor::Sensor *s) { dnd_end_min_ = s; }

  void setup() override {
    ESP_LOGI(TAG, "PetkitFountain setup: service=%s notify=%s write=%s",
             service_uuid_.to_string().c_str(), notify_uuid_.to_string().c_str(), write_uuid_.to_string().c_str());
  }

  void update() override {
    // Optional: Wenn du später ein Request-Frame an write_uuid_ senden willst, hier einbauen.
  }

  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override {
    switch (event) {
      case ESP_GATTC_SEARCH_CMPL_EVT: {
        auto *parent = this->parent();
        if (parent == nullptr) {
          ESP_LOGW(TAG, "No BLE client parent");
          break;
        }

        auto *chr = parent->get_characteristic(service_uuid_, notify_uuid_);
        if (chr == nullptr) {
          ESP_LOGW(TAG, "Notify characteristic not found: %s in service %s",
                   notify_uuid_.to_string().c_str(), service_uuid_.to_string().c_str());
          break;
        }

        notify_handle_ = chr->handle;
        ESP_LOGI(TAG, "Notify characteristic handle=0x%04x", notify_handle_);

        esp_err_t err = esp_ble_gattc_register_for_notify(gattc_if, parent->get_remote_bda(), notify_handle_);
        if (err != ESP_OK) {
          ESP_LOGW(TAG, "register_for_notify failed: %d", (int) err);
          break;
        }

        subscribed_ = true;
        ESP_LOGI(TAG, "Registered for notify (waiting for NOTIFY events)");
        break;
      }

      case ESP_GATTC_NOTIFY_EVT: {
        if (notify_handle_ == 0) break;
        const auto &nt = param->notify;
        if (nt.handle != notify_handle_) break;

        handle_frame_(nt.value, nt.value_len);
        break;
      }

      case ESP_GATTC_DISCONNECT_EVT:
      case ESP_GATTC_CLOSE_EVT: {
        subscribed_ = false;
        notify_handle_ = 0;
        break;
      }

      default:
        break;
    }
  }

 private:
  static constexpr const char *TAG = "petkit_fountain";

  esp32_ble::ESPBTUUID service_uuid_;
  esp32_ble::ESPBTUUID notify_uuid_;
  esp32_ble::ESPBTUUID write_uuid_;

  uint16_t notify_handle_{0};
  bool subscribed_{false};

  // Sensors (optional, may be nullptr if not configured)
  sensor::Sensor *power_{nullptr};
  sensor::Sensor *mode_{nullptr};
  sensor::Sensor *is_night_dnd_{nullptr};
  sensor::Sensor *breakdown_warning_{nullptr};
  sensor::Sensor *lack_warning_{nullptr};
  sensor::Sensor *filter_warning_{nullptr};

  sensor::Sensor *filter_percent_{nullptr};
  sensor::Sensor *run_status_{nullptr};

  sensor::Sensor *water_pump_runtime_seconds_{nullptr};
  sensor::Sensor *today_pump_runtime_seconds_{nullptr};
  sensor::Sensor *today_purified_water_times_{nullptr};
  sensor::Sensor *today_energy_kwh_{nullptr};

  sensor::Sensor *smart_working_time_{nullptr};
  sensor::Sensor *smart_sleep_time_{nullptr};
  sensor::Sensor *light_switch_{nullptr};
  sensor::Sensor *light_brightness_{nullptr};
  sensor::Sensor *light_schedule_start_min_{nullptr};
  sensor::Sensor *light_schedule_end_min_{nullptr};
  sensor::Sensor *dnd_switch_{nullptr};
  sensor::Sensor *dnd_start_min_{nullptr};
  sensor::Sensor *dnd_end_min_{nullptr};

  // --- Decoder helpers (match your original Python) ---
  static uint32_t u32_be_(const uint8_t *p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
  }
  static uint16_t u16_be_(const uint8_t *p) {
    return (uint16_t(p[0]) << 8) | uint16_t(p[1]);
  }

  static float purified_water_(uint32_t runtime_seconds) {
    const float x = 1.5f;
    const float y = 1.8f;
    return (x * (float(runtime_seconds) / 60.0f)) / y;
  }

  static float energy_kwh_(uint32_t runtime_seconds) {
    const float x = 0.75f;
    return (x * float(runtime_seconds)) / 3600000.0f;
  }

  void pub_(sensor::Sensor *s, float v) {
    if (s != nullptr) s->publish_state(v);
  }

  void handle_frame_(const uint8_t *data, size_t len) {
    if (len < 24) return;

    // Header: FA FC FD
    if (!(data[0] == 0xFA && data[1] == 0xFC && data[2] == 0xFD)) return;

    // Command
    if (data[3] != 0xE6) return;

    // Core fields
    pub_(power_, data[8]);
    pub_(mode_, data[9]);
    pub_(is_night_dnd_, data[10]);
    pub_(breakdown_warning_, data[11]);
    pub_(lack_warning_, data[12]);
    pub_(filter_warning_, data[13]);

    const uint32_t pump_runtime = u32_be_(&data[14]);
    pub_(water_pump_runtime_seconds_, (float) pump_runtime);

    pub_(filter_percent_, data[18]);
    pub_(run_status_, data[19]);

    const uint32_t today_runtime = u32_be_(&data[20]);
    pub_(today_pump_runtime_seconds_, (float) today_runtime);

    pub_(today_purified_water_times_, purified_water_(today_runtime));
    pub_(today_energy_kwh_, energy_kwh_(today_runtime));

    // Settings block (only if present)
    if (len > 37) {
      pub_(smart_working_time_, data[24]);
      pub_(smart_sleep_time_, data[25]);
      pub_(light_switch_, data[26]);
      pub_(light_brightness_, data[27]);

      pub_(light_schedule_start_min_, (float) u16_be_(&data[28]));
      pub_(light_schedule_end_min_, (float) u16_be_(&data[30]));

      pub_(dnd_switch_, data[32]);
      pub_(dnd_start_min_, (float) u16_be_(&data[33]));
      pub_(dnd_end_min_, (float) u16_be_(&data[35]));
    }

    ESP_LOGD(TAG, "E6 decoded (len=%u)", (unsigned) len);
  }
};

}  // namespace petkit_fountain
}  // namespace esphome
