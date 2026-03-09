# Petkit Fountain BLE (ESPHome External Component)

This repository provides an **ESPHome external component** to connect to **Petkit water fountains** over **Bluetooth Low Energy (BLE)** and expose sensors/controls to Home Assistant via ESPHome.

It is built around the Petkit BLE GATT service/characteristics:

- Service UUID: `0000aaa0-0000-1000-8000-00805f9b34fb`
- Notify (read) UUID: `0000aaa1-0000-1000-8000-00805f9b34fb`
- Write UUID: `0000aaa2-0000-1000-8000-00805f9b34fb`

Tested with ESP32 (ESP-IDF framework). Example target: **Wemos D1 Mini ESP32**.

## Features

- Maintains a BLE connection via ESPHome `ble_client`
- Subscribes to notifications and parses incoming frames
- Implements a minimal **init chain** to start a stable session:
  - CMD73 (session init / handshake)
  - CMD86 (sync; requires secret from CMD73)
  - CMD84 (set device time)
  - CMD210 (read current device state)
  - CMD211 (read current configuration)
- Exposes:
  - **Sensors** (power/mode/filter percent, etc.)
  - **Switches** (power, light, DND)
  - **Select** (mode)
  - **Numbers** (brightness, schedule times, DND times)
  - **Buttons** (refresh/read state, read config, set datetime, reset filter, init session, sync)

Important: Some models report brightness as a small range (e.g. `0..2` = off/low/high). Although the protocol uses a byte, device firmware may only support a few levels.

---

## Installation (ESPHome)

Add this repository as an **external component** in your ESPHome YAML:

```yaml
external_components:
  - source: github://kubax/esphome_components@main
    components: [ petkit_fountain ]
    refresh: 0s   # always pull latest on build
```

---

## Example ESPHome Configuration (ESP-IDF)

