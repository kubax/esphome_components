import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number

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


def _num_schema(class_, default_min, default_max, default_step):
    # Allow YAML to override min/max/step
    return (
        number.number_schema(class_)
        .extend(
            {
                cv.Optional(CONF_MIN_VALUE, default=default_min): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=default_max): cv.float_,
                cv.Optional(CONF_STEP, default=default_step): cv.float_,
            }
        )
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PARENT_ID): cv.use_id(PetkitFountain),

        # brightness 0..255
        cv.Optional(CONF_LIGHT_BRIGHTNESS): _num_schema(PetkitBrightnessNumber, 0, 255, 1),

        # minutes 0..1439
        cv.Optional(CONF_LIGHT_SCHEDULE_START_MIN): _num_schema(PetkitTimeNumber, 0, 1439, 1),
        cv.Optional(CONF_LIGHT_SCHEDULE_END_MIN): _num_schema(PetkitTimeNumber, 0, 1439, 1),
        cv.Optional(CONF_DND_START_MIN): _num_schema(PetkitTimeNumber, 0, 1439, 1),
        cv.Optional(CONF_DND_END_MIN): _num_schema(PetkitTimeNumber, 0, 1439, 1),
    }
)


async def _new_num(cfg):
    # ESPHome 2025.12 requires these keyword-only args
    return await number.new_number(
        cfg,
        min_value=cfg[CONF_MIN_VALUE],
        max_value=cfg[CONF_MAX_VALUE],
        step=cfg[CONF_STEP],
    )


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    if CONF_LIGHT_BRIGHTNESS in config:
        cfg = config[CONF_LIGHT_BRIGHTNESS]
        n = await _new_num(cfg)
        cg.add(n.set_parent(parent))

    # Time kinds: 0=LIGHT_START, 1=LIGHT_END, 2=DND_START, 3=DND_END
    if CONF_LIGHT_SCHEDULE_START_MIN in config:
        cfg = config[CONF_LIGHT_SCHEDULE_START_MIN]
        n = await _new_num(cfg)
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(0))

    if CONF_LIGHT_SCHEDULE_END_MIN in config:
        cfg = config[CONF_LIGHT_SCHEDULE_END_MIN]
        n = await _new_num(cfg)
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(1))

    if CONF_DND_START_MIN in config:
        cfg = config[CONF_DND_START_MIN]
        n = await _new_num(cfg)
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(2))

    if CONF_DND_END_MIN in config:
        cfg = config[CONF_DND_END_MIN]
        n = await _new_num(cfg)
        cg.add(n.set_parent(parent))
        cg.add(n.set_kind(3))
