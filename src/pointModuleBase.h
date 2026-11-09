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
	struct KeyframeData;

	void setStaticAttributes(const TransformPoint* point, Math::Point3d position,
		CO_OrCommand& curMacro) const;
	void setAttributes(const TransformPoint* point, Math::Point3d position,
		CO_OrCommand& curMacro, const double frameNo, const bool isFirst) const;
	virtual KeyframeData generateKeyframeData(const TransformPoint* point, const Math::Point3d position,
		const double frameNo, const bool isFirst) const;

	void addTransformPoint(std::unique_ptr<TransformPoint> point);
	void setMatrixComplexity() { m_freezeMatrixComplexity = getFreezeManagerPtr()->getMatrixComplexity(false); };
	FrameRange getFrameRange() const override;

	std::vector<std::unique_ptr<TransformPoint>> m_transformPoints;
	MatrixComplexity m_freezeMatrixComplexity;
	Math::Point3d m_prevPos;

private:
	void processPoint(const TransformPoint* point, CO_OrCommand& curMacro);
	bool isComplexTransform() const { return m_freezeMatrixComplexity == MatrixComplexity::Complex; };
};

struct PointModuleBase::KeyframeData
{
	bool posX = false;
	bool posY = false;
	bool pos2D = false;

	bool isConstSegX = false;
	bool isConstSegY = false;
	bool isConstSeg2D = false;

	bool isSeparate = false;
};
#endif
