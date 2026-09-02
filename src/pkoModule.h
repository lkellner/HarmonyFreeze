/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef PKOMODULE_H
#define PKOMODULE_H

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

enum class TransformationType : uint8_t
{
	Simple,
	SingleFreeze,
	DoubleFreeze
};


class PkoModule : public ModuleBase
{
public:
	explicit PkoModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);

	void readjustSecondary();


private:
	FrameRange getFrameRange() const override;

	Math::Matrix4x4 calculateChangeMatrix(double frameNo);

	void processPivot(AT_Position2dAttr* pivotAttr, QString pivotKeyword, CO_OrCommand& curMacro);
	void setStaticAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, CO_OrCommand& curMacro);
	void setAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, CO_OrCommand& curMacro, 
		KeyframeState keyframeState, double frameNo);
	void identifyTransformationType();
	KeyframeState generateKeyframeData(AT_Position2dAttr* attr, double frameNo, bool isFirst);
	bool hasComplexPort1Parent();
	bool hasNoParentKeyframe(double frameNo);
	bool isComplexTransform() { return m_freezeMatrixComplexity != MatrixComplexity::Simple; } //This is different from curve module
	void setComplexTransform(const MatrixComplexity complexity) { m_freezeMatrixComplexity = complexity; }

	AT_Position2dAttr* m_pivot01Attr;
	AT_Position2dAttr* m_pivot02Attr;
	AT_Position2dAttr* m_pivot03Attr;

	TransformationType m_transformationType = TransformationType::Simple;
	MatrixComplexity m_freezeMatrixComplexity;
	Math::Point3d m_prevPos;
};

#endif
