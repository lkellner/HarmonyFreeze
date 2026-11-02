/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef QUADMAPMODULE_H
#define QUADMAPMODULE_H

#include "pointModuleBase.h"

class QuadmapModule : public PointModuleBase
{
public:
	explicit QuadmapModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);

private:
};

#endif
