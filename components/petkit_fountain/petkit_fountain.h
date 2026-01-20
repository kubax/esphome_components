#pragma once

#include "esphome.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble/ble_uuid.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/button/button.h"

#include <esp_gattc_api.h>
#include <deque>
#include <vector>
#include <string>
#include <cstring>

namespace esphome {
namespace petkit_fountain {

class PetkitFountain : public PollingComponent, public ble_client::BLEClientNode {
 public:
  PetkitFountain(const std::string &service_uuid, const std::string &notify_uuid, const std::string &write_uuid)
      : PollingComponent(60000),
        service_uuid_(esp32_ble::ESPBTUUID::from_raw(service_uuid)),
        notify_uuid_(esp32_ble::ESPBTUUID::from_raw(notify_uuid)),
        write_uuid_(esp32_ble::ESPBTUUID::from_raw(write_uuid)) {}

  // ---------- Sensor setters ----------
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
  void set_light_switch_state_sensor(sensor::Sensor *s) { light_switch_state_ = s; }
  void set_light_brightness_state_sensor(sensor::Sensor *s) { light_brightness_state_ = s; }
  void set_light_schedule_start_min_sensor(sensor::Sensor *s) { light_schedule_start_min_ = s; }
  void set_light_schedule_end_min_sensor(sensor::Sensor *s) { light_schedule_end_min_ = s; }
  void set_dnd_switch_state_sensor(sensor::Sensor *s) { dnd_switch_state_ = s; }
  void set_dnd_start_min_sensor(sensor::Sensor *s) { dnd_start_min_ = s; }
  void set_dnd_end_min_sensor(sensor::Sensor *s) { dnd_end_min_ = s; }
  void set_battery_sensor(sensor::Sensor *s) { battery_ = s; }

  // ---------- Switch/Number/Select/Button setters ----------
  void set_light_switch(switch_::Switch *sw) {
    light_switch_ = sw;
    light_switch_->add_on_state_callback([this](bool st) { this->on_light_switch_(st); });
  }
  void set_dnd_switch(switch_::Switch *sw) {
    dnd_switch_ = sw;
    dnd_switch_->add_on_state_callback([this](bool st) { this->on_dnd_switch_(st); });
  }
  void set_power_switch(switch_::Switch *sw) {
    power_switch_ = sw;
    power_switch_->add_on_state_callback([this](bool st) { this->on_power_switch_(st); });
  }

  void set_light_brightness_number(number::Number *n) {
    light_brightness_num_ = n;
    light_brightness_num_->add_on_state_callback([this](float v) { this->on_light_brightness_(v); });
  }
  void set_light_schedule_start_number(number::Number *n) {
    light_start_num_ = n;
    light_start_num_->add_on_state_callback([this](float v) { this->on_light_start_(v); });
  }
  void set_light_schedule_end_number(number::Number *n) {
    light_end_num_ = n;
    light_end_num_->add_on_state_callback([this](float v) { this->on_light_end_(v); });
  }
  void set_dnd_start_number(number::Number *n) {
    dnd_start_num_ = n;
    dnd_start_num_->add_on_state_callback([this](float v) { this->on_dnd_start_(v); });
  }
  void set_dnd_end_number(number::Number *n) {
    dnd_end_num_ = n;
    dnd_end_num_->add_on_state_callback([this](float v) { this->on_dnd_end_(v); });
  }

  void set_mode_select(select::Select *s) {
    mode_select_ = s;
    mode_select_->add_on_state_callback([this](const std::string &opt, size_t) {
      this->on_mode_select_(opt);
    });
  }

  void set_init_session_button(button::Button *b) {
    init_session_btn_ = b;
    init_session_btn_->add_on_press_callback([this]() { this->cmd_init_session_(); });
  }
  void set_sync_button(button::Button *b) {
    sync_btn_ = b;
    sync_btn_->add_on_press_callback([this]() { this->cmd_sync_(); });
  }
  void set_datetime_button(button::Button *b) {
    datetime_btn_ = b;
    datetime_btn_->add_on_press_callback([this]() { this->cmd_set_datetime_(); });
  }
  void set_reset_filter_button(button::Button *b) {
    reset_filter_btn_ = b;
    reset_filter_btn_->add_on_press_callback([this]() { this->cmd_reset_filter_(); });
  }
  void set_refresh_button(button::Button *b) {
    refresh_btn_ = b;
    refresh_btn_->add_on_press_callback([this]() { this->cmd_refresh_(); });
  }

