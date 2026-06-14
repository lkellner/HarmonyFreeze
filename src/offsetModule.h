/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef OFFSETMODULE_H
#define OFFSETMODULE_H

#include "curveModuleBase.h"
#include "utils.h"

#include <SceneCore/attribute/AT_BoolAttr.h>
#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>

#include <SceneCore/module/MO_Module.h>
#include <SceneCore/module/MO_NetworkUtils.h>
#include <SceneCore/module/MO_Port.h>
#include <SceneCore/module/MO_SoftContext.h>

class OffsetModule : public CurveModuleBase
{
public:
	explicit OffsetModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr, ModuleType moduleType);

	void readjustSecondary() override;

protected:
	FrameRange getFrameRange() const override;

private:
	void loadAttributes();
	void readjustOrientation(CO_OrCommand& curMacro, const Math::Matrix4x4& changeMatrix);

	AT_DoubleAttr* m_restingOrientationAttr;
	AT_DoubleAttr* m_orientationAttr;
};

#endif
