# my_components/petkit_fountain/switch.py
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

CONF_ID_PARENT = CONF_ID

CONF_LIGHT_SWITCH = "light_switch"
CONF_DND_SWITCH = "dnd_switch"
CONF_POWER_SWITCH = "power_switch"  # optional: on/off (CMD220 state)

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID_PARENT): cv.use_id(PetkitFountain),
        cv.Optional(CONF_LIGHT_SWITCH): switch.switch_schema(),
        cv.Optional(CONF_DND_SWITCH): switch.switch_schema(),
        cv.Optional(CONF_POWER_SWITCH): switch.switch_schema(),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID_PARENT])

    if CONF_LIGHT_SWITCH in config:
        sw = await switch.new_switch(config[CONF_LIGHT_SWITCH])
        cg.add(parent.set_light_switch(sw))

    if CONF_DND_SWITCH in config:
        sw = await switch.new_switch(config[CONF_DND_SWITCH])
        cg.add(parent.set_dnd_switch(sw))

    if CONF_POWER_SWITCH in config:
        sw = await switch.new_switch(config[CONF_POWER_SWITCH])
        cg.add(parent.set_power_switch(sw))
