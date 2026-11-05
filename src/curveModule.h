/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */



#ifndef CURVEMODULE_H
#define CURVEMODULE_H

#include "curveModuleBase.h"
#include "regionOfInfluence.h"
#include "utils.h"

class CurveModule : public CurveModuleBase
{
public:
	explicit CurveModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr, ModuleType moduleType);

	void readjustSecondary() override;

private:
	void loadAttributes();

	BezKeyframeData generateKeyframeData(const BezierHandle& bezierHandle,
		const double frameNo, const bool isFirst) const;

	FrameRange getFrameRange() const override;

	void readjustBezierHandles(CO_OrCommand& curMacro, const Math::Matrix4x4 matrix);

	std::unique_ptr<RegionOfInfluence> m_regionOfInfluence;

	AT_DoubleAttr* m_restLength0Attr;
	AT_DoubleAttr* m_restLength1Attr;
	AT_DoubleAttr* m_restingOrientation0Attr;
	AT_DoubleAttr* m_restingOrientation1Attr;

	AT_DoubleAttr* m_length0Attr;
	AT_DoubleAttr* m_length1Attr;
	AT_DoubleAttr* m_orientation0Attr;
	AT_DoubleAttr* m_orientation1Attr;

	std::vector<BezierHandle> m_bezierHandles;
};

#endif
