import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, sensor
from esphome.const import CONF_ID

CONF_BLE_CLIENT_ID = "ble_client_id"
CONF_SERVICE_UUID = "service_uuid"
CONF_NOTIFY_UUID = "notify_uuid"
CONF_WRITE_UUID = "write_uuid"

# Sensor keys
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
CONF_LIGHT_SWITCH = "light_switch"
CONF_LIGHT_BRIGHTNESS = "light_brightness"
CONF_LIGHT_SCHEDULE_START_MIN = "light_schedule_start_min"
CONF_LIGHT_SCHEDULE_END_MIN = "light_schedule_end_min"
CONF_DND_SWITCH = "dnd_switch"
CONF_DND_START_MIN = "dnd_start_min"
CONF_DND_END_MIN = "dnd_end_min"

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
        cv.Optional(CONF_LIGHT_SWITCH): _opt_sensor(),
        cv.Optional(CONF_LIGHT_BRIGHTNESS): _opt_sensor(),
        cv.Optional(CONF_LIGHT_SCHEDULE_START_MIN): _opt_sensor(),
        cv.Optional(CONF_LIGHT_SCHEDULE_END_MIN): _opt_sensor(),
        cv.Optional(CONF_DND_SWITCH): _opt_sensor(),
        cv.Optional(CONF_DND_START_MIN): _opt_sensor(),
        cv.Optional(CONF_DND_END_MIN): _opt_sensor(),
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

    # Create sensors if configured and pass into C++
    if CONF_POWER in config:
        s = await sensor.new_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(s))
    if CONF_MODE in config:
        s = await sensor.new_sensor(config[CONF_MODE])
        cg.add(var.set_mode_sensor(s))
    if CONF_IS_NIGHT_DND in config:
        s = await sensor.new_sensor(config[CONF_IS_NIGHT_DND])
        cg.add(var.set_is_night_dnd_sensor(s))
    if CONF_BREAKDOWN_WARNING in config:
        s = await sensor.new_sensor(config[CONF_BREAKDOWN_WARNING])
        cg.add(var.set_breakdown_warning_sensor(s))
    if CONF_LACK_WARNING in config:
        s = await sensor.new_sensor(config[CONF_LACK_WARNING])
        cg.add(var.set_lack_warning_sensor(s))
    if CONF_FILTER_WARNING in config:
        s = await sensor.new_sensor(config[CONF_FILTER_WARNING])
        cg.add(var.set_filter_warning_sensor(s))
    if CONF_FILTER_PERCENT in config:
        s = await sensor.new_sensor(config[CONF_FILTER_PERCENT])
        cg.add(var.set_filter_percent_sensor(s))
    if CONF_RUN_STATUS in config:
        s = await sensor.new_sensor(config[CONF_RUN_STATUS])
        cg.add(var.set_run_status_sensor(s))
    if CONF_WATER_PUMP_RUNTIME_SECONDS in config:
        s = await sensor.new_sensor(config[CONF_WATER_PUMP_RUNTIME_SECONDS])
        cg.add(var.set_water_pump_runtime_seconds_sensor(s))
    if CONF_TODAY_PUMP_RUNTIME_SECONDS in config:
        s = await sensor.new_sensor(config[CONF_TODAY_PUMP_RUNTIME_SECONDS])
        cg.add(var.set_today_pump_runtime_seconds_sensor(s))
    if CONF_TODAY_PURIFIED_WATER_TIMES in config:
        s = await sensor.new_sensor(config[CONF_TODAY_PURIFIED_WATER_TIMES])
        cg.add(var.set_today_purified_water_times_sensor(s))
    if CONF_TODAY_ENERGY_KWH in config:
        s = await sensor.new_sensor(config[CONF_TODAY_ENERGY_KWH])
        cg.add(var.set_today_energy_kwh_sensor(s))
    if CONF_SMART_WORKING_TIME in config:
        s = await sensor.new_sensor(config[CONF_SMART_WORKING_TIME])
        cg.add(var.set_smart_working_time_sensor(s))
    if CONF_SMART_SLEEP_TIME in config:
        s = await sensor.new_sensor(config[CONF_SMART_SLEEP_TIME])
        cg.add(var.set_smart_sleep_time_sensor(s))
    if CONF_LIGHT_SWITCH in config:
        s = await sensor.new_sensor(config[CONF_LIGHT_SWITCH])
        cg.add(var.set_light_switch_sensor(s))
    if CONF_LIGHT_BRIGHTNESS in config:
        s = await sensor.new_sensor(config[CONF_LIGHT_BRIGHTNESS])
        cg.add(var.set_light_brightness_sensor(s))
    if CONF_LIGHT_SCHEDULE_START_MIN in config:
        s = await sensor.new_sensor(config[CONF_LIGHT_SCHEDULE_START_MIN])
        cg.add(var.set_light_schedule_start_min_sensor(s))
    if CONF_LIGHT_SCHEDULE_END_MIN in config:
        s = await sensor.new_sensor(config[CONF_LIGHT_SCHEDULE_END_MIN])
        cg.add(var.set_light_schedule_end_min_sensor(s))
    if CONF_DND_SWITCH in config:
        s = await sensor.new_sensor(config[CONF_DND_SWITCH])
        cg.add(var.set_dnd_switch_sensor(s))
    if CONF_DND_START_MIN in config:
        s = await sensor.new_sensor(config[CONF_DND_START_MIN])
        cg.add(var.set_dnd_start_min_sensor(s))
    if CONF_DND_END_MIN in config:
        s = await sensor.new_sensor(config[CONF_DND_END_MIN])
        cg.add(var.set_dnd_end_min_sensor(s))