  // ---------- ESPHome lifecycle ----------
  void setup() override {
    ESP_LOGI(TAG, "setup: service=%s notify=%s write=%s",
             service_uuid_.to_string().c_str(), notify_uuid_.to_string().c_str(), write_uuid_.to_string().c_str());
  }

  void update() override {
    // periodic refresh if you want:
    // cmd_refresh_();
  }

  void loop() override {
    this->process_tx_queue_();
  }

  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override {
    switch (event) {
      case ESP_GATTC_SEARCH_CMPL_EVT: {
        auto *parent = this->parent();
        if (!parent) break;

        auto *nchr = parent->get_characteristic(service_uuid_, notify_uuid_);
        if (!nchr) {
          ESP_LOGW(TAG, "Notify characteristic not found");
          break;
        }
        notify_handle_ = nchr->handle;

        auto *wchr = parent->get_characteristic(service_uuid_, write_uuid_);
        if (!wchr) {
          ESP_LOGW(TAG, "Write characteristic not found");
        } else {
          write_handle_ = wchr->handle;
        }

        ESP_LOGI(TAG, "handles: notify=0x%04x write=0x%04x", notify_handle_, write_handle_);

        // subscribe notify (ESPHome will write CCCD internally; this just registers)
        esp_err_t err = esp_ble_gattc_register_for_notify(gattc_if, parent->get_remote_bda(), notify_handle_);
        if (err != ESP_OK) {
          ESP_LOGW(TAG, "register_for_notify failed: %d", (int) err);
        } else {
          ESP_LOGI(TAG, "Registered for notify");
        }

        // optional first refresh:
        this->cmd_refresh_();
        break;
      }

      case ESP_GATTC_NOTIFY_EVT: {
        if (param->notify.handle == notify_handle_) {
          this->handle_frame_(param->notify.value, param->notify.value_len);
        }
        break;
      }

      case ESP_GATTC_DISCONNECT_EVT:
      case ESP_GATTC_CLOSE_EVT:
        notify_handle_ = 0;
        write_handle_ = 0;
        break;

      default:
        break;
    }
  }

 private:
  static constexpr const char *TAG = "petkit_fountain";

  // UUIDs
  esp32_ble::ESPBTUUID service_uuid_;
  esp32_ble::ESPBTUUID notify_uuid_;
  esp32_ble::ESPBTUUID write_uuid_;

  // handles
  uint16_t notify_handle_{0};
  uint16_t write_handle_{0};

  // sequencing
  uint8_t seq_{0};
  uint32_t last_tx_ms_{0};

  // device/session (optional for cmd73/86)
  bool have_device_id_{false};
  std::vector<uint8_t> device_id_{};
  std::vector<uint8_t> secret_{};

  // queue
  struct PendingCmd {
    uint8_t cmd;
    uint8_t type;
    std::vector<uint8_t> data;
  };
  std::deque<PendingCmd> txq_;

  // Sensors
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
  sensor::Sensor *light_switch_state_{nullptr};
  sensor::Sensor *light_brightness_state_{nullptr};
  sensor::Sensor *light_schedule_start_min_{nullptr};
  sensor::Sensor *light_schedule_end_min_{nullptr};
  sensor::Sensor *dnd_switch_state_{nullptr};
  sensor::Sensor *dnd_start_min_{nullptr};
  sensor::Sensor *dnd_end_min_{nullptr};
  sensor::Sensor *battery_{nullptr};

  // UI entities
  switch_::Switch *light_switch_{nullptr};
  switch_::Switch *dnd_switch_{nullptr};
  switch_::Switch *power_switch_{nullptr};

  number::Number *light_brightness_num_{nullptr};
  number::Number *light_start_num_{nullptr};
  number::Number *light_end_num_{nullptr};
  number::Number *dnd_start_num_{nullptr};
  number::Number *dnd_end_num_{nullptr};

  select::Select *mode_select_{nullptr};

  button::Button *init_session_btn_{nullptr};
  button::Button *sync_btn_{nullptr};
  button::Button *datetime_btn_{nullptr};
  button::Button *reset_filter_btn_{nullptr};
  button::Button *refresh_btn_{nullptr};

  // ---- helpers ----
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

