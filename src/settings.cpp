#include "settings.h"

#include <QSettings>

SettingsDesc loadSettings()
{
	QSettings file(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("HarmonyFreeze"), QStringLiteral("HarmonyFreeze"));

	SettingsDesc settings = g_defaultSettings;
	for (auto& setting : settings)
	{
		const QLatin1String name(setting.name);

		// ensure it gets written back to the file
		if (!file.contains(name))
			file.setValue(name, setting.value);

		setting.value = file.value(name).toBool();
	}

	return settings;
}


void storeSettings(const SettingsDesc& settings)
{
	QSettings file(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("HarmonyFreeze"), QStringLiteral("HarmonyFreeze"));

	for (const auto& setting : settings)
	{
		file.setValue(QLatin1String(setting.name), setting.value);
	}
}
