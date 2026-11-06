/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef POINTMODULEBASE_H
#define POINTMODULEBASE_H

#include "moduleBase.h"

#include <SceneCore/attribute/AT_PositionAttrCmds.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <Util/command/CO_OrCommand.h>


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

	void processPoint(const TransformPoint*, CO_OrCommand& curMacro) const;
	void setStaticAttributes(const TransformPoint* point, Math::Point3d position,
		CO_OrCommand& curMacro) const;
	void setAttributes(const TransformPoint* point, Math::Point3d position, CO_OrCommand& curMacro,
		const double frameNo) const;
	
	std::vector<std::unique_ptr<TransformPoint>> m_transformPoints;
};

#endif
