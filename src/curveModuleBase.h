/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */

#ifndef CURVEMODULEBASE_H
#define CURVEMODULEBASE_H

#include "moduleBase.h"
#include "utils.h"

#include <SceneCore/attribute/AT_AttrCmds.h>
#include <SceneCore/attribute/AT_BoolAttr.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/attribute/AT_PositionAttrCmds.h>


/*
 * Notes on curve and offset modules:
 * Offset modules' attributes are always set in fields coordiates, that includes the angle, e.g. a 45 degree angle is the diagonal of a 4:3 field
 * Curve modules' attributes are set in fields coordinates when indepentent. However when used in curve mode, they are using 4:4 field
*/

/*
 * The process of deciding when to set a keyframe can be considerable simplified if the JS script workaround ever becomes obsolete.
 * Checking whether the current value equals the previous one would no longer be necessary, neither would be the distinction beetween
 * 2DPath and Separate for keyframes that are not forced.
*/

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
		const bool isRest,
		const double frameNo = 1.0) const;

	double getChainRotation(const double frameNo) const;
	double getStaticChainRotation(const bool isRest) const;

protected:
	void readjustOffsets(CO_OrCommand& curMacro, const Math::Matrix4x4& changeMatrix);
	bool isParentKeyframe(const double frameNo) const;
	bool isComplexTransform() const {
		return m_freezeMatrixComplexity == MatrixComplexity::Complex
			|| m_freezeMatrixComplexity == MatrixComplexity::ScaleTranslationOnly;
	}
	void setMatrixComplexity(const MatrixComplexity complexity) { m_freezeMatrixComplexity = complexity; }

private:
	void initAttributes();

	template <typename ValueFunc>
	double getRotationImpl(const QLatin1StringView curveAttrKeyword,
		const QLatin1StringView offsetAttrKeyword,
		ValueFunc&& valFunc) const;

	AT_Position2dAttr* m_offsetAttr;
	AT_Position2dAttr* m_restingOffsetAttr;

	Math::Point2d m_prevOffset;
	MatrixComplexity m_freezeMatrixComplexity;

	bool m_isIndependent;
};

Math::Matrix4x4 recalculateIndependentHandleMatrix(const SC_SceneMetrics* sceneMetrics,
	Math::Matrix4x4 handleMatrix, const Math::Matrix4x4& changeMatrix);
#endif
