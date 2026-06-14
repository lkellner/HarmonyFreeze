/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef CURVEMODULEBASE_H
#define CURVEMODULEBASE_H

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

/*
 * Notes on curve and offset modules:
 * Offset modules' attributes are always set in fields coordiates, that includes the angle, e.g. a 45 degree angle is the diagonal of a 4:3 field
 * Curve modules' attributes are set in fields coordinates when indepentent. However when used in curve mode, they are using 4:4 field
*/

enum class KeyframeState : uint8_t
{
	Keyframe,
	PossibleKeyframe,
	NoKeyframe
};

//TODO: is it possible to make this an inner class?
struct BezKeyframeData
{
	KeyframeState length;
	KeyframeState orientation;
};

class BezierHandle
{
public:
	enum class Kind : uint8_t
	{
		Rest,
		Animatable
	};

	explicit BezierHandle(std::shared_ptr<FreezeManager> freezeManager,
			AT_DoubleAttr* lengthAttr,
			AT_DoubleAttr* orientationAttr,
			QString lengthAttrJS,
			QString orientationAttrJS,
			Kind kind);

	void recalculateStaticHandlePoint(MO_Module* modulePtr,
			const Math::Matrix4x4& changeMatrix,
			CO_OrCommand& curMacro,
			bool isIndependent);

	void recalculateAnimatedHandlePoint(MO_Module* modulePtr,
			Math::Matrix4x4 changeMatrix,
			CO_OrCommand& curMacro,
			const BezKeyframeData& kfData,
			double frameNo,
			bool isIndependent);

	Kind kind() const { return m_kind; }

	bool isAnimatable() const { return m_kind == Kind::Animatable; }
	bool isRest() const { return m_kind == Kind::Rest; }

	const AT_DoubleAttr* getLengthAttr() { return m_lengthAttr; }
	const AT_DoubleAttr* getOrientationAttr() { return m_orientationAttr; }

private:
	std::shared_ptr<FreezeManager> m_freezeManager;

	AT_DoubleAttr* m_lengthAttr;
	AT_DoubleAttr* m_orientationAttr;

	QString m_lengthAttrJS;
	QString m_orientationAttrJS;

	double m_prevRadius;
	double m_prevAngle;

	Kind m_kind;
};

struct BezierHandleInfo
{
	double radius;
	double angle;
};

class CurveModuleBase : public ModuleBase
{
public:
	explicit CurveModuleBase(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);

	bool isIndependent() const { return m_isIndependent; }

	FrameRange getFrameRange() const override;

	Math::Matrix4x4 adjustDependentMatrix(const Math::Matrix4x4 matrix,
			const bool isAnimated,
			const double frameNo = 1.0,
			const bool isRest = false) const;

	double getChainRotation(const double frameNo) const;
	double getStaticChainRotation(const bool isRest) const;

protected:
	void readjustOffsets(CO_OrCommand& curMacro, const Math::Matrix4x4& changeMatrix);
	bool isParentKeyframe(double frameNo);
	bool isComplexTransform() { return m_freezeMatrixComplexity != MatrixComplexity::Simple; }
	void setComplexTransform(const MatrixComplexity complexity) { m_freezeMatrixComplexity = complexity; }

private:
	void initAttributes();

	template <typename ValueFunc>
	double getRotationImpl(const QString& curveAttrKeyword,
			const QString& offsetAttrKeyword,
			ValueFunc&& valFunc) const;

	AT_Position2dAttr* m_offsetAttr;
	AT_Position2dAttr* m_restingOffsetAttr;

	Math::Point2d m_prevOffset;
	MatrixComplexity m_freezeMatrixComplexity;

	bool m_isIndependent;
};

BezierHandleInfo getRecalculatedHandleInfo(SC_SceneMetrics* sceneMetrics,
		const Math::Matrix4x4& changeMatrix,
		double length,
		double angle,
		bool isIndependent);

#endif
