/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef FREEFORMMODULE_H
#define FREEFORMMODULE_H

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
