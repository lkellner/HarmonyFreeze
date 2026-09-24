/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef BONEMODULE_H
#define BONEMODULE_H

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

class BoneModule : public ModuleBase
{
public:
	explicit BoneModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);

	void readjustSecondary();


private:
	FrameRange getFrameRange() const override;
	void setStaticAttributes(Math::Point3d restPosition, Math::Point3d position,
		double restLength, double length, double restOrientation,
		double orientation, CO_OrCommand& curMacro);

	AT_Position2dAttr* m_restOffsetAttr;
	AT_DoubleAttr* m_restRadiusAttr;
	AT_DoubleAttr* m_restLengthAttr;
	AT_DoubleAttr* m_restOrientationAttr;

	AT_Position2dAttr* m_offsetAttr;
	AT_DoubleAttr* m_radiusAttr;
	AT_DoubleAttr* m_lengthAttr;
	AT_DoubleAttr* m_orientationAttr;

	AT_DoubleAttr* m_longitudinalRadiusAttr;
	AT_DoubleAttr* m_longitudinalRadiusBeginAttr;
	AT_DoubleAttr* m_transversalRadiusAttr;
	AT_DoubleAttr* m_transversalRadiusRightAttr;
	AT_DoubleAttr* m_influenceFadeAttr;

	/*
	void processPoint(FreeformPoint*, CO_OrCommand& curMacro);
	void setStaticAttributes(FreeformPoint* point, Math::Point3d position, Math::Point3d restingPos,
		CO_OrCommand& curMacro);
	void setAttributes(FreeformPoint* point, Math::Point3d position, CO_OrCommand& curMacro,
		double frameNo);
	*/
};

#endif
