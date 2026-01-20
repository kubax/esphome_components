import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

CONF_PARENT_ID = "parent_id"

CONF_LIGHT_BRIGHTNESS = "light_brightness"
CONF_LIGHT_SCHEDULE_START_MIN = "light_schedule_start_min"
CONF_LIGHT_SCHEDULE_END_MIN = "light_schedule_end_min"
CONF_DND_START_MIN = "dnd_start_min"
CONF_DND_END_MIN = "dnd_end_min"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")

PetkitBrightnessNumber = petkit_ns.class_("PetkitBrightnessNumber", number.Number)
PetkitTimeNumber = petkit_ns.class_("PetkitTimeNumber", number.Number)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PARENT_ID): cv.use_id(PetkitFountain),

        cv.Optional(CONF_LIGHT_BRIGHTNESS): number.number_schema(PetkitBrightnessNumber),

        cv.Optional(CONF_LIGHT_SCHEDULE_START_MIN): number.number_schema(PetkitTimeNumber),
        cv.Optional(CONF_LIGHT_SCHEDULE_END_MIN): number.number_schema(PetkitTimeNumber),
        cv.Optional(CONF_DND_START_MIN): number.number_schema(PetkitTimeNumber),
        cv.Optional(CONF_DND_END_MIN): number.number_schema(PetkitTimeNumber),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    if CONF_LIGHT_BRIGHTNESS in config:
        n = await number.new_number(config[CONF_LIGHT_BRIGHTNESS])
        cg.add(n.set_parent(parent))

    if CONF_LIGHT_SCHEDULE_START_MIN in config:
        n = await number.new_number(config[CONF_LIGHT_SCHEDULE_START_MIN])
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(petkit_ns.enum("PetkitTimeNumber::Kind").LIGHT_START))

    if CONF_LIGHT_SCHEDULE_END_MIN in config:
        n = await number.new_number(config[CONF_LIGHT_SCHEDULE_END_MIN])
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(petkit_ns.enum("PetkitTimeNumber::Kind").LIGHT_END))

    if CONF_DND_START_MIN in config:
        n = await number.new_number(config[CONF_DND_START_MIN])
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(petkit_ns.enum("PetkitTimeNumber::Kind").DND_START))

    if CONF_DND_END_MIN in config:
        n = await number.new_number(config[CONF_DND_END_MIN])
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(petkit_ns.enum("PetkitTimeNumber::Kind").DND_END))
