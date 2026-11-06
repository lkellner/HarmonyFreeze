/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef FREEZETRANSFORMATION_H
#define FREEZETRANSFORMATION_H

#include "moduleBase.h"

#include <Util/pluginmanager/PLUG_Services.h>

#include <QObject>


class GR_CompositeVectorDrawingObj;
class CA_CelPtr;

using ModuleWrappers = std::vector<std::unique_ptr<ModuleBase>>;


class FreezeResponder : public QObject
{
	Q_OBJECT

public slots:
	void onActionFreezeTransformation();
	void onActionShowSettings();
};

#endif

