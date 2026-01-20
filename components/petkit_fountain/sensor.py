# my_components/petkit_fountain/sensor.py
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, sensor
from esphome.const import CONF_ID

CONF_BLE_CLIENT_ID = "ble_client_id"
CONF_SERVICE_UUID = "service_uuid"
CONF_NOTIFY_UUID = "notify_uuid"
CONF_WRITE_UUID = "write_uuid"

# sensors
CONF_POWER = "power"
CONF_MODE = "mode"
CONF_IS_NIGHT_DND = "is_night_dnd"
CONF_BREAKDOWN_WARNING = "breakdown_warning"
CONF_LACK_WARNING = "lack_warning"
CONF_FILTER_WARNING = "filter_warning"
CONF_FILTER_PERCENT = "filter_percent"
CONF_RUN_STATUS = "run_status"
CONF_WATER_PUMP_RUNTIME_SECONDS = "water_pump_runtime_seconds"
CONF_TODAY_PUMP_RUNTIME_SECONDS = "today_pump_runtime_seconds"
CONF_TODAY_PURIFIED_WATER_TIMES = "today_purified_water_times"
CONF_TODAY_ENERGY_KWH = "today_energy_kwh"
CONF_SMART_WORKING_TIME = "smart_working_time"
CONF_SMART_SLEEP_TIME = "smart_sleep_time"
CONF_LIGHT_SWITCH_STATE = "light_switch_state"
CONF_LIGHT_BRIGHTNESS_STATE = "light_brightness_state"
CONF_LIGHT_SCHEDULE_START_MIN = "light_schedule_start_min"
CONF_LIGHT_SCHEDULE_END_MIN = "light_schedule_end_min"
CONF_DND_SWITCH_STATE = "dnd_switch_state"
CONF_DND_START_MIN = "dnd_start_min"
CONF_DND_END_MIN = "dnd_end_min"
CONF_BATTERY = "battery"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain", cg.PollingComponent, ble_client.BLEClientNode)

def _opt_sensor():
    return sensor.sensor_schema()

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PetkitFountain),
        cv.Required(CONF_BLE_CLIENT_ID): cv.use_id(ble_client.BLEClient),
        cv.Required(CONF_SERVICE_UUID): cv.string,
        cv.Required(CONF_NOTIFY_UUID): cv.string,
        cv.Required(CONF_WRITE_UUID): cv.string,

        cv.Optional(CONF_POWER): _opt_sensor(),
        cv.Optional(CONF_MODE): _opt_sensor(),
        cv.Optional(CONF_IS_NIGHT_DND): _opt_sensor(),
        cv.Optional(CONF_BREAKDOWN_WARNING): _opt_sensor(),
        cv.Optional(CONF_LACK_WARNING): _opt_sensor(),
        cv.Optional(CONF_FILTER_WARNING): _opt_sensor(),
        cv.Optional(CONF_FILTER_PERCENT): _opt_sensor(),
        cv.Optional(CONF_RUN_STATUS): _opt_sensor(),
        cv.Optional(CONF_WATER_PUMP_RUNTIME_SECONDS): _opt_sensor(),
        cv.Optional(CONF_TODAY_PUMP_RUNTIME_SECONDS): _opt_sensor(),
        cv.Optional(CONF_TODAY_PURIFIED_WATER_TIMES): _opt_sensor(),
        cv.Optional(CONF_TODAY_ENERGY_KWH): _opt_sensor(),
        cv.Optional(CONF_SMART_WORKING_TIME): _opt_sensor(),
        cv.Optional(CONF_SMART_SLEEP_TIME): _opt_sensor(),
        cv.Optional(CONF_LIGHT_SWITCH_STATE): _opt_sensor(),
        cv.Optional(CONF_LIGHT_BRIGHTNESS_STATE): _opt_sensor(),
        cv.Optional(CONF_LIGHT_SCHEDULE_START_MIN): _opt_sensor(),
        cv.Optional(CONF_LIGHT_SCHEDULE_END_MIN): _opt_sensor(),
        cv.Optional(CONF_DND_SWITCH_STATE): _opt_sensor(),
        cv.Optional(CONF_DND_START_MIN): _opt_sensor(),
        cv.Optional(CONF_DND_END_MIN): _opt_sensor(),
        cv.Optional(CONF_BATTERY): _opt_sensor(),
    }
).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    var = cg.new_Pvariable(
        config[CONF_ID],
        config[CONF_SERVICE_UUID],
        config[CONF_NOTIFY_UUID],
        config[CONF_WRITE_UUID],
    )
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)

    def bind(key, setter):
        if key in config:
            s = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(s))

    bind(CONF_POWER, "set_power_sensor")
    bind(CONF_MODE, "set_mode_sensor")
    bind(CONF_IS_NIGHT_DND, "set_is_night_dnd_sensor")
    bind(CONF_BREAKDOWN_WARNING, "set_breakdown_warning_sensor")
    bind(CONF_LACK_WARNING, "set_lack_warning_sensor")
    bind(CONF_FILTER_WARNING, "set_filter_warning_sensor")
    bind(CONF_FILTER_PERCENT, "set_filter_percent_sensor")
    bind(CONF_RUN_STATUS, "set_run_status_sensor")
    bind(CONF_WATER_PUMP_RUNTIME_SECONDS, "set_water_pump_runtime_seconds_sensor")
    bind(CONF_TODAY_PUMP_RUNTIME_SECONDS, "set_today_pump_runtime_seconds_sensor")
    bind(CONF_TODAY_PURIFIED_WATER_TIMES, "set_today_purified_water_times_sensor")
    bind(CONF_TODAY_ENERGY_KWH, "set_today_energy_kwh_sensor")
    bind(CONF_SMART_WORKING_TIME, "set_smart_working_time_sensor")
    bind(CONF_SMART_SLEEP_TIME, "set_smart_sleep_time_sensor")
    bind(CONF_LIGHT_SWITCH_STATE, "set_light_switch_state_sensor")
    bind(CONF_LIGHT_BRIGHTNESS_STATE, "set_light_brightness_state_sensor")
    bind(CONF_LIGHT_SCHEDULE_START_MIN, "set_light_schedule_start_min_sensor")
    bind(CONF_LIGHT_SCHEDULE_END_MIN, "set_light_schedule_end_min_sensor")
    bind(CONF_DND_SWITCH_STATE, "set_dnd_switch_state_sensor")
    bind(CONF_DND_START_MIN, "set_dnd_start_min_sensor")
    bind(CONF_DND_END_MIN, "set_dnd_end_min_sensor")
    bind(CONF_BATTERY, "set_battery_sensor")
