import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, binary_sensor
from esphome.const import CONF_ID

CONF_BLE_CLIENT_ID = "ble_client_id"
CONF_SERVICE_UUID = "service_uuid"
CONF_NOTIFY_UUID = "notify_uuid"
CONF_WRITE_UUID = "write_uuid"

# Binary sensor keys (match semantics; you can enable any subset in YAML)
CONF_LACK_WARNING = "lack_warning"
CONF_BREAKDOWN_WARNING = "breakdown_warning"
CONF_FILTER_WARNING = "filter_warning"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain", cg.PollingComponent, ble_client.BLEClientNode)


def _opt_bin(device_class: str = "problem"):
    # Default: device_class problem (nice HA UI)
    return binary_sensor.binary_sensor_schema(device_class=device_class)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PetkitFountain),
        cv.Required(CONF_BLE_CLIENT_ID): cv.use_id(ble_client.BLEClient),
        cv.Required(CONF_SERVICE_UUID): cv.string,
        cv.Required(CONF_NOTIFY_UUID): cv.string,
        cv.Required(CONF_WRITE_UUID): cv.string,

        # Optional binary sensors
        cv.Optional(CONF_LACK_WARNING): _opt_bin("problem"),
        cv.Optional(CONF_BREAKDOWN_WARNING): _opt_bin("problem"),
        cv.Optional(CONF_FILTER_WARNING): _opt_bin("problem"),
    }
).extend(cv.polling_component_schema("60s"))


async def to_code(config):
    # Important: must construct the SAME C++ component instance as sensor.py does.
    # In ESPHome, only one of the platform modules should create the component.
    #
    # If you already create the component in sensor.py, then binary_sensor.py must
    # NOT create a second instance. Instead we retrieve it via ID.
    #
    # This file assumes you use the same 'id:' in YAML under this platform.
    var = cg.new_Pvariable(
        config[CONF_ID],
        config[CONF_SERVICE_UUID],
        config[CONF_NOTIFY_UUID],
        config[CONF_WRITE_UUID],
    )
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)

    if CONF_LACK_WARNING in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_LACK_WARNING])
        cg.add(var.set_lack_warning_binary_sensor(bs))

    if CONF_BREAKDOWN_WARNING in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_BREAKDOWN_WARNING])
        cg.add(var.set_breakdown_warning_binary_sensor(bs))

    if CONF_FILTER_WARNING in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_FILTER_WARNING])
        cg.add(var.set_filter_warning_binary_sensor(bs))
