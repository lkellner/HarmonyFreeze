/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef POINTMODULEBASE_H
#define POINTMODULEBASE_H

#include <SceneCore/module/MO_Module.h>
#include <SceneCore/module/MO_SoftContext.h>
#include <SceneCore/module/MO_Port.h>

#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/attribute/AT_BoolAttr.h>
#include <SceneCore/module/MO_NetworkUtils.h>
#include "SceneCore/selectable/SLB_ManipContext.h"
#include "SceneCore/attribute/AT_AttrCmds.h"
#include "SceneCore/attribute/AT_PositionAttrCmds.h"
#include <Util/command/CO_OrCommand.h>

#include "moduleBase.h"
#include "utils.h"

struct TransformPoint
{
	QString name;
	AT_Position2dAttr* posAttr;
	bool isAnimated;
};

class PointModuleBase : public ModuleBase
{
public:
	explicit PointModuleBase(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);

	void readjustSecondary() override;

protected:
	void addTransformPoint(std::unique_ptr<TransformPoint> point);


private:
	FrameRange getFrameRange() const override;

	void processPoint(TransformPoint*, CO_OrCommand& curMacro);
	void setStaticAttributes(TransformPoint* point, Math::Point3d position,
		CO_OrCommand& curMacro) const;
	void setAttributes(TransformPoint* point, Math::Point3d position, CO_OrCommand& curMacro,
		double frameNo) const;
	
	std::vector<std::unique_ptr<TransformPoint>> m_transformPoints;
};

#endif