  void pub_(sensor::Sensor *s, float v) { if (s) s->publish_state(v); }
  void pub_sw_(switch_::Switch *sw, bool v) { if (sw) sw->publish_state(v); }
  void pub_num_(number::Number *n, float v) { if (n) n->publish_state(v); }
  void pub_sel_(select::Select *s, const std::string &v) { if (s) s->publish_state(v); }

  // ---- command framing (assumed) ----
  std::vector<uint8_t> build_cmd_(uint8_t seq, uint8_t cmd, uint8_t type, const std::vector<uint8_t> &data) {
    std::vector<uint8_t> out;
    out.reserve(8 + data.size());
    out.push_back(0xFA);
    out.push_back(0xFC);
    out.push_back(0xFD);
    out.push_back(cmd);
    out.push_back(type);
    out.push_back(seq);
    out.push_back((uint8_t) data.size());
    out.push_back(0x00);
    out.insert(out.end(), data.begin(), data.end());
    return out;
  }

  void enqueue_(uint8_t cmd, uint8_t type, std::vector<uint8_t> data) {
    txq_.push_back(PendingCmd{cmd, type, std::move(data)});
  }

  void process_tx_queue_() {
    if (txq_.empty()) return;

    auto *parent = this->parent();
    if (!parent) return;
    if (write_handle_ == 0) return;

    // throttle a bit (avoid flooding)
    const uint32_t now = millis();
    if ((now - last_tx_ms_) < 120) return;

    PendingCmd p = std::move(txq_.front());
    txq_.pop_front();

    auto frame = build_cmd_(seq_, p.cmd, p.type, p.data);
    seq_ = (uint8_t) (seq_ + 1);

    esp_err_t err = esp_ble_gattc_write_char(
        parent->get_gattc_if(),
        parent->get_conn_id(),
        write_handle_,
        frame.size(),
        (uint8_t *) frame.data(),
        ESP_GATT_WRITE_TYPE_RSP,   // change to NO_RSP if needed
        ESP_GATT_AUTH_REQ_NONE);

    if (err != ESP_OK) {
      ESP_LOGW(TAG, "write_char failed cmd=%u err=%d", p.cmd, (int) err);
    } else {
      ESP_LOGD(TAG, "TX cmd=%u type=%u len=%u seq=%u", p.cmd, p.type, (unsigned) p.data.size(), (unsigned) (seq_ - 1));
    }

    last_tx_ms_ = now;
  }

  // ---- high-level commands (from your Python) ----
  void cmd_get_battery_() { enqueue_(66, 1, {0x00, 0x00}); }
  void cmd_get_details_() { enqueue_(213, 1, {0x00, 0x00}); }
  void cmd_get_state_() { enqueue_(210, 1, {0x00, 0x00}); }
  void cmd_get_config_() { enqueue_(211, 1, {0x00, 0x00}); }
  void cmd_get_info_() { enqueue_(200, 1, {}); }
  void cmd_get_type_() { enqueue_(201, 1, {}); }

  void cmd_set_mode_(bool on, uint8_t mode) { enqueue_(220, 1, {(uint8_t)(on ? 1 : 0), mode}); }
  void cmd_reset_filter_() { enqueue_(222, 1, {0x00}); }

  std::vector<uint8_t> time_in_bytes_() {
    // not sure if Utils.time_in_bytes uses UTC/local; we'll send unix time BE (4 bytes) + padding.
    // If this doesn't work, share a sample from Python's Utils.time_in_bytes().
    auto now = ::time(nullptr);
    uint32_t t = (uint32_t) now;
    return { (uint8_t)((t >> 24) & 0xFF), (uint8_t)((t >> 16) & 0xFF), (uint8_t)((t >> 8) & 0xFF), (uint8_t)(t & 0xFF) };
  }

  void cmd_set_datetime_() { enqueue_(84, 1, time_in_bytes_()); }

  // CMD73/86 flow: we need device_id from notifications (CMD213 response) to derive secret.
  // We'll implement minimal derivation as per comments in your Python.
  static std::vector<uint8_t> pad8_(const std::vector<uint8_t> &in) {
    std::vector<uint8_t> out = in;
    out.resize(8, 0x00);
    return out;
  }
  static std::vector<uint8_t> reverse_(std::vector<uint8_t> v) {
    std::reverse(v.begin(), v.end());
    return v;
  }
  static std::vector<uint8_t> replace_last_two_if_zero_(std::vector<uint8_t> v) {
    if (v.size() >= 2) {
      if (v[v.size()-1] == 0x00 && v[v.size()-2] == 0x00) {
        v[v.size()-2] = 0x13;
        v[v.size()-1] = 0x37;
      }
    }
    return v;
  }

