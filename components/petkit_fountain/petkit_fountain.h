#pragma once

#include "esphome.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble/ble_uuid.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/button/button.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <deque>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <esp_gattc_api.h>

namespace esphome {
namespace petkit_fountain {

struct Petkit213Info {
  bool ok{false};
  uint8_t cmd{0};
  uint8_t type{0};
  uint8_t seq{0};
  uint8_t data_len{0};

  std::vector<uint8_t> device_id_bytes;  // 6 bytes (data[2:8])
  uint64_t device_id_int{0};             // big endian from those 6 bytes
  std::string serial;                    // printable ASCII run
};

static inline bool petkit_is_printable_ascii_(uint8_t b) {
  return (b >= 0x20 && b <= 0x7E);
}

static Petkit213Info petkit_parse_cmd213_(const uint8_t *frame, size_t len) {
  Petkit213Info out;

  // Minimum: 3 header + cmd/type/seq/len/start + end = 9 bytes
  if (len < 9) return out;

  if (frame[0] != 0xFA || frame[1] != 0xFC || frame[2] != 0xFD) return out;
  if (frame[len - 1] != 0xFB) return out;

  out.cmd = frame[3];
  out.type = frame[4];
  out.seq = frame[5];
  out.data_len = frame[6];
  // uint8_t data_start = frame[7]; // expected 0; not strictly required

  // Expected total frame length: 9 + data_len
  if (len != static_cast<size_t>(9 + out.data_len)) return out;
  if (out.cmd != 0xD5) return out;  // 213

  const uint8_t *data = frame + 8;
  const size_t dlen = out.data_len;

  // Python does: device_id_bytes = data[2:8] (6 bytes)
  if (dlen < 8) return out;
  out.device_id_bytes.assign(data + 2, data + 8);

  out.device_id_int = 0;
  for (auto b : out.device_id_bytes) out.device_id_int = (out.device_id_int << 8) | (uint64_t) b;

  // Extract longest printable ASCII run (serial sits at tail for CTW2)
  size_t best_start = 0, best_len = 0;
  size_t cur_start = 0, cur_len = 0;

  for (size_t i = 0; i < dlen; i++) {
    if (petkit_is_printable_ascii_(data[i])) {
      if (cur_len == 0) cur_start = i;
      cur_len++;
      if (cur_len > best_len) {
        best_len = cur_len;
        best_start = cur_start;
      }
    } else {
      cur_len = 0;
    }
  }

  if (best_len >= 6) {
    out.serial.assign(reinterpret_cast<const char *>(data + best_start), best_len);
  }

  out.ok = true;
  return out;
}

class PetkitFountain;

// ---------------- Switch entities ----------------
class PetkitBaseSwitch : public switch_::Switch {
 public:
  void set_parent(PetkitFountain *p) { parent_ = p; }

 protected:
  PetkitFountain *parent_{nullptr};
};

class PetkitLightSwitch : public PetkitBaseSwitch {
 protected:
  void write_state(bool state) override;
};

class PetkitDndSwitch : public PetkitBaseSwitch {
 protected:
  void write_state(bool state) override;
};

class PetkitPowerSwitch : public PetkitBaseSwitch {
 protected:
  void write_state(bool state) override;
};

// ---------------- Number entities ----------------
class PetkitBaseNumber : public number::Number {
 public:
  void set_parent(PetkitFountain *p) { parent_ = p; }

 protected:
  PetkitFountain *parent_{nullptr};
};

class PetkitBrightnessNumber : public PetkitBaseNumber {
 protected:
  void control(float value) override;
};

class PetkitTimeNumber : public PetkitBaseNumber {
 public:
  enum Kind { LIGHT_START, LIGHT_END, DND_START, DND_END };
  void set_kind(Kind k) { kind_ = k; }
  void set_kind(int k) { kind_ = (Kind) k; }
  Kind get_kind() const { return kind_; }

 protected:
  void control(float value) override;

 private:
  Kind kind_{LIGHT_START};
};

class PetkitSmartOnNumber : public PetkitBaseNumber {
 protected:
  void control(float value) override;
};

class PetkitSmartOffNumber : public PetkitBaseNumber {
 protected:
  void control(float value) override;
};

// ---------------- Select entity ----------------
class PetkitModeSelect : public select::Select {
 public:
  void set_parent(PetkitFountain *p) { parent_ = p; }

 protected:
  void control(const std::string &value) override;

 private:
  PetkitFountain *parent_{nullptr};
};

// ---------------- Button entity ----------------
class PetkitActionButton : public button::Button {
 public:
  enum Action { REFRESH, RESET_FILTER, SET_DATETIME, INIT_SESSION, SYNC };
  void set_parent(PetkitFountain *p) { parent_ = p; }
  void set_action(Action a) { action_ = a; }
  void set_action(int a) { action_ = (Action) a; }

 protected:
  void press_action() override;

 private:
  PetkitFountain *parent_{nullptr};
  Action action_{REFRESH};
};

// ---------------- Parent component ----------------
class PetkitFountain : public PollingComponent, public ble_client::BLEClientNode {
 public:
  PetkitFountain(const std::string &service_uuid, const std::string &notify_uuid, const std::string &write_uuid)
      : PollingComponent(60000),
        service_uuid_(esp32_ble::ESPBTUUID::from_raw(service_uuid)),
        notify_uuid_(esp32_ble::ESPBTUUID::from_raw(notify_uuid)),
        write_uuid_(esp32_ble::ESPBTUUID::from_raw(write_uuid)) {}

  // sensor setters
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
  void set_filter_remaining_days_sensor(sensor::Sensor *s) { filter_remaining_days_ = s; }

  // number setters
  void set_light_start_number(number::Number *n) { light_start_num_ = n; }
  void set_light_end_number(number::Number *n) { light_end_num_ = n; }
  void set_dnd_start_number(number::Number *n) { dnd_start_num_ = n; }
  void set_dnd_end_number(number::Number *n) { dnd_end_num_ = n; }
  void set_smart_on_number(PetkitSmartOnNumber *n) { smart_on_num_ = n; n->set_parent(this); }
  void set_smart_off_number(PetkitSmartOffNumber *n) { smart_off_num_ = n; n->set_parent(this); }
  
