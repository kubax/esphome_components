# my_components/petkit_fountain/number.py
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

CONF_ID_PARENT = CONF_ID

CONF_LIGHT_BRIGHTNESS = "light_brightness"
CONF_LIGHT_SCHEDULE_START_MIN = "light_schedule_start_min"
CONF_LIGHT_SCHEDULE_END_MIN = "light_schedule_end_min"
CONF_DND_START_MIN = "dnd_start_min"
CONF_DND_END_MIN = "dnd_end_min"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID_PARENT): cv.use_id(PetkitFountain),
        cv.Optional(CONF_LIGHT_BRIGHTNESS): number.number_schema(),
        cv.Optional(CONF_LIGHT_SCHEDULE_START_MIN): number.number_schema(),
        cv.Optional(CONF_LIGHT_SCHEDULE_END_MIN): number.number_schema(),
        cv.Optional(CONF_DND_START_MIN): number.number_schema(),
        cv.Optional(CONF_DND_END_MIN): number.number_schema(),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID_PARENT])

    if CONF_LIGHT_BRIGHTNESS in config:
        n = await number.new_number(config[CONF_LIGHT_BRIGHTNESS])
        cg.add(parent.set_light_brightness_number(n))

    if CONF_LIGHT_SCHEDULE_START_MIN in config:
        n = await number.new_number(config[CONF_LIGHT_SCHEDULE_START_MIN])
        cg.add(parent.set_light_schedule_start_number(n))

    if CONF_LIGHT_SCHEDULE_END_MIN in config:
        n = await number.new_number(config[CONF_LIGHT_SCHEDULE_END_MIN])
        cg.add(parent.set_light_schedule_end_number(n))

    if CONF_DND_START_MIN in config:
        n = await number.new_number(config[CONF_DND_START_MIN])
        cg.add(parent.set_dnd_start_number(n))

    if CONF_DND_END_MIN in config:
        n = await number.new_number(config[CONF_DND_END_MIN])
        cg.add(parent.set_dnd_end_number(n))