  void cmd_init_session_() {
    // Ensure we have device_id
    if (!have_device_id_) {
      ESP_LOGW(TAG, "Init Session: device_id unknown -> requesting details (CMD213) first");
      cmd_get_details_();
      return;
    }

    // secret derived like Python:
    auto did8 = pad8_(device_id_);
    auto sec = pad8_(replace_last_two_if_zero_(reverse_(device_id_)));
    secret_ = sec;

    std::vector<uint8_t> data;
    data.reserve(2 + 8 + 8);
    data.push_back(0x00);
    data.push_back(0x00);
    data.insert(data.end(), did8.begin(), did8.end());
    data.insert(data.end(), sec.begin(), sec.end());

    enqueue_(73, 1, data);
    ESP_LOGI(TAG, "Init Session: queued CMD73");
  }

  void cmd_sync_() {
    if (secret_.empty()) {
      ESP_LOGW(TAG, "Sync: secret unknown -> press Init Session first");
      return;
    }
    std::vector<uint8_t> data = {0x00, 0x00};
    data.insert(data.end(), secret_.begin(), secret_.end());
    enqueue_(86, 1, data);
    ESP_LOGI(TAG, "Sync: queued CMD86");
  }

  void cmd_refresh_() {
    cmd_get_state_();
    cmd_get_config_();
    cmd_get_battery_();
  }

  // ---- UI callbacks (write operations) ----
  void on_light_switch_(bool st) { this->apply_config_partial_("light_switch", st ? 1 : 0, -1, -1, -1, -1, -1); }
  void on_dnd_switch_(bool st) { this->apply_config_partial_("dnd_switch", st ? 1 : 0, -1, -1, -1, -1, -1); }

  void on_power_switch_(bool st) {
    // mode stays as last known (or default 1 normal)
    uint8_t m = last_mode_ == 2 ? 2 : 1;
    cmd_set_mode_(st, m);
  }

  void on_light_brightness_(float v) {
    int iv = (int) lroundf(v);
    if (iv < 0) iv = 0;
    if (iv > 255) iv = 255;
    this->apply_config_partial_("light_brightness", -1, iv, -1, -1, -1, -1);
  }
  void on_light_start_(float v) { this->apply_config_partial_("light_start", -1, -1, (int) lroundf(v), -1, -1, -1); }
  void on_light_end_(float v) { this->apply_config_partial_("light_end", -1, -1, -1, (int) lroundf(v), -1, -1); }
  void on_dnd_start_(float v) { this->apply_config_partial_("dnd_start", -1, -1, -1, -1, (int) lroundf(v), -1); }
  void on_dnd_end_(float v) { this->apply_config_partial_("dnd_end", -1, -1, -1, -1, -1, (int) lroundf(v)); }

  void on_mode_select_(const std::string &opt) {
    uint8_t m = (opt == "smart") ? 2 : 1;
    // keep current power state if known
    bool on = last_power_ > 0;
    cmd_set_mode_(on, m);
  }

  // We maintain a last-known config image from notifications, and resend with one field changed.
  // That avoids guessing endianness for minutes: we update both bytes BE here based on last raw decode.
  std::vector<uint8_t> last_config_payload_;   // what we decoded from E6 settings part (starting at smart_working_time)
  uint8_t last_power_{0};
  uint8_t last_mode_{1};

