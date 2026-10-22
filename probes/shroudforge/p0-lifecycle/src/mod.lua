-- This feature check is NOT a build/symbol guard for the native loader.
-- The loader must pass its independent safety gates before this mod is enabled.
runtime.require("runtime.lifecycle")

return {
    on_load = function()
        shroudforge.log.info("EC_P0_LIFECYCLE v0.0.1 loaded")
    end,
    on_unload = function()
        shroudforge.log.info("EC_P0_LIFECYCLE v0.0.1 unloaded")
    end,
}
