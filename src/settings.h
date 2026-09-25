/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>

struct Setting
{
	const char* name;
	bool value;
};

inline constexpr std::array g_defaultSettings{
	Setting{ "2dMode", true },
	Setting{ "ExperimentalMode", false },
	Setting{ "PassOnOglControllerTransformation", true },
	Setting{ "UseMultithreading", false },
	Setting{ "MoveUnusedPivots", false },
	Setting{ "DebugMode", false },
	Setting{ "SetInbetweenKeyframesMode", false }
};

using SettingsDesc = std::remove_const_t<decltype(g_defaultSettings)>;

// compile-time lookup of a setting's position in g_defaultSettings;
// an unknown name fails the build
consteval std::size_t settingIndex(std::string_view name)
{
	for (std::size_t i = 0; i < g_defaultSettings.size(); ++i)
	{
		if (name == g_defaultSettings[i].name)
			return i;
	}

	throw "unknown setting";
}

// reads all settings from the ini file, writing back defaults for missing ones
SettingsDesc loadSettings();
void storeSettings(const SettingsDesc& settings);

#endif