  // text sensor setter
  void set_serial_text_sensor(text_sensor::TextSensor *t) { serial_text_ = t; }

  // entity attachers
  void set_light_switch(PetkitLightSwitch *s) { light_sw_ = s; s->set_parent(this); }
  void set_dnd_switch(PetkitDndSwitch *s) { dnd_sw_ = s; s->set_parent(this); }
  void set_power_switch(PetkitPowerSwitch *s) { power_sw_ = s; s->set_parent(this); }

  void set_brightness_number(PetkitBrightnessNumber *n) { brightness_num_ = n; n->set_parent(this); }
  void set_time_number(PetkitTimeNumber *n) { time_nums_.push_back(n); n->set_parent(this); }

  void set_mode_select(PetkitModeSelect *s) { mode_sel_ = s; s->set_parent(this); }

  void set_action_button(PetkitActionButton *b) { buttons_.push_back(b); b->set_parent(this); }

  void setup() override {
    ESP_LOGI(TAG, "setup");
  }

  void update() override {
    // optional periodic refresh
    // cmd_refresh_();
  }

  void loop() override {
    process_tx_queue_();
    // Auto-init: send CMD213 once after notify is ready
    if (this->notify_ready_ && !this->auto_213_sent_ && millis() > this->auto_213_at_ms_) {
      // enqueue cmd=213 type=1 seq=... data=[0,0]
      this->enqueue_(213, 1, {0x00, 0x00});
      this->auto_213_sent_ = true;
      ESP_LOGD(TAG, "Auto TX: CMD213 requested");
    }
    if (this->schedule_cmd210_ && (int32_t)(millis() - this->cmd210_at_ms_) >= 0) {
      this->schedule_cmd210_ = false;
  
      // CMD210: type=1, data=[0,0]
      this->enqueue_(210, 1, {0x00, 0x00});
  
      this->cmd210_sent_after_213_ = true;
      ESP_LOGD(TAG, "TX scheduled CMD210 fired");
    }
    if (this->init_stage_ != INIT_NONE && (int32_t)(millis() - this->init_at_ms_) >= 0) {
      switch (this->init_stage_) {
        case INIT_SEND_73: {
          // CMD73 payload: [0,0] + device_id(8) + secret(8)
          std::vector<uint8_t> payload;
          payload.reserve(2 + 8 + 8);
          payload.push_back(0x00);
          payload.push_back(0x00);
          payload.insert(payload.end(), device_id8_.begin(), device_id8_.end());
          payload.insert(payload.end(), secret_.begin(), secret_.end());
    
          this->enqueue_(73, 1, payload);
          ESP_LOGD(TAG, "Init chain: sent CMD73");
          this->init_stage_ = INIT_SEND_86;
          this->init_at_ms_ = millis() + 1500;
          break;
        }
    
        case INIT_SEND_86: {
          // CMD86 payload: [0,0] + secret(8)
          std::vector<uint8_t> payload;
          payload.reserve(2 + 8);
          payload.push_back(0x00);
          payload.push_back(0x00);
          payload.insert(payload.end(), secret_.begin(), secret_.end());
    
          this->enqueue_(86, 1, payload);
          ESP_LOGD(TAG, "Init chain: sent CMD86");
          this->init_stage_ = INIT_SEND_84;
          this->init_at_ms_ = millis() + 750;
          break;
        }
    
        case INIT_SEND_84: {
          // CMD84 payload: Utils.time_in_bytes() equivalent: [0, sec>>24, sec>>16, sec>>8, sec, 13]
          // Reference time: 2000-01-01 UTC. If you already implemented time bytes, call that.
          auto t = this->build_time_bytes_();  // <-- implement or reuse existing
          this->enqueue_(84, 1, t);
          ESP_LOGD(TAG, "Init chain: sent CMD84");
          this->init_stage_ = INIT_SEND_210;
          this->init_at_ms_ = millis() + 750;
          break;
        }
    
        case INIT_SEND_210: {
          this->enqueue_(210, 1, {0x00, 0x00});
          ESP_LOGD(TAG, "Init chain: sent CMD210");
          this->init_stage_ = INIT_SEND_211;
          break;
        }
    
        case INIT_SEND_211: {
          this->enqueue_(211, 1, {0x00, 0x00});
          ESP_LOGD(TAG, "Init chain: sent CMD211");
          this->init_stage_ = INIT_NONE;
          break;
        }
    
        default:
          this->init_stage_ = INIT_NONE;
          break;
      }
    }
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

        esp_err_t err = esp_ble_gattc_register_for_notify(gattc_if, parent->get_remote_bda(), notify_handle_);
        if (err != ESP_OK) ESP_LOGW(TAG, "register_for_notify failed: %d", (int) err);

        // cmd_refresh_();
        break;
      }