```yaml
esphome:
  name: petkit-fountain-ble
  friendly_name: Petkit Fountain BLE (ESP-IDF)

esp32:
  board: wemos_d1_mini32
  framework:
    type: esp-idf

logger:
  level: DEBUG
  logs:
    ble_client: DEBUG
    esp32_ble: DEBUG
    esp32_ble_tracker: DEBUG

api:
  reboot_timeout: 0s

ota:
  - platform: esphome

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

web_server:
  port: 80
  version: 3

substitutions:
  petkit_mac: "A4:C1:38:73:4F:40"
  service_uuid: "0000aaa0-0000-1000-8000-00805f9b34fb"
  notify_uuid:  "0000aaa1-0000-1000-8000-00805f9b34fb"
  write_uuid:   "0000aaa2-0000-1000-8000-00805f9b34fb"

esp32_ble_tracker:

ble_client:
  - mac_address: ${petkit_mac}
    id: petkit_client
    auto_connect: true

# ---------- Core sensor platform (creates the parent instance) ----------
sensor:
  - platform: petkit_fountain
    id: petkit
    ble_client_id: petkit_client
    service_uuid: ${service_uuid}
    notify_uuid: ${notify_uuid}
    write_uuid: ${write_uuid}

    # State/config sensors
    power: { name: "Petkit Power (raw)" }
    mode: { name: "Petkit Mode (raw)" }
    is_night_dnd: { name: "Petkit Night DND (raw)" }
    filter_percent: { name: "Petkit Filter %" }
    run_status: { name: "Petkit Run Status (raw)" }
    water_pump_runtime_seconds: { name: "Petkit Pump Runtime (s)" }
    today_pump_runtime_seconds: { name: "Petkit Today Pump Runtime (s)" }
    today_purified_water_times: { name: "Petkit Today Purified Water Times" }
    today_energy_kwh: { name: "Petkit Today Energy (raw)" }
    smart_working_time: { name: "Petkit Smart Working Time (min)" }
    smart_sleep_time: { name: "Petkit Smart Sleep Time (min)" }
    light_switch: { name: "Petkit Light Switch (raw)" }
    light_brightness: { name: "Petkit Light Brightness (raw)" }
    light_schedule_start_min: { name: "Petkit Light Start (raw min)" }
    light_schedule_end_min: { name: "Petkit Light End (raw min)" }
    dnd_switch: { name: "Petkit DND Switch (raw)" }
    dnd_start_min: { name: "Petkit DND Start (raw min)" }
    dnd_end_min: { name: "Petkit DND End (raw min)" }
    filter_remaining_days: { name: "Petkit Filter Remaining Days (calc)" }

# ---------- Binary sensors (warnings) ----------
binary_sensor:
  - platform: petkit_fountain
    parent_id: petkit
    lack_warning: { name: "Petkit Lack Warning" }
    breakdown_warning: { name: "Petkit Breakdown Warning" }
    filter_warning: { name: "Petkit Filter Warning" }

# ---------- Switches (controls) ----------
switch:
  - platform: petkit_fountain
    parent_id: petkit
    power_switch: { name: "Petkit Power" }
    light_switch: { name: "Petkit Light" }
    dnd_switch: { name: "Petkit DND" }

# ---------- Numbers (configuration values) ----------
number:
  - platform: petkit_fountain
    parent_id: petkit

    # Many devices only support 0..2 levels. Adjust if your model supports more.
    light_brightness:
      name: "Petkit Brightness"
      min_value: 0
      max_value: 2
      step: 1

    light_schedule_start_min:
      name: "Petkit Light Start (min)"
      min_value: 0
      max_value: 1439
      step: 1

    light_schedule_end_min:
      name: "Petkit Light End (min)"
      min_value: 0
      max_value: 1439
      step: 1

    dnd_start_min:
      name: "Petkit DND Start (min)"
      min_value: 0
      max_value: 1439
      step: 1

    dnd_end_min:
      name: "Petkit DND End (min)"
      min_value: 0
      max_value: 1439
      step: 1

    smart_on:
      name: "Petkit Smart On (min)"
      min_value: 0
      max_value: 255
      step: 1

    smart_off:
      name: "Petkit Smart Off (min)"
      min_value: 0
      max_value: 255
      step: 1

# ---------- Select (mode control) ----------
select:
  - platform: petkit_fountain
    parent_id: petkit
    mode_select:
      name: "Petkit Mode"
      # typically: normal/smart (mapped to protocol values)

# ---------- Buttons (manual actions) ----------
button:
  - platform: petkit_fountain
    parent_id: petkit
    refresh_state: { name: "Petkit Refresh (CMD210)" }
    read_config:   { name: "Petkit Read Config (CMD211)" }
    set_datetime:  { name: "Petkit Set Datetime (CMD84)" }
    reset_filter:  { name: "Petkit Reset Filter (CMD222)" }
    init_session:  { name: "Petkit Init Session (CMD73)" }
    sync:          { name: "Petkit Sync (CMD86)" }

# ---------- Text sensors (identifiers) ----------
text_sensor:
  - platform: petkit_fountain
    parent_id: petkit
    serial:
      name: "Petkit Serial"
```

---

## Stability Note (Reboots / Stack Size)

If the ESP reboots under BLE load (especially with `logger: DEBUG`), increasing the loop task stack can help.

```yaml
esp32:
  board: wemos_d1_mini32
  framework:
    type: esp-idf
    advanced:
      loop_task_stack_size: 32768
```

Use this as a stability workaround when logs show stack-related crashes (stack canary, stack overflow, Guru Meditation).  
After stabilizing, you can try smaller values to recover RAM.

---

## Multiple Fountains On One ESP (Separate HA Devices)

You can run multiple Petkit fountains from one ESP by creating:
- one `ble_client` per fountain (`mac_address` differs),
- one `sensor: - platform: petkit_fountain` parent per fountain,
- optional ESPHome `sub-devices` + `device_id` for clean separation in Home Assistant.

```yaml
esphome:
  name: petkit-ble-hub
  min_version: 2025.7.0
  devices:
    - id: fountain_kitchen
      name: "Petkit Kitchen"
    - id: fountain_hall
      name: "Petkit Hall"

ble_client:
  - mac_address: "AA:BB:CC:DD:EE:01"
    id: petkit_client_1
    auto_connect: true
  - mac_address: "AA:BB:CC:DD:EE:02"
    id: petkit_client_2
    auto_connect: true

sensor:
  - platform: petkit_fountain
    id: petkit_1
    ble_client_id: petkit_client_1
    service_uuid: "0000aaa0-0000-1000-8000-00805f9b34fb"
    notify_uuid: "0000aaa1-0000-1000-8000-00805f9b34fb"
    write_uuid: "0000aaa2-0000-1000-8000-00805f9b34fb"
    filter_percent: { name: "Kitchen Filter %", device_id: fountain_kitchen }

  - platform: petkit_fountain
    id: petkit_2
    ble_client_id: petkit_client_2
    service_uuid: "0000aaa0-0000-1000-8000-00805f9b34fb"
    notify_uuid: "0000aaa1-0000-1000-8000-00805f9b34fb"
    write_uuid: "0000aaa2-0000-1000-8000-00805f9b34fb"
    filter_percent: { name: "Hall Filter %", device_id: fountain_hall }
```