  void apply_config_partial_(const char *reason,
                            int light_switch, int light_brightness,
                            int light_start_min, int light_end_min,
                            int dnd_start_min, int dnd_end_min) {
    // Need baseline config. If we don't have it, request it first.
    if (last_config_payload_.empty()) {
      ESP_LOGW(TAG, "Set %s: no baseline config yet -> requesting config (CMD211) first", reason);
      cmd_get_config_();
      return;
    }

    // Baseline layout (expected):
    // [0]=smart_work_on, [1]=smart_sleep_off, [2]=led_switch, [3]=led_brightness,
    // [4..5]=led_start (BE), [6..7]=led_end, [8]=dnd_switch, [9..10]=dnd_start, [11..12]=dnd_end
    // Some devices might have more bytes; we only touch what we know.
    std::vector<uint8_t> cfg = last_config_payload_;

    if (cfg.size() < 13) {
      ESP_LOGW(TAG, "Baseline config too short (%u), cannot set %s", (unsigned) cfg.size(), reason);
      return;
    }

    if (light_switch >= 0) cfg[2] = (uint8_t) light_switch;
    if (light_brightness >= 0) cfg[3] = (uint8_t) light_brightness;

    if (light_start_min >= 0) { cfg[4] = (uint8_t)((light_start_min >> 8) & 0xFF); cfg[5] = (uint8_t)(light_start_min & 0xFF); }
    if (light_end_min >= 0)   { cfg[6] = (uint8_t)((light_end_min >> 8) & 0xFF);   cfg[7] = (uint8_t)(light_end_min & 0xFF); }
    if (dnd_start_min >= 0)   { cfg[9] = (uint8_t)((dnd_start_min >> 8) & 0xFF);   cfg[10] = (uint8_t)(dnd_start_min & 0xFF); }
    if (dnd_end_min >= 0)     { cfg[11] = (uint8_t)((dnd_end_min >> 8) & 0xFF);    cfg[12] = (uint8_t)(dnd_end_min & 0xFF); }

    // CMD221 expects data "as-is" (Python uses set_device_config(data))
    enqueue_(221, 1, cfg);
    ESP_LOGI(TAG, "Queued CMD221 (%s)", reason);
  }

  // ---- Notification decode (your E6) ----
  void handle_frame_(const uint8_t *data, size_t len) {
    if (len < 24) return;
    if (!(data[0] == 0xFA && data[1] == 0xFC && data[2] == 0xFD)) return;

    const uint8_t cmd = data[3];

    // Battery response? (If it matches cmd 66, adjust once you see actual response format)
    // We keep battery decode conservative; you may need to tweak after first capture.

    if (cmd == 0xE6) {
      // core
      last_power_ = data[8];
      last_mode_ = data[9];

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

      // settings (if present)
      if (len >= 38) {
        // publish state-like sensors
        pub_(smart_working_time_, data[24]);
        pub_(smart_sleep_time_, data[25]);
        pub_(light_switch_state_, data[26]);
        pub_(light_brightness_state_, data[27]);
        pub_(light_schedule_start_min_, (float) u16_be_(&data[28]));
        pub_(light_schedule_end_min_, (float) u16_be_(&data[30]));
        pub_(dnd_switch_state_, data[32]);
        pub_(dnd_start_min_, (float) u16_be_(&data[33]));
        pub_(dnd_end_min_, (float) u16_be_(&data[35]));

        // update UI entities (read-back)
        pub_sw_(power_switch_, data[8] != 0);
        pub_sw_(light_switch_, data[26] != 0);
        pub_sw_(dnd_switch_, data[32] != 0);

        if (light_brightness_num_) light_brightness_num_->publish_state((float) data[27]);
        if (light_start_num_) light_start_num_->publish_state((float) u16_be_(&data[28]));
        if (light_end_num_) light_end_num_->publish_state((float) u16_be_(&data[30]));
        if (dnd_start_num_) dnd_start_num_->publish_state((float) u16_be_(&data[33]));
        if (dnd_end_num_) dnd_end_num_->publish_state((float) u16_be_(&data[35]));

        if (mode_select_) {
          pub_sel_(mode_select_, (data[9] == 2) ? "smart" : "normal");
        }

        // store baseline config payload for CMD221:
        // build in the expected 13-byte layout from what we saw in E6:
        // [24]=smart_work, [25]=smart_sleep, [26]=led_switch, [27]=led_brightness,
        // [28..29]=led_start, [30..31]=led_end, [32]=dnd_switch, [33..34]=dnd_start, [35..36]=dnd_end
        last_config_payload_.assign({
          data[24], data[25], data[26], data[27],
          data[28], data[29], data[30], data[31],
          data[32], data[33], data[34], data[35], data[36]
        });
      }
      return;
    }

    // Device details response (CMD213): without the exact format we can't parse.
    // If you capture one CMD213 notification, we can decode device_id properly.
    // For now, we do not auto-derive device_id; Init Session button will first request CMD213.
  }
};

}  // namespace petkit_fountain
}  // namespace esphome