      case ESP_GATTC_NOTIFY_EVT: {
        ESP_LOGD(TAG, "NOTIFY handle=0x%04x len=%u", param->notify.handle, param->notify.value_len);
      
        std::string hx;
        hx.reserve(param->notify.value_len * 3);
        static const char *d = "0123456789ABCDEF";
        for (int i = 0; i < param->notify.value_len; i++) {
          uint8_t b = param->notify.value[i];
          hx.push_back(d[(b >> 4) & 0xF]);
          hx.push_back(d[b & 0xF]);
          if (i + 1 < param->notify.value_len) hx.push_back(' ');
        }
        ESP_LOGD(TAG, "RX raw: %s", hx.c_str());
      
        // dann erst dein handle_frame_() aufrufen
        handle_frame_(param->notify.value, param->notify.value_len);
        break;
      }

                                
      case ESP_GATTC_WRITE_DESCR_EVT: {
        // Existing log bleibt; dann:
        this->notify_ready_ = true;
        this->auto_213_sent_ = false;
        this->auto_213_at_ms_ = millis() + 1500;  // 1.5s Delay, entspricht "manuell später drücken"
        ESP_LOGD(TAG, "Notify ready; scheduling auto CMD213 in 1500ms");
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

  // ---------- called by entities ----------
  void set_light_enabled(bool on) {
  apply_config_partial_("light",
      -1, -1,                 // smart_work, smart_sleep
      on ? 1 : 0, -1,          // light_switch, light_brightness
      -1,                      // dnd_switch
      -1, -1,                  // light_start, light_end
      -1, -1                   // dnd_start, dnd_end
    );
  }
  
  void set_dnd_enabled(bool on) {
    apply_config_partial_("dnd",
      -1, -1,                  // smart_work, smart_sleep
      -1, -1,                  // light_switch, light_brightness
      on ? 1 : 0,               // dnd_switch
      -1, -1,                  // light_start, light_end
      -1, -1                   // dnd_start, dnd_end
    );
  }

  void set_power(bool on) {
    uint8_t m = last_mode_ == 2 ? 2 : 1;
    cmd_set_mode_(on, m);
  }

  void set_mode_by_name(const std::string &name) {
    uint8_t m = (name == "smart") ? 2 : 1;
    bool on = last_power_ != 0;
    cmd_set_mode_(on, m);
  }

  void set_brightness(float v) {
    int iv = (int) lroundf(v);
    if (iv < 0) iv = 0;
    if (iv > 255) iv = 255;
    apply_config_partial_("brightness",
      -1, -1,
      -1, iv,
      -1,
      -1, -1,
      -1, -1
    );
  }
  
  void set_time(PetkitTimeNumber::Kind kind, float v) {
    int mins = (int) lroundf(v);
    if (mins < 0) mins = 0;
    if (mins > 1439) mins = 1439;
  
    switch (kind) {
      case PetkitTimeNumber::LIGHT_START:
        apply_config_partial_("light_start",
          -1, -1, -1, -1, -1,
          mins, -1,
          -1, -1
        );
        break;
  
      case PetkitTimeNumber::LIGHT_END:
        apply_config_partial_("light_end",
          -1, -1, -1, -1, -1,
          -1, mins,
          -1, -1
        );
        break;
  
      case PetkitTimeNumber::DND_START:
        apply_config_partial_("dnd_start",
          -1, -1, -1, -1, -1,
          -1, -1,
          mins, -1
        );
        break;
  
      case PetkitTimeNumber::DND_END:
        apply_config_partial_("dnd_end",
          -1, -1, -1, -1, -1,
          -1, -1,
          -1, mins
        );
        break;
    }
  }

  void do_action(PetkitActionButton::Action a) {
    switch (a) {
      case PetkitActionButton::REFRESH: cmd_refresh_(); break;
      case PetkitActionButton::RESET_FILTER: cmd_reset_filter_(); break;
      case PetkitActionButton::SET_DATETIME: cmd_set_datetime_(); break;
      case PetkitActionButton::INIT_SESSION: cmd_init_session_(); break;
      case PetkitActionButton::SYNC: cmd_sync_(); break;
    }
  }

 private:
  static constexpr const char *TAG = "petkit_fountain";

  bool notify_ready_{false};
  bool auto_213_sent_{false};
  uint32_t auto_213_at_ms_{0};

  std::vector<uint8_t> device_id_bytes_;
  uint64_t device_id_int_{0};
  std::string serial_;
  bool have_identifiers_{false};

  bool schedule_cmd210_{false};
  bool cmd210_sent_after_213_{false};
  uint32_t cmd210_at_ms_{0};

  enum InitStage : uint8_t { INIT_NONE, INIT_SEND_73, INIT_SEND_86, INIT_SEND_84, INIT_SEND_210, INIT_SEND_211 };
  InitStage init_stage_{INIT_NONE};
  uint32_t init_at_ms_{0};
  
  std::array<uint8_t, 8> secret_{};
  std::array<uint8_t, 8> device_id8_{};
  bool have_secret_{false};

  bool have_init_{false};   // CMD73 accepted (ACK 0x49)
  bool have_sync_{false};   // CMD86 accepted (ACK 0x56)
  bool have_time_{false};   // CMD84 accepted (ACK 0x54)

  // UUIDs
  esp32_ble::ESPBTUUID service_uuid_;
  esp32_ble::ESPBTUUID notify_uuid_;
  esp32_ble::ESPBTUUID write_uuid_;

  // handles
  uint16_t notify_handle_{0};
  uint16_t write_handle_{0};

  // sensors
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
  sensor::Sensor *filter_remaining_days_{nullptr};

  // switches (aus switch.py)
  switch_::Switch *power_sw_{nullptr};
  switch_::Switch *light_sw_{nullptr};
  switch_::Switch *dnd_sw_{nullptr};
  
  // numbers (aus number.py)
  number::Number *brightness_num_{nullptr};
  number::Number *light_start_num_{nullptr};
  number::Number *light_end_num_{nullptr};
  number::Number *dnd_start_num_{nullptr};
  number::Number *dnd_end_num_{nullptr};
  PetkitSmartOnNumber *smart_on_num_{nullptr};
  PetkitSmartOffNumber *smart_off_num_{nullptr};

  
  // select (aus select.py)
  select::Select *mode_sel_{nullptr};
  
  // text_sensor (aus text_sensor.py)
  text_sensor::TextSensor *serial_text_{nullptr};

  // entities
  std::vector<PetkitTimeNumber *> time_nums_{};
  std::vector<PetkitActionButton *> buttons_{};

  // state cache
  uint8_t last_power_{0};
  uint8_t last_mode_{1};
  std::vector<uint8_t> last_config_payload_;  // 13 bytes baseline for CMD221
  uint8_t last_filter_percent_raw_{0};  // 0..100
  uint8_t last_smart_on_min_{0};        // 0..255 (min)
  uint8_t last_smart_off_min_{0};       // 0..255 (min)


  // tx queue
  struct PendingCmd { uint8_t cmd; uint8_t type; std::vector<uint8_t> data; };
  std::deque<PendingCmd> txq_;
  uint8_t seq_{0};
  uint32_t last_tx_ms_{0};

  static uint16_t u16_be_(const uint8_t *p) { return (uint16_t(p[0]) << 8) | uint16_t(p[1]); }
  static uint32_t u32_be_(const uint8_t *p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
  }

  struct PetkitStateD2 {
    bool ok{false};
    uint8_t seq{0};
  
    uint8_t power{0};
    uint8_t mode{0};
    uint8_t night_dnd{0};
    uint8_t breakdown_warn{0};
    uint8_t lack_warn{0};
    uint8_t filter_warn{0};
  
    uint8_t filter_percent{0};
    uint8_t run_status{0};
  };
  
  static PetkitStateD2 petkit_parse_state_d2_(const uint8_t *frame, size_t len) {
    PetkitStateD2 out;
    if (len < 9) return out;
    if (frame[0] != 0xFA || frame[1] != 0xFC || frame[2] != 0xFD) return out;
    if (frame[len - 1] != 0xFB) return out;
  
    const uint8_t cmd = frame[3];
    const uint8_t type = frame[4];
    const uint8_t seq = frame[5];
    const uint8_t dlen = frame[6];
  
    if (cmd != 0xD2) return out;
    if (type != 0x02) return out;
    if (len != (size_t)(9 + dlen)) return out;
  
    const uint8_t *d = frame + 8;
    const size_t dl = dlen;
  
    // Need at least up to filter_percent/run_status indices we use
    if (dl < 12) return out;
  
    out.seq = seq;
    out.power = d[0];
    out.mode = d[1];
    out.night_dnd = d[2];
    out.breakdown_warn = d[3];
    out.lack_warn = d[4];
    out.filter_warn = d[5];
  
    // These offsets match your observed payload (0x64 at position 10)
    out.filter_percent = d[10];
    out.run_status = d[11];
  
    out.ok = true;
    return out;
  }

  struct PetkitConfigD3 {
    bool ok{false};
    uint8_t seq{0};
  
    uint8_t smart_on{0};
    uint8_t smart_off{0};
  
    uint8_t light_sw{0};
    uint8_t brightness{0};
    uint16_t light_start{0};
    uint16_t light_end{0};
  
    uint8_t dnd_sw{0};
    uint16_t dnd_start{0};
    uint16_t dnd_end{0};
  };
  
  static PetkitConfigD3 petkit_parse_config_d3_(const uint8_t *frame, size_t len) {
    PetkitConfigD3 out;
    if (len < 9) return out;
    if (frame[0] != 0xFA || frame[1] != 0xFC || frame[2] != 0xFD) return out;
    if (frame[len - 1] != 0xFB) return out;
  
    const uint8_t cmd = frame[3];
    const uint8_t type = frame[4];
    const uint8_t seq  = frame[5];
    const uint8_t dlen = frame[6];
  
    if (cmd != 0xD3) return out;
    if (type != 0x02) return out;
    if (len != (size_t)(9 + dlen)) return out;
  
    // Expected config payload length = 13 bytes
    if (dlen != 13) return out;
  
    const uint8_t *d = frame + 8;
  
    out.seq = seq;
  
    // Layout (13 bytes):
    // 0 smart_on, 1 smart_off, 2 light_sw, 3 brightness,
    // 4..5 light_start, 6..7 light_end,
    // 8 dnd_sw, 9..10 dnd_start, 11..12 dnd_end
    out.smart_on    = d[0];
    out.smart_off   = d[1];
    out.light_sw    = d[2];
    out.brightness  = d[3];
    out.light_start = u16_be_(d + 4);
    out.light_end   = u16_be_(d + 6);
    out.dnd_sw      = d[8];
    out.dnd_start   = u16_be_(d + 9);
    out.dnd_end     = u16_be_(d + 11);
  
    out.ok = true;
    return out;
  }

  void set_smart_on_(uint8_t v) {
    apply_config_partial_("smart_on",
      v, -1,      // smart_on, smart_off
      -1, -1,     // light_sw, brightness
      -1,         // dnd_sw
      -1, -1,     // light_start, light_end
      -1, -1      // dnd_start, dnd_end
    );
  }
  
  void set_smart_off_(uint8_t v) {
    apply_config_partial_("smart_off",
      -1, v,
      -1, -1,
      -1,
      -1, -1,
      -1, -1
    );
  }

  struct PetkitAck {
    bool ok{false};
    uint8_t cmd{0};
    uint8_t seq{0};
    uint8_t value{0};  // usually 1 = ok
  };
  
  static PetkitAck petkit_parse_ack_(const uint8_t *frame, size_t len) {
    PetkitAck out;
    if (len < 9) return out;
    if (frame[0] != 0xFA || frame[1] != 0xFC || frame[2] != 0xFD) return out;
    if (frame[len - 1] != 0xFB) return out;
  
    const uint8_t cmd = frame[3];
    const uint8_t type = frame[4];
    const uint8_t seq = frame[5];
    const uint8_t dlen = frame[6];
  
    if (type != 0x02) return out;
    if (len != (size_t)(9 + dlen)) return out;
    if (dlen < 1) return out;
  
    out.ok = true;
    out.cmd = cmd;
    out.seq = seq;
    out.value = frame[8];
    return out;
  }

  std::vector<uint8_t> build_time_bytes_() {
    // Build payload like Python Utils.time_in_bytes():
    // [0, sec>>24, sec>>16, sec>>8, sec, 13]
    // sec = seconds since 2000-01-01 00:00:00 UTC
  
    std::vector<uint8_t> out;
    out.reserve(6);
  
    // If system time isn't set, we still return something deterministic to avoid empty writes.
    // You can also skip CMD84 if time is invalid.
    time_t now_unix = time(nullptr);
  
    // 2000-01-01 00:00:00 UTC in Unix epoch seconds
    const int64_t unix_to_2000 = 946684800LL;
  
    int64_t sec2000 = (int64_t) now_unix - unix_to_2000;
    if (sec2000 < 0) sec2000 = 0;  // clamp if clock not set
  
    uint32_t s = (uint32_t) sec2000;
  
    out.push_back(0x00);
    out.push_back((s >> 24) & 0xFF);
    out.push_back((s >> 16) & 0xFF);
    out.push_back((s >> 8) & 0xFF);
    out.push_back((s) & 0xFF);
    out.push_back(0x0D);  // 13
  
    return out;
  }


  void compute_secret_from_device_id_() {
    // device_id_bytes_ is 6 bytes (may be all 0)
    // Build device_id8_ = pad left with zeros to 8
    device_id8_.fill(0);
    int di_len = (int) this->device_id_bytes_.size();
    for (int i = 0; i < di_len && i < 8; i++) {
      device_id8_[8 - di_len + i] = this->device_id_bytes_[i];
    }
  
    // secret = reverse(device_id_bytes) + replace last two if zero + pad left to 8
    std::vector<uint8_t> tmp = this->device_id_bytes_;
    std::reverse(tmp.begin(), tmp.end());
    if (tmp.size() >= 2 && tmp[tmp.size() - 1] == 0 && tmp[tmp.size() - 2] == 0) {
      tmp[tmp.size() - 2] = 13;
      tmp[tmp.size() - 1] = 37;
    }
  
    secret_.fill(0);
    int s_len = (int) tmp.size();
    for (int i = 0; i < s_len && i < 8; i++) {
      secret_[8 - s_len + i] = tmp[i];
    }
  
    have_secret_ = true;
  
    ESP_LOGI(TAG, "Secret computed. device_id8=%02X%02X%02X%02X%02X%02X%02X%02X secret=%02X%02X%02X%02X%02X%02X%02X%02X",
             device_id8_[0], device_id8_[1], device_id8_[2], device_id8_[3], device_id8_[4], device_id8_[5], device_id8_[6], device_id8_[7],
             secret_[0], secret_[1], secret_[2], secret_[3], secret_[4], secret_[5], secret_[6], secret_[7]);
  }


  std::vector<uint8_t> build_cmd_(uint8_t seq, uint8_t cmd, uint8_t type,
                                 const std::vector<uint8_t> &data) {
    std::vector<uint8_t> out;
    out.reserve(3 + 1 + 1 + 1 + 1 + 1 + data.size() + 1);
  
    out.push_back(0xFA);
    out.push_back(0xFC);
    out.push_back(0xFD);
    out.push_back(cmd);
    out.push_back(type);
    out.push_back(seq);
    out.push_back((uint8_t) data.size());
    out.push_back(0x00);              // data_start
    out.insert(out.end(), data.begin(), data.end());
    out.push_back(0xFB);              // end_byte
  
    return out;
  }

  void enqueue_(uint8_t cmd, uint8_t type, std::vector<uint8_t> data) {
    txq_.push_back(PendingCmd{cmd, type, std::move(data)});
  }

  void publish_filter_remaining_days_() {
    if (!filter_remaining_days_) return;
  
    // clamp percent 0..100
    uint8_t fp = last_filter_percent_raw_;
    if (fp > 100) fp = 100;
  
    const float p = float(fp) / 100.0f;
    float days = 0.0f;
  
    // Base lifespan: 30 days
    if (last_mode_ == 2) {
      // Smart mode: on/off in minutes (0..1439)
      const float on  = float(last_smart_on_min_);
      const float off = float(last_smart_off_min_);
  
      if (on <= 0.0f) {
        // fallback: behave like normal mode
        days = std::ceil(p * 30.0f);
      } else {
        days = std::ceil(((p * 30.0f) * (on + off)) / on);
      }
    } else {
      // Normal mode
      days = std::ceil(p * 30.0f);
    }
  
    // Safety against NaN/Inf
    if (!std::isfinite(days) || days < 0.0f) days = 0.0f;
  
    filter_remaining_days_->publish_state(days);
  }


  void process_tx_queue_() {
    if (txq_.empty()) return;
    auto *parent = this->parent();
    if (!parent || write_handle_ == 0) return;

    const uint32_t now = millis();
    if ((now - last_tx_ms_) < 120) return;

    PendingCmd p = std::move(txq_.front());
    txq_.pop_front();

    auto frame = build_cmd_(seq_, p.cmd, p.type, p.data);
    uint8_t used_seq = seq_;
    seq_ = uint8_t(seq_ + 1);

    esp_err_t err = esp_ble_gattc_write_char(
        parent->get_gattc_if(), parent->get_conn_id(), write_handle_,
        frame.size(), (uint8_t *) frame.data(),
        ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);

    if (err != ESP_OK) {
      ESP_LOGW(TAG, "write_char failed cmd=%u err=%d", (unsigned) p.cmd, (int) err);
    } else {
      ESP_LOGD(TAG, "TX cmd=%u type=%u seq=%u len=%u", (unsigned) p.cmd, (unsigned) p.type, (unsigned) used_seq,
               (unsigned) p.data.size());
    }
    last_tx_ms_ = now;
  }

  // commands
  void cmd_get_state_() { enqueue_(210, 1, {0x00, 0x00}); }
  void cmd_get_config_() { enqueue_(211, 1, {0x00, 0x00}); }
  void cmd_get_battery_() { enqueue_(66, 1, {0x00, 0x00}); }
  void cmd_refresh_() {
          cmd_get_state_();
          // cmd_get_config_();
          // cmd_get_battery_();
  }

  void cmd_set_mode_(bool on, uint8_t mode) { enqueue_(220, 1, {(uint8_t) (on ? 1 : 0), mode}); }
  void cmd_reset_filter_() { enqueue_(222, 1, {0x00}); }

  std::vector<uint8_t> time_in_bytes_() {
    uint32_t t = (uint32_t) ::time(nullptr);
    return { (uint8_t)((t >> 24) & 0xFF), (uint8_t)((t >> 16) & 0xFF), (uint8_t)((t >> 8) & 0xFF), (uint8_t)(t & 0xFF) };
  }
  void cmd_set_datetime_() { enqueue_(84, 1, time_in_bytes_()); }

  // placeholders (need CMD213 parsing to work)
  void cmd_init_session_() { ESP_LOGW(TAG, "CMD73 not implemented yet (needs CMD213 parsing)"); }
  void cmd_sync_() { ESP_LOGW(TAG, "CMD86 not implemented yet (needs CMD73 secret)"); }

  void apply_config_partial_(const char *reason,
                            int smart_work, int smart_sleep,
                            int light_switch, int light_brightness,
                            int dnd_switch,
                            int light_start_min, int light_end_min,
                            int dnd_start_min, int dnd_end_min) {
    if (last_config_payload_.empty()) {
      ESP_LOGW(TAG, "Set %s: no baseline config yet -> requesting config", reason);
      cmd_get_config_();
      return;
    }
    if (last_config_payload_.size() < 13) {
      ESP_LOGW(TAG, "Set %s: baseline config too short (%u)", reason, (unsigned) last_config_payload_.size());
      return;
    }

    std::vector<uint8_t> cfg = last_config_payload_;

    if (smart_work >= 0) cfg[0] = (uint8_t) smart_work;
    if (smart_sleep >= 0) cfg[1] = (uint8_t) smart_sleep;
    if (light_switch >= 0) cfg[2] = (uint8_t) light_switch;
    if (light_brightness >= 0) cfg[3] = (uint8_t) light_brightness;
    if (light_start_min >= 0) { cfg[4] = (uint8_t)((light_start_min >> 8) & 0xFF); cfg[5] = (uint8_t)(light_start_min & 0xFF); }
    if (light_end_min >= 0)   { cfg[6] = (uint8_t)((light_end_min >> 8) & 0xFF);   cfg[7] = (uint8_t)(light_end_min & 0xFF); }
    if (dnd_switch >= 0) cfg[8] = (uint8_t) dnd_switch;
    if (dnd_start_min >= 0)   { cfg[9] = (uint8_t)((dnd_start_min >> 8) & 0xFF);   cfg[10] = (uint8_t)(dnd_start_min & 0xFF); }
    if (dnd_end_min >= 0)     { cfg[11] = (uint8_t)((dnd_end_min >> 8) & 0xFF);    cfg[12] = (uint8_t)(dnd_end_min & 0xFF); }

    enqueue_(221, 1, cfg);
    ESP_LOGI(TAG, "Queued CMD221 (%s)", reason);
  }

  void handle_frame_(const uint8_t *data, size_t len) {
    // Petkit frame min length: 9 bytes (FA FC FD + cmd/type/seq/len/start + FB)
    if (len < 9) return;
    if (!(data[0] == 0xFA && data[1] == 0xFC && data[2] == 0xFD)) return;
    if (data[len - 1] != 0xFB) return;
  
    const uint8_t cmd = data[3];
  
    // ----- CMD213: device identifiers -----
    if (cmd == 0xD5) {  // 213
      auto info = petkit_parse_cmd213_(data, len);
      if (info.ok) {
        this->device_id_bytes_ = info.device_id_bytes;
        this->device_id_int_ = info.device_id_int;
        this->serial_ = info.serial;
        if (this->serial_text_ && !this->serial_.empty()) {
          this->serial_text_->publish_state(this->serial_);
        }
        this->have_identifiers_ = true;
  
        ESP_LOGI(TAG, "CMD213 parsed: device_id=%llu serial=%s",
                 (unsigned long long) this->device_id_int_,
                 this->serial_.c_str());
        // // schedule CMD210 1.5s after identifiers
        // this->schedule_cmd210_ = true;
        // this->cmd210_at_ms_ = millis() + 1500;
        // this->cmd210_sent_after_213_ = false;
        // ESP_LOGD(TAG, "Scheduling CMD210 in 1500ms after CMD213");
        this->compute_secret_from_device_id_();
        this->init_stage_ = INIT_SEND_73;
        this->init_at_ms_ = millis() + 1500;
        ESP_LOGD(TAG, "Starting init chain: CMD73 in 1500ms");

      } else {
        ESP_LOGW(TAG, "CMD213 received but parse failed (len=%u)", (unsigned) len);
      }
      return;
    }
  
    // ----- CMD230 or other notify frames can be handled here too -----
  
    // ----- CMD0xE6 (your existing state/config frame) -----
    // if (cmd == 0xE6 && len >= 24) {
    //   last_power_ = data[8];
    //   last_mode_ = data[9];
    //
    //   if (power_) power_->publish_state(data[8]);
    //   if (mode_) mode_->publish_state(data[9]);
    //   if (power_sw_) power_sw_->publish_state(data[8] != 0);
    //   if (mode_sel_) mode_sel_->publish_state((data[9] == 2) ? "smart" : "normal");
    //
    //   if (filter_percent_ && len > 18) filter_percent_->publish_state(data[18]);
    //
    //   // settings part
    //   if (len >= 38) {
    //     if (light_sw_) light_sw_->publish_state(data[26] != 0);
    //     if (dnd_sw_) dnd_sw_->publish_state(data[32] != 0);
    //     if (brightness_num_) brightness_num_->publish_state((float) data[27]);
    //
    //     last_config_payload_.assign({
    //       data[24], data[25], data[26], data[27],
    //       data[28], data[29], data[30], data[31],
    //       data[32], data[33], data[34], data[35], data[36]
    //     });
    //
    //     uint16_t ls = u16_be_(&data[28]);
    //     uint16_t le = u16_be_(&data[30]);
    //     uint16_t ds = u16_be_(&data[33]);
    //     uint16_t de = u16_be_(&data[35]);
    //     (void) ls; (void) le; (void) ds; (void) de;
    //   }
    //   return;
    // }
    if (cmd == 0xE6 && len >= 38) {
      const uint8_t power = data[8];
      const uint8_t mode  = data[9];
    
      const uint8_t night_dnd = data[10];
      const uint8_t breakdown_warn = data[11];
      const uint8_t lack_warn = data[12];
      const uint8_t filter_warn = data[13];
    
      const uint32_t pump_runtime =
          ((uint32_t)data[14] << 24) | ((uint32_t)data[15] << 16) | ((uint32_t)data[16] << 8) | (uint32_t)data[17];
    
      const uint8_t filter_percent = data[18];
      const uint8_t run_status = data[19];
    
      const uint32_t today_runtime =
          ((uint32_t)data[20] << 24) | ((uint32_t)data[21] << 16) | ((uint32_t)data[22] << 8) | (uint32_t)data[23];

      last_mode_ = mode;
      last_filter_percent_raw_ = filter_percent;

    
      // --- publish base state ---
      if (power_) power_->publish_state(power);
      if (mode_) mode_->publish_state(mode);
    
      if (power_sw_) power_sw_->publish_state(power != 0);
      if (mode_sel_) mode_sel_->publish_state((mode == 2) ? "smart" : "normal");
    
      if (filter_percent_) filter_percent_->publish_state(filter_percent);
    
      // --- publish warnings / flags (if you have the sensors) ---
      if (is_night_dnd_) is_night_dnd_->publish_state(night_dnd);
      if (breakdown_warning_) breakdown_warning_->publish_state(breakdown_warn);
      if (lack_warning_) lack_warning_->publish_state(lack_warn);
      if (filter_warning_) filter_warning_->publish_state(filter_warn);
    
      if (run_status_) run_status_->publish_state(run_status);
    
      // --- runtimes ---
      if (water_pump_runtime_seconds_) water_pump_runtime_seconds_->publish_state((float) pump_runtime);
      if (today_pump_runtime_seconds_) today_pump_runtime_seconds_->publish_state((float) today_runtime);
    
      // --- settings block ---
      const uint8_t smart_on  = data[24];
      const uint8_t smart_off = data[25];
      const uint8_t light_sw  = data[26];
      const uint8_t brightness = data[27];
    
      const uint16_t light_start = ((uint16_t)data[28] << 8) | data[29];
      const uint16_t light_end   = ((uint16_t)data[30] << 8) | data[31];
    
      const uint8_t dnd_sw = data[32];
      const uint16_t dnd_start = ((uint16_t)data[33] << 8) | data[34];
      const uint16_t dnd_end   = ((uint16_t)data[35] << 8) | data[36];

      last_smart_on_min_  = smart_on;
      last_smart_off_min_ = smart_off;
      publish_filter_remaining_days_();

    
      if (smart_working_time_) smart_working_time_->publish_state(smart_on);
      if (smart_sleep_time_) smart_sleep_time_->publish_state(smart_off);
    
      // Diese beiden sind bei dir aktuell als "Sensor" implementiert – daher publish_state() ok.
      // Wenn du später echte ESPHome switches/numbers draus machst, musst du die jeweiligen publish APIs nutzen.
      if (light_switch_) light_switch_->publish_state(light_sw);
      if (light_brightness_) light_brightness_->publish_state(brightness);
    
      if (light_schedule_start_min_) light_schedule_start_min_->publish_state(light_start);
      if (light_schedule_end_min_)   light_schedule_end_min_->publish_state(light_end);
    
      if (dnd_switch_) dnd_switch_->publish_state(dnd_sw);
      if (dnd_start_min_) dnd_start_min_->publish_state(dnd_start);
      if (dnd_end_min_)   dnd_end_min_->publish_state(dnd_end);
    
      ESP_LOGD(TAG, "entities: pwr_sw=%p light_sw=%p dnd_sw=%p mode_sel=%p bright=%p",
         (void*) power_sw_, (void*) light_sw_, (void*) dnd_sw_, (void*) mode_sel_, (void*) brightness_num_);

      return;
    }


    // Generic ACKs (format: FA FC FD <cmd> 02 <seq> 01 00 <status> FB)
    if (cmd == 0x49 || cmd == 0x56 || cmd == 0x54 || cmd == 0xDC || cmd == 0xDD) {
      auto ack = petkit_parse_ack_(data, len);
      if (ack.ok) {
        if (ack.cmd == 0x49) {
          ESP_LOGI(TAG, "CMD73 ACK: seq=%u status=%u", ack.seq, ack.value);
          have_init_ = (ack.value == 1);
        } else if (ack.cmd == 0x56) {
          ESP_LOGI(TAG, "CMD86 ACK: seq=%u status=%u", ack.seq, ack.value);
          have_sync_ = (ack.value == 1);
        } else if (ack.cmd == 0x54) {
          ESP_LOGI(TAG, "CMD84 ACK: seq=%u status=%u", ack.seq, ack.value);
          have_time_ = (ack.value == 1);
        } else if (ack.cmd == 0xDC) {
          ESP_LOGI(TAG, "CMD220 ACK: seq=%u status=%u", ack.seq, ack.value);
        } else if (ack.cmd == 0xDD) {
          ESP_LOGI(TAG, "CMD221 ACK: seq=%u status=%u", ack.seq, ack.value);
        }
      } else {
        ESP_LOGW(TAG, "ACK parse failed cmd=0x%02X len=%u", cmd, (unsigned) len);
      }
      return;
    }



    if (cmd == 0xD2) {  // response to CMD210
      auto st = petkit_parse_state_d2_(data, len);
      if (st.ok) {
        ESP_LOGI(TAG, "CMD210->D2: power=%u mode=%u dnd=%u warn(break=%u lack=%u filter=%u) filter=%u run=%u",
                 st.power, st.mode, st.night_dnd, st.breakdown_warn, st.lack_warn, st.filter_warn,
                 st.filter_percent, st.run_status);
        last_mode_ = st.mode;
        last_filter_percent_raw_ = st.filter_percent;
        publish_filter_remaining_days_();

    
        // publish to sensors if you have them
        if (power_) power_->publish_state(st.power);
        if (mode_) mode_->publish_state(st.mode);
        if (power_sw_) power_sw_->publish_state(st.power != 0);
        if (mode_sel_) mode_sel_->publish_state((st.mode == 2) ? "smart" : "normal");
    
        if (is_night_dnd_) is_night_dnd_->publish_state(st.night_dnd);
        if (breakdown_warning_) breakdown_warning_->publish_state(st.breakdown_warn);
        if (lack_warning_) lack_warning_->publish_state(st.lack_warn);
        if (filter_warning_) filter_warning_->publish_state(st.filter_warn);
    
        if (filter_percent_) filter_percent_->publish_state(st.filter_percent);
        if (run_status_) run_status_->publish_state(st.run_status);
      } else {
        ESP_LOGW(TAG, "CMD0xD2 parse failed (len=%u)", (unsigned) len);
      }
      return;
    }

    if (cmd == 0xD3) {  // response to CMD211 (get_config)
      auto cfg = petkit_parse_config_d3_(data, len);
      if (!cfg.ok) {
        ESP_LOGW(TAG, "CMD0xD3 parse failed (len=%u)", (unsigned) len);
        return;
      }
      last_smart_on_min_  = cfg.smart_on;
      last_smart_off_min_ = cfg.smart_off;
      publish_filter_remaining_days_();

    
      // 1) baseline config setzen (damit CMD221 möglich wird)
      last_config_payload_.assign({
        cfg.smart_on,
        cfg.smart_off,
        cfg.light_sw,
        cfg.brightness,
        (uint8_t)((cfg.light_start >> 8) & 0xFF), (uint8_t)(cfg.light_start & 0xFF),
        (uint8_t)((cfg.light_end   >> 8) & 0xFF), (uint8_t)(cfg.light_end   & 0xFF),
        cfg.dnd_sw,
        (uint8_t)((cfg.dnd_start  >> 8) & 0xFF), (uint8_t)(cfg.dnd_start  & 0xFF),
        (uint8_t)((cfg.dnd_end    >> 8) & 0xFF), (uint8_t)(cfg.dnd_end    & 0xFF),
      });
    
      ESP_LOGD(TAG,
        "CMD211->D3 cfg: smart_on=%u smart_off=%u light=%u bright=%u ls=%u le=%u dnd=%u ds=%u de=%u",
        cfg.smart_on, cfg.smart_off, cfg.light_sw, cfg.brightness,
        cfg.light_start, cfg.light_end, cfg.dnd_sw, cfg.dnd_start, cfg.dnd_end
      );
    
      // 2) Sensoren publishen (falls in YAML vorhanden)
      if (smart_working_time_) smart_working_time_->publish_state(cfg.smart_on);
      if (smart_on_num_)  smart_on_num_->publish_state((float) cfg.smart_on);
      if (smart_off_num_) smart_off_num_->publish_state((float) cfg.smart_off);
      if (smart_sleep_time_)   smart_sleep_time_->publish_state(cfg.smart_off);
    
      if (light_switch_)               light_switch_->publish_state(cfg.light_sw);
      if (light_brightness_)           light_brightness_->publish_state(cfg.brightness);
      if (light_schedule_start_min_)   light_schedule_start_min_->publish_state(cfg.light_start);
      if (light_schedule_end_min_)     light_schedule_end_min_->publish_state(cfg.light_end);
    
      if (dnd_switch_) dnd_switch_->publish_state(cfg.dnd_sw);
      if (dnd_start_min_) dnd_start_min_->publish_state(cfg.dnd_start);
      if (dnd_end_min_)   dnd_end_min_->publish_state(cfg.dnd_end);
    
      // 3) ESPHome Entities publishen (Switch/Number)
      if (light_sw_) light_sw_->publish_state(cfg.light_sw != 0);
      if (dnd_sw_)   dnd_sw_->publish_state(cfg.dnd_sw != 0);
    
      if (brightness_num_) brightness_num_->publish_state((float) cfg.brightness);
    
      // 4) Time Number Entities publishen (die 4 Kind-Varianten)
      for (auto *tn : time_nums_) {
        if (!tn) continue;
        switch (tn->get_kind()) {
          case PetkitTimeNumber::LIGHT_START: tn->publish_state((float) cfg.light_start); break;
          case PetkitTimeNumber::LIGHT_END:   tn->publish_state((float) cfg.light_end); break;
          case PetkitTimeNumber::DND_START:   tn->publish_state((float) cfg.dnd_start); break;
          case PetkitTimeNumber::DND_END:     tn->publish_state((float) cfg.dnd_end); break;
        }
      }
    
      return;
    }


  
    // optional: log unknown cmds
    ESP_LOGD(TAG, "Unhandled cmd=0x%02X len=%u", cmd, (unsigned) len);
  }
};

// ------------- entity implementations -------------
inline void PetkitLightSwitch::write_state(bool state) {
  if (!this->parent_) return;
  this->parent_->set_light_enabled(state);
  this->publish_state(state);
}

inline void PetkitDndSwitch::write_state(bool state) {
  if (!this->parent_) return;
  this->parent_->set_dnd_enabled(state);
  this->publish_state(state);
}

inline void PetkitPowerSwitch::write_state(bool state) {
  if (!this->parent_) return;
  this->parent_->set_power(state);
  this->publish_state(state);
}

inline void PetkitBrightnessNumber::control(float value) {
  if (!this->parent_) return;
  this->parent_->set_brightness(value);
  this->publish_state(value);
}

inline void PetkitTimeNumber::control(float value) {
  if (!this->parent_) return;
  this->parent_->set_time(kind_, value);
  this->publish_state(value);
}

inline void PetkitModeSelect::control(const std::string &value) {
  if (!this->parent_) return;
  this->parent_->set_mode_by_name(value);
  this->publish_state(value);
}

inline void PetkitActionButton::press_action() {
  if (!this->parent_) return;
  this->parent_->do_action(action_);
}

inline void PetkitSmartOnNumber::control(float value) {
  if (!this->parent_) return;
  int v = (int) lroundf(value);
  if (v < 0) v = 0;
  if (v > 255) v = 255;   // 1 Byte
  this->parent_->set_smart_on_((uint8_t) v);
  this->publish_state((float) v);
}

inline void PetkitSmartOffNumber::control(float value) {
  if (!this->parent_) return;
  int v = (int) lroundf(value);
  if (v < 0) v = 0;
  if (v > 255) v = 255;   // 1 Byte
  this->parent_->set_smart_off_((uint8_t) v);
  this->publish_state((float) v);
}


}  // namespace petkit_fountain
}  // namespace esphome
