import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID

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

    def _mk(key, action_name):
        if key in config:
            b = await button.new_button(config[key])
            cg.add(b.set_parent(parent))
            # action enum set via integer mapping, keeps it simple
            # REFRESH=0 RESET_FILTER=1 SET_DATETIME=2 INIT_SESSION=3 SYNC=4
            mapping = {
                "REFRESH": 0,
                "RESET_FILTER": 1,
                "SET_DATETIME": 2,
                "INIT_SESSION": 3,
                "SYNC": 4,
            }
            cg.add(b.set_action(mapping[action_name]))

    _mk(CONF_REFRESH_STATE, "REFRESH")
    _mk(CONF_RESET_FILTER, "RESET_FILTER")
    _mk(CONF_SET_DATETIME, "SET_DATETIME")
    _mk(CONF_INIT_SESSION, "INIT_SESSION")
    _mk(CONF_SYNC, "SYNC")
