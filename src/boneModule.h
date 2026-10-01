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

struct BoneKeyframeData
{
	KeyframeState position;
	KeyframeState length;
	KeyframeState orientation;
	KeyframeState radius;
};

bool hasOffset(const MO_Node* node);

class BoneModule : public ModuleBase
{
public:
	explicit BoneModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType);

	void readjustSecondary();


private:
	FrameRange getFrameRange() const override;

	void processAttributeSet(CO_OrCommand& curMacro, bool isRest);
	void setStaticAttributes(const Math::Point3d position, const double radius, const double length,
		const double orientation, CO_OrCommand& curMacro, const bool isRest);
	void setAttributes(const Math::Point3d position, const double orientation,const double radius,
		const double length, CO_OrCommand& curMacro, const double frameNo, const bool isFirst);

	double getStaticChainRotation(const bool isRest) const;
	double getChainRotation(const double frameNo) const;

	void readjustRegionOfInfluence(CO_OrCommand& curMacro, const Math::Matrix4x4& matrix);

	void setMatrixComplexity(const MatrixComplexity complexity) { m_freezeMatrixComplexity = complexity; }
	BoneKeyframeData generateKeyframeData(const double frameNo, const bool isFirst);

	bool isParentKeyframe(const double frameNo) const;
	bool hasBoneParents() const;
	bool hasFieldsOrientation() const;

	bool isComplexTransform() const {
		return m_freezeMatrixComplexity == MatrixComplexity::Complex
			|| m_freezeMatrixComplexity == MatrixComplexity::ScaleTranslationOnly;
	}

	template <typename ValueFunc>
	double getRotationImpl(const QString& orientationKeyword, ValueFunc&& valFunc) const;

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

	MatrixComplexity m_freezeMatrixComplexity;

	Math::Point3d m_prevPosition;
	double m_prevLength;
	double m_prevOrientation;
	double m_prevRadius;
};
#endif
