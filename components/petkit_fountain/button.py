import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button

CONF_PARENT_ID = "parent_id"

CONF_REFRESH_STATE = "refresh_state"
CONF_RESET_FILTER = "reset_filter"
CONF_SET_DATETIME = "set_datetime"
CONF_INIT_SESSION = "init_session"
CONF_SYNC = "sync"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")
PetkitActionButton = petkit_ns.class_("PetkitActionButton", button.Button)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PARENT_ID): cv.use_id(PetkitFountain),

        cv.Optional(CONF_REFRESH_STATE): button.button_schema(PetkitActionButton),
        cv.Optional(CONF_RESET_FILTER): button.button_schema(PetkitActionButton),
        cv.Optional(CONF_SET_DATETIME): button.button_schema(PetkitActionButton),
        cv.Optional(CONF_INIT_SESSION): button.button_schema(PetkitActionButton),
        cv.Optional(CONF_SYNC): button.button_schema(PetkitActionButton),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    # Action mapping: REFRESH=0 RESET_FILTER=1 SET_DATETIME=2 INIT_SESSION=3 SYNC=4
    if CONF_REFRESH_STATE in config:
        b = await button.new_button(config[CONF_REFRESH_STATE])
        cg.add(b.set_parent(parent))
        cg.add(parent.set_action_button(b))
        cg.add(b.set_action(0))

    if CONF_RESET_FILTER in config:
        b = await button.new_button(config[CONF_RESET_FILTER])
        cg.add(b.set_parent(parent))
        cg.add(parent.set_action_button(b))
        cg.add(b.set_action(1))

    if CONF_SET_DATETIME in config:
        b = await button.new_button(config[CONF_SET_DATETIME])
        cg.add(b.set_parent(parent))
        cg.add(parent.set_action_button(b))
        cg.add(b.set_action(2))

    if CONF_INIT_SESSION in config:
        b = await button.new_button(config[CONF_INIT_SESSION])
        cg.add(b.set_parent(parent))
        cg.add(parent.set_action_button(b))
        cg.add(b.set_action(3))

    if CONF_SYNC in config:
        b = await button.new_button(config[CONF_SYNC])
        cg.add(b.set_parent(parent))
        cg.add(parent.set_action_button(b))
        cg.add(b.set_action(4))
