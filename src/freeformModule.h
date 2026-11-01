/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef FREEFORMMODULE_H
#define FREEFORMMODULE_H

#include <SceneCore/module/MO_Module.h>
#include <SceneCore/module/MO_SoftContext.h>
#include <SceneCore/module/MO_Port.h>

#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/attribute/AT_BoolAttr.h>
#include <SceneCore/module/MO_NetworkUtils.h>
#include "SceneCore/selectable/SLB_ManipContext.h"
#include <Util/command/CO_OrCommand.h>

#include "moduleBase.h"
#include "pointModuleBase.h"

class FreeformModule : public PointModuleBase
{
public:
	explicit FreeformModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);

private:
};

#endif
