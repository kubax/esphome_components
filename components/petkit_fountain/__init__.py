import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DOMAIN = "petkit_fountain"

CONF_PARENT_ID = "parent_id"

petkit_fountain_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_fountain_ns.class_("PetkitFountain", cg.Component)
