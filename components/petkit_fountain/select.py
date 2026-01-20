# my_components/petkit_fountain/select.py
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID

CONF_ID_PARENT = CONF_ID
CONF_MODE_SELECT = "mode_select"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID_PARENT): cv.use_id(PetkitFountain),
        cv.Optional(CONF_MODE_SELECT): select.select_schema(),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID_PARENT])
    if CONF_MODE_SELECT in config:
        sel = await select.new_select(config[CONF_MODE_SELECT], options=["normal", "smart"])
        cg.add(parent.set_mode_select(sel))
