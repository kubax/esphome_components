#pragma once

#include "esphome.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble/ble_uuid.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/button/button.h"

#include <deque>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <esp_gattc_api.h>

namespace esphome {
namespace petkit_fountain {

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

 protected:
  void control(float value) override;

 private:
  Kind kind_{LIGHT_START};
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

  // sensor setters (keep your existing ones; minimal subset shown)
  void set_power_sensor(sensor::Sensor *s) { power_ = s; }
  void set_mode_sensor(sensor::Sensor *s) { mode_ = s; }
  void set_filter_percent_sensor(sensor::Sensor *s) { filter_percent_ = s; }

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

        cmd_refresh_();
        break;
      }

      case ESP_GATTC_NOTIFY_EVT: {
        if (param->notify.handle == notify_handle_) {
          handle_frame_(param->notify.value, param->notify.value_len);
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

  // UUIDs
  esp32_ble::ESPBTUUID service_uuid_;
  esp32_ble::ESPBTUUID notify_uuid_;
  esp32_ble::ESPBTUUID write_uuid_;

  // handles
  uint16_t notify_handle_{0};
  uint16_t write_handle_{0};

  // sensors (minimal example; extend as you like)
  sensor::Sensor *power_{nullptr};
  sensor::Sensor *mode_{nullptr};
  sensor::Sensor *filter_percent_{nullptr};

  // entities
  PetkitLightSwitch *light_sw_{nullptr};
  PetkitDndSwitch *dnd_sw_{nullptr};
  PetkitPowerSwitch *power_sw_{nullptr};
  PetkitBrightnessNumber *brightness_num_{nullptr};
  std::vector<PetkitTimeNumber *> time_nums_{};
  PetkitModeSelect *mode_sel_{nullptr};
  std::vector<PetkitActionButton *> buttons_{};

  // state cache
  uint8_t last_power_{0};
  uint8_t last_mode_{1};
  std::vector<uint8_t> last_config_payload_;  // 13 bytes baseline for CMD221

  // tx queue
  struct PendingCmd { uint8_t cmd; uint8_t type; std::vector<uint8_t> data; };
  std::deque<PendingCmd> txq_;
  uint8_t seq_{0};
  uint32_t last_tx_ms_{0};

  static uint16_t u16_be_(const uint8_t *p) { return (uint16_t(p[0]) << 8) | uint16_t(p[1]); }
  static uint32_t u32_be_(const uint8_t *p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
  }

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
          ESP_LOGD(TAG, "cmd_refresh called);
  //        cmd_get_state_();
  //        cmd_get_config_();
  //        cmd_get_battery_();
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
    if (len < 8) return;
    if (!(data[0] == 0xFA && data[1] == 0xFC && data[2] == 0xFD)) return;
    const uint8_t cmd = data[3];

    if (cmd == 0xE6 && len >= 24) {
      last_power_ = data[8];
      last_mode_ = data[9];

      if (power_) power_->publish_state(data[8]);
      if (mode_) mode_->publish_state(data[9]);
      if (power_sw_) power_sw_->publish_state(data[8] != 0);
      if (mode_sel_) mode_sel_->publish_state((data[9] == 2) ? "smart" : "normal");

      if (filter_percent_ && len > 18) filter_percent_->publish_state(data[18]);

      // settings part
      if (len >= 38) {
        if (light_sw_) light_sw_->publish_state(data[26] != 0);
        if (dnd_sw_) dnd_sw_->publish_state(data[32] != 0);
        if (brightness_num_) brightness_num_->publish_state((float) data[27]);

        // store baseline config
        last_config_payload_.assign({
          data[24], data[25], data[26], data[27],
          data[28], data[29], data[30], data[31],
          data[32], data[33], data[34], data[35], data[36]
        });

        // update time numbers if present
        uint16_t ls = u16_be_(&data[28]);
        uint16_t le = u16_be_(&data[30]);
        uint16_t ds = u16_be_(&data[33]);
        uint16_t de = u16_be_(&data[35]);
        for (auto *tn : time_nums_) {
          if (!tn) continue;
          // we don't know which instance is which without storing kind in object; control() stores kind.
          // We'll update via current state if set_kind used.
        }
        // If you want automatic publish by kind, we can store pointers per kind; omitted for brevity.
      }
    }
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

}  // namespace petkit_fountain
}  // namespace esphome
