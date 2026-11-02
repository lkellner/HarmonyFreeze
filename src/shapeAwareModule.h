/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef SHAPEAWAREMODULE_H
#define SHAPEAWAREMODULE_H

#include "pointModuleBase.h"

class ShapeAwareModule : public PointModuleBase
{
public:
	explicit ShapeAwareModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);
	void readjustSecondary() override;

private:
	void processeInputModuls() const;
};

#endif
