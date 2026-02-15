/**
 * @file sdk_registration.cpp
 * @brief Registers SDK example plugins with the PluginRegistry
 */

#include "sdk/sdk_example_plugins.hpp"
#include "engine/plugins/plugin_registry.hpp"

namespace Beam {

// Register SDK example plugins
REGISTER_BEAM_PLUGIN(Examples::ExampleGain)
REGISTER_BEAM_PLUGIN(Examples::ExampleFilter)

} // namespace Beam
