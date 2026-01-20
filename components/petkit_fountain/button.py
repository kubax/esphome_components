# my_components/petkit_fountain/button.py
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID

CONF_ID_PARENT = CONF_ID

CONF_INIT_SESSION = "init_session"
CONF_SYNC = "sync"
CONF_SET_DATETIME = "set_datetime"
CONF_RESET_FILTER = "reset_filter"
CONF_REFRESH_STATE = "refresh_state"   # get_state+get_config+battery

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID_PARENT): cv.use_id(PetkitFountain),

        cv.Optional(CONF_INIT_SESSION): button.button_schema(),
        cv.Optional(CONF_SYNC): button.button_schema(),
        cv.Optional(CONF_SET_DATETIME): button.button_schema(),
        cv.Optional(CONF_RESET_FILTER): button.button_schema(),
        cv.Optional(CONF_REFRESH_STATE): button.button_schema(),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID_PARENT])

    if CONF_INIT_SESSION in config:
        b = await button.new_button(config[CONF_INIT_SESSION])
        cg.add(parent.set_init_session_button(b))

    if CONF_SYNC in config:
        b = await button.new_button(config[CONF_SYNC])
        cg.add(parent.set_sync_button(b))

    if CONF_SET_DATETIME in config:
        b = await button.new_button(config[CONF_SET_DATETIME])
        cg.add(parent.set_datetime_button(b))

    if CONF_RESET_FILTER in config:
        b = await button.new_button(config[CONF_RESET_FILTER])
        cg.add(parent.set_reset_filter_button(b))

    if CONF_REFRESH_STATE in config:
        b = await button.new_button(config[CONF_REFRESH_STATE])
        cg.add(parent.set_refresh_button(b))
