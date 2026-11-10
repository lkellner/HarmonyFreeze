/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */



#ifndef CURVEMODULE_H
#define CURVEMODULE_H

#include "curveModuleBase.h"
#include "regionOfInfluence.h"
#include "utils.h"

class BezierHandle
{
public:
	enum class Kind : uint8_t
	{
		Rest,
		Animatable
	};

	struct BezierHandleInfo
	{
		double radius;
		double angle;
	};

	struct KeyframeData
	{
		bool length;
		bool orientation;
		bool isConstSegLength;
		bool isConstSegOrientation;
		bool isAdjustKeyframe;
		bool isForceKeyframe;
		bool isProcessFrame;
	};

	explicit BezierHandle(std::shared_ptr<FreezeManager> freezeManager,
		AT_DoubleAttr* lengthAttr,
		AT_DoubleAttr* orientationAttr,
		QString lengthAttrJS,
		QString orientationAttrJS,
		Kind kind,
		bool isIndependent);

	void recalculateStaticHandlePoint(const MO_Module* modulePtr,
		const Math::Matrix4x4& changeMatrix,
		CO_OrCommand& curMacro);

	void recalculateAnimatedHandlePoint(const MO_Module* modulePtr,
		const Math::Matrix4x4 changeMatrix,
		CO_OrCommand& curMacro,
		KeyframeData& kfData,
		const double frameNo);

	Kind kind() const { return m_kind; }

	bool isAnimatable() const { return m_kind == Kind::Animatable; }
	bool isRest() const { return m_kind == Kind::Rest; }

	const AT_DoubleAttr* getLengthAttr() const { return m_lengthAttr; }
	const AT_DoubleAttr* getOrientationAttr() const { return m_orientationAttr; }

private:
	BezierHandle::BezierHandleInfo getRecalculatedHandleInfo(const MO_Module* modulePtr,
		const Math::Matrix4x4& changeMatrix,
		const double length,
		const double angle) const;
	void updateKeyframeData(KeyframeData& kfData,
		const BezierHandleInfo handleInfo) const;
	std::shared_ptr<FreezeManager> m_freezeManager;

	AT_DoubleAttr* m_lengthAttr;
	AT_DoubleAttr* m_orientationAttr;

	QString m_lengthAttrJS;
	QString m_orientationAttrJS;

	double m_prevRadius;
	double m_prevAngle;

	Kind m_kind;
	bool m_isIndependent;
};

class CurveModule : public CurveModuleBase
{
public:
	explicit CurveModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr, ModuleType moduleType);

	void readjustSecondary() override;

private:
	void loadAttributes();

	BezierHandle::KeyframeData generateKeyframeData(const BezierHandle& bezierHandle,
		const double frameNo, 
		const bool isFirst) const;
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
