/**
 ******************************************************************************
 * @file    AppConfigFields.cpp
 * @brief   The app's copy of the configuration contract in its app-manifest.json.
 ******************************************************************************
 */

#include "AppConfigFields.hpp"

namespace RawTimeConfig {

using SDK::AppConfig;

// Every value here must match Output/app-manifest.json exactly. CI checks it with
// validate_app_config.py --check <app-manifest.json> --check-bounds <this file>.
const AppConfig::Field kFields[] = {
    AppConfig::boolField("showBattery", true),
};

const size_t kFieldCount = sizeof(kFields) / sizeof(kFields[0]);

} // namespace RawTimeConfig
