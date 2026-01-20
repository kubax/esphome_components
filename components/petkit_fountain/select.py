import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID

CONF_PARENT_ID = "parent_id"
CONF_MODE_SELECT = "mode_select"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")
PetkitModeSelect = petkit_ns.class_("PetkitModeSelect", select.Select)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PARENT_ID): cv.use_id(PetkitFountain),
        cv.Optional(CONF_MODE_SELECT): select.select_schema(PetkitModeSelect, options=["normal", "smart"]),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    if CONF_MODE_SELECT in config:
        s = await select.new_select(config[CONF_MODE_SELECT], options=["normal", "smart"])
        cg.add(s.set_parent(parent))
