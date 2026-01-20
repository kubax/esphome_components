import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_NAME

from . import petkit_fountain_ns, PetkitFountain, CONF_PARENT_ID

CONF_SERIAL = "serial"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PetkitFountain),
    cv.Required(CONF_PARENT_ID): cv.use_id(PetkitFountain),
    cv.Optional(CONF_SERIAL): text_sensor.text_sensor_schema(),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    if CONF_SERIAL in config:
        ts = await text_sensor.new_text_sensor(config[CONF_SERIAL])
        cg.add(parent.set_serial_text_sensor(ts))
