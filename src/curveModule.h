/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */



#ifndef CURVEMODULE_H
#define CURVEMODULE_H

#include "curveModuleBase.h"
#include "utils.h"

#include <SceneCore/attribute/AT_BoolAttr.h>
#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/module/MO_Module.h>
#include <SceneCore/module/MO_NetworkUtils.h>
#include <SceneCore/module/MO_Port.h>
#include <SceneCore/module/MO_SoftContext.h>
#include <SceneCore/selectable/SLB_CmdManipulator.h>
#include <SceneCore/selectable/SLB_ManipContext.h>

class CurveModule : public CurveModuleBase
{
public:
	explicit CurveModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr, ModuleType moduleType);

	void readjustSecondary() override;

private:
	void loadAttributes();

	BezKeyframeData generateKeyframeData(BezierHandle& bezierHandle, double frameNo, bool isFirst);

	FrameRange getFrameRange() const override;

	void readjustBezierHandles(CO_OrCommand& curMacro, Math::Matrix4x4 matrix);
	void readjustRegionOfInfluence(CO_OrCommand& curMacro, const Math::Matrix4x4& matrix);


	AT_DoubleAttr* m_restLength0Attr;
	AT_DoubleAttr* m_restLength1Attr;
	AT_DoubleAttr* m_restingOrientation0Attr;
	AT_DoubleAttr* m_restingOrientation1Attr;

	AT_DoubleAttr* m_length0Attr;
	AT_DoubleAttr* m_length1Attr;
	AT_DoubleAttr* m_orientation0Attr;
	AT_DoubleAttr* m_orientation1Attr;

	AT_DoubleAttr* m_longitudinalRadiusAttr;
	AT_DoubleAttr* m_longitudinalRadiusBeginAttr;
	AT_DoubleAttr* m_transversalRadiusAttr;
	AT_DoubleAttr* m_transversalRadiusRightAttr;
	AT_DoubleAttr* m_influenceFadeRadiusAttr;

	std::vector<BezierHandle> m_bezierHandles;
};

#endif
