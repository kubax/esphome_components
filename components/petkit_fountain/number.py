import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

CONF_PARENT_ID = "parent_id"

# per-number config keys
CONF_MIN_VALUE = "min_value"
CONF_MAX_VALUE = "max_value"
CONF_STEP = "step"

CONF_LIGHT_BRIGHTNESS = "light_brightness"
CONF_LIGHT_SCHEDULE_START_MIN = "light_schedule_start_min"
CONF_LIGHT_SCHEDULE_END_MIN = "light_schedule_end_min"
CONF_DND_START_MIN = "dnd_start_min"
CONF_DND_END_MIN = "dnd_end_min"

petkit_ns = cg.esphome_ns.namespace("petkit_fountain")
PetkitFountain = petkit_ns.class_("PetkitFountain")

PetkitBrightnessNumber = petkit_ns.class_("PetkitBrightnessNumber", number.Number)
PetkitTimeNumber = petkit_ns.class_("PetkitTimeNumber", number.Number)


def _num_schema(class_):
    # Extend the base schema so YAML can use min/max/step
    return (
        number.number_schema(class_)
        .extend(
            {
                cv.Optional(CONF_MIN_VALUE): cv.float_,
                cv.Optional(CONF_MAX_VALUE): cv.float_,
                cv.Optional(CONF_STEP): cv.float_,
            }
        )
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PARENT_ID): cv.use_id(PetkitFountain),

        cv.Optional(CONF_LIGHT_BRIGHTNESS): _num_schema(PetkitBrightnessNumber),

        cv.Optional(CONF_LIGHT_SCHEDULE_START_MIN): _num_schema(PetkitTimeNumber),
        cv.Optional(CONF_LIGHT_SCHEDULE_END_MIN): _num_schema(PetkitTimeNumber),
        cv.Optional(CONF_DND_START_MIN): _num_schema(PetkitTimeNumber),
        cv.Optional(CONF_DND_END_MIN): _num_schema(PetkitTimeNumber),
    }
)


async def _apply_limits(n, cfg):
    # Apply min/max/step if present
    if CONF_MIN_VALUE in cfg:
        cg.add(n.set_min_value(cfg[CONF_MIN_VALUE]))
    if CONF_MAX_VALUE in cfg:
        cg.add(n.set_max_value(cfg[CONF_MAX_VALUE]))
    if CONF_STEP in cfg:
        cg.add(n.set_step(cfg[CONF_STEP]))


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    if CONF_LIGHT_BRIGHTNESS in config:
        cfg = config[CONF_LIGHT_BRIGHTNESS]
        n = await number.new_number(cfg)
        cg.add(n.set_parent(parent))
        await _apply_limits(n, cfg)

    # Time kinds: 0=LIGHT_START, 1=LIGHT_END, 2=DND_START, 3=DND_END
    if CONF_LIGHT_SCHEDULE_START_MIN in config:
        cfg = config[CONF_LIGHT_SCHEDULE_START_MIN]
        n = await number.new_number(cfg)
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(0))
        await _apply_limits(n, cfg)

    if CONF_LIGHT_SCHEDULE_END_MIN in config:
        cfg = config[CONF_LIGHT_SCHEDULE_END_MIN]
        n = await number.new_number(cfg)
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(1))
        await _apply_limits(n, cfg)

    if CONF_DND_START_MIN in config:
        cfg = config[CONF_DND_START_MIN]
        n = await number.new_number(cfg)
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(2))
        await _apply_limits(n, cfg)

    if CONF_DND_END_MIN in config:
        cfg = config[CONF_DND_END_MIN]
        n = await number.new_number(cfg)
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(3))
        await _apply_limits(n, cfg)