Apply the same pattern to `switch`, `number`, `select`, `button`, `text_sensor`, and `binary_sensor` entities by setting `device_id` per fountain.

---

## How It Works

### BLE Connection
ESPHome handles BLE scanning/connection using:
- `esp32_ble_tracker` (scanner)
- `ble_client` (connection manager + reconnect logic)

The Petkit component registers as a `BLEClientNode`, subscribes to the notify characteristic, and sends Petkit protocol frames to the write characteristic.

### Protocol Basics
Frames are simple byte packets:

- Header: `FA FC FD`
- Command byte (e.g. `0xD5` for 213, `0xD2` for 210 response, `0xE6` for periodic status)
- Type byte:
  - `1` = request (TX)
  - `2` = response/ack (RX)
- Sequence
- Length
- Data...
- End byte: `FB`

### Recommended Init Sequence
Some devices require a short handshake before returning full data:

1. **CMD73**: session init / generates `secret`
2. **CMD86**: sync (needs secret)
3. **CMD84**: set datetime (optional but recommended)
4. **CMD210**: read state
5. **CMD211**: read configuration

If you skip CMD211, you may still see periodic `E6` frames, but config values (light/dnd schedules, brightness) may be missing or not updated reliably.

---

## Entities (Overview)

### Sensors
- `power` (raw power state)
- `mode` (raw mode)
- `filter_percent`
- `is_night_dnd`
- `run_status`
- `water_pump_runtime_seconds`
- `today_pump_runtime_seconds`
- `today_purified_water_times`
- `today_energy_kwh`
- `smart_working_time`
- `smart_sleep_time`
- `light_switch` (raw)
- `light_brightness` (raw)
- `light_schedule_start_min` (raw)
- `light_schedule_end_min` (raw)
- `dnd_switch` (raw)
- `dnd_start_min` (raw)
- `dnd_end_min` (raw)
- `filter_remaining_days` (calculated)

### Binary Sensors
- `lack_warning`
- `breakdown_warning`
- `filter_warning`

### Switches
- `power_switch`
- `light_switch`
- `dnd_switch`

### Numbers
- `light_brightness` (often 0..2)
- `light_schedule_start_min` (0..1439)
- `light_schedule_end_min`
- `dnd_start_min` (0..1439)
- `dnd_end_min`
- `smart_on` (0..255)
- `smart_off` (0..255)

### Select
- `mode_select` (e.g. normal/smart)

### Buttons
- Refresh (CMD210)
- Read Config (CMD211)
- Set Datetime (CMD84)
- Reset Filter (CMD222)
- Init Session (CMD73)
- Sync (CMD86)

### Text Sensors
- Serial number (from CMD213 response)

---

## Troubleshooting

### Frequent disconnects (reason 0x13)
`0x13` is a common BLE disconnect reason (remote/user terminated / link loss). Typical causes:
- weak RSSI / distance
- concurrent BLE clients (phone app or another proxy)
- device firmware rejecting writes without proper init chain
- too aggressive command rate (send fewer commands / add delays)

### Brightness only shows 0..2
This is normal for many models; treat brightness as a level selector rather than a 0..255 dimmer.

---

### Inspiration, Resources & Acknowledgements

- [RobertD502](https://github.com/RobertD502): [Home Assistant Petkit Custom Integration](https://www.reddit.com/r/homeassistant/comments/145ebp1/petkit_custom_integration/)
- [RobertD502](https://github.com/RobertD502): [petkitaio](https://github.com/RobertD502/petkitaio)
- [PetKit Eversweet Pro 3 - Decoding BLE response](https://colab.research.google.com/drive/1gWwLz1Wi_WujvvSaTJpPMW5i3YideSAb#scrollTo=RWO3w4-XTmNd)
- [slespersen](https://github.com/slespersen): [PetkitW5BLEMQTT](https://github.com/slespersen/PetkitW5BLEMQTT)

---

## Disclaimer
This is a community project. Petkit’s BLE protocol is undocumented and may differ by model/firmware. Use at your own risk.
