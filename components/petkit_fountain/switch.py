import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

CONF_PARENT_ID = "parent_id"
CONF_LIGHT_SWITCH = "light_switch"
CONF_DND_SWITCH = "dnd_switch"
CONF_POWER_SWITCH = "power_switch"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")

PetkitLightSwitch = petkit_ns.class_("PetkitLightSwitch", switch.Switch)
PetkitDndSwitch = petkit_ns.class_("PetkitDndSwitch", switch.Switch)
PetkitPowerSwitch = petkit_ns.class_("PetkitPowerSwitch", switch.Switch)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PARENT_ID): cv.use_id(PetkitFountain),

        cv.Optional(CONF_LIGHT_SWITCH): switch.switch_schema(PetkitLightSwitch),
        cv.Optional(CONF_DND_SWITCH): switch.switch_schema(PetkitDndSwitch),
        cv.Optional(CONF_POWER_SWITCH): switch.switch_schema(PetkitPowerSwitch),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    if CONF_LIGHT_SWITCH in config:
        sw = await switch.new_switch(config[CONF_LIGHT_SWITCH])
        cg.add(sw.set_parent(parent))

    if CONF_DND_SWITCH in config:
        sw = await switch.new_switch(config[CONF_DND_SWITCH])
        cg.add(sw.set_parent(parent))

    if CONF_POWER_SWITCH in config:
        sw = await switch.new_switch(config[CONF_POWER_SWITCH])
        cg.add(sw.set_parent(parent))
