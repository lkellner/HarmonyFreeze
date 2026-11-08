/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef PKOMODULE_H
#define PKOMODULE_H

#include "pointModuleBase.h"



class PkoModule : public PointModuleBase
{
public:
	explicit PkoModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);

	void readjustSecondary() override;


private:
	enum class TransformationType : uint8_t
	{
		Simple,
		SingleFreeze,
		DoubleFreeze
	};

	Math::Matrix4x4 calculateChangeMatrix(const double frameNo) const;

	void processPivot(TransformPoint* point, CO_OrCommand& curMacro);
	void identifyTransformationType();
	KeyframeData generateKeyframeData(TransformPoint* point, const Math::Point3d position,
		const double frameNo, const bool isFirst) const;
	bool hasComplexPort1Parent() const;
	bool hasNoParentKeyframe(double frameNo) const;
	bool isComplexTransform() const { return m_freezeMatrixComplexity != MatrixComplexity::Simple; }; //This is different from curve module

	TransformationType m_transformationType = TransformationType::Simple;
};
#endif
