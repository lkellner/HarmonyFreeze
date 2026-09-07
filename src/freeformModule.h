/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef FREEFORMMODULE_H
#define FREEFORMMODULE_H

#include <cstdint>
#include <cstdio>

#include <SceneCore/module/MO_Module.h>
#include <SceneCore/module/MO_SoftContext.h>
#include <SceneCore/module/MO_Port.h>

#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/attribute/AT_BoolAttr.h>
#include <SceneCore/module/MO_NetworkUtils.h>
#include "SceneCore/selectable/SLB_CmdManipulator.h"
#include "SceneCore/selectable/SLB_ManipContext.h"
#include "SceneCore/attribute/AT_AttrCmds.h"
#include "SceneCore/attribute/AT_PositionAttrCmds.h"
#include <Util/command/CO_OrCommand.h>

#include "moduleBase.h"
#include "utils.h"

struct FreeformPoint
{
	QString name;

	AT_Position2dAttr* posAttr;
	AT_Position2dAttr* restingPosAttr;

	AT_DoubleAttr* rotAttr;
};

class FreeformModule : public ModuleBase
{
public:
	explicit FreeformModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);

	void readjustSecondary();


private:
	FrameRange getFrameRange() const override;

	void processPoint(FreeformPoint*, CO_OrCommand& curMacro);
	void setStaticAttributes(FreeformPoint* point, Math::Point3d position, Math::Point3d restingPos, double rotation, CO_OrCommand& curMacro);
	void setAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, 
		CO_OrCommand& curMacro, double frameNo);
	
	std::vector<std::unique_ptr<FreeformPoint>> m_freeformPoints;
};

#endif
