/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef FREEZETRANSFORMATION_H
#define FREEZETRANSFORMATION_H

#include "curveModule.h"
#include "drawingTransformationModule.h"
#include "elementModule.h"
#include "offsetModule.h"
#include "oglControllerModule.h"
#include "pegModule.h"
#include "staticTransformationModule.h"
#include "utils.h"

#include <Util/pluginmanager/PLUG_Services.h>

#include <QObject>
#include <QString>

class GR_CompositeVectorDrawingObj;
class CA_CelPtr;

using ModuleWrappers = std::vector<std::unique_ptr<ModuleBase>>;


class FreezeResponder : public QObject
{
	Q_OBJECT

public slots:
	void onActionFreezeTransformation();
};

#endif

