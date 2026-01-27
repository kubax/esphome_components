import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

CONF_PARENT_ID = "parent_id"
CONF_LACK_WARNING = "lack_warning"
CONF_BREAKDOWN_WARNING = "breakdown_warning"
CONF_FILTER_WARNING = "filter_warning"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")

def _opt_bs():
    return binary_sensor.binary_sensor_schema()

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PetkitFountain),
        cv.Required(CONF_PARENT_ID): cv.use_id(PetkitFountain),

        cv.Optional(CONF_LACK_WARNING): _opt_bs(),
        cv.Optional(CONF_BREAKDOWN_WARNING): _opt_bs(),
        cv.Optional(CONF_FILTER_WARNING): _opt_bs(),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    if CONF_LACK_WARNING in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_LACK_WARNING])
        cg.add(parent.set_lack_warning_binary_sensor(bs))

    if CONF_BREAKDOWN_WARNING in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_BREAKDOWN_WARNING])
        cg.add(parent.set_breakdown_warning_binary_sensor(bs))

    if CONF_FILTER_WARNING in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_FILTER_WARNING])
        cg.add(parent.set_filter_warning_binary_sensor(bs))
