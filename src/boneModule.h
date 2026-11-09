/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef BONEMODULE_H
#define BONEMODULE_H

#include "moduleBase.h"
#include "regionOfInfluence.h"
#include "utils.h"

#include <SceneCore/attribute/AT_PositionAttrCmds.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>


bool hasOffset(const MO_Node* node);

class BoneModule : public ModuleBase
{
public:
	struct KeyframeData;
	struct BoneData;

	explicit BoneModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType);

	void readjustSecondary() override;

private:
	FrameRange getFrameRange() const override;

	void processAttributeSet(CO_OrCommand& curMacro, const bool isRest);
	void setStaticAttributes(const BoneData boneData, CO_OrCommand& curMacro,
		const bool isRest) const;
	void setAttributes(const BoneData boneData, CO_OrCommand& curMacro,
		const double frameNo, const bool isFirst);

	double getStaticChainRotation(const bool isRest) const;
	double getChainRotation(const double frameNo) const;

	BoneModule::KeyframeData generateKeyframeData(const double frameNo, const bool isFirst) const;

	bool isParentKeyframe(const double frameNo) const;
	bool hasBoneParents() const;
	bool hasFieldsOrientation() const;

	bool isComplexTransform() const {
		return m_freezeMatrixComplexity == MatrixComplexity::Complex
			|| m_freezeMatrixComplexity == MatrixComplexity::ScaleTranslationOnly;
	}

	template <typename ValueFunc>
	double getRotationImpl(const QLatin1StringView orientationKeyword, ValueFunc&& valFunc) const;

	std::unique_ptr<RegionOfInfluence> m_regionOfInfluence;

	AT_Position2dAttr* m_restOffsetAttr;
	AT_DoubleAttr* m_restRadiusAttr;
	AT_DoubleAttr* m_restLengthAttr;
	AT_DoubleAttr* m_restOrientationAttr;

	AT_Position2dAttr* m_offsetAttr;
	AT_DoubleAttr* m_radiusAttr;
	AT_DoubleAttr* m_lengthAttr;
	AT_DoubleAttr* m_orientationAttr;

	MatrixComplexity m_freezeMatrixComplexity;

	Math::Point3d m_prevPosition;
	double m_prevLength;
	double m_prevOrientation;
	double m_prevRadius;
};

struct BoneModule::BoneData
{
	Math::Point3d position;
	double orientation;
	double radius;
	double length;
};

struct BoneModule::KeyframeData
{
	KeyframeState position;
	KeyframeState length;
	KeyframeState orientation;
	KeyframeState radius;
	bool positionSeg2D;
	bool positionSegX;
	bool positionSegY;
	bool lengthSeg;
	bool orientationSeg;
	bool radiusSeg;
};

#endif
