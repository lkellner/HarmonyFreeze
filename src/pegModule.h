/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef PEGMODULE_H
#define PEGMODULE_H

#include "transformationModule.h"

class PegModule : public TransformationModule
{
public:
	using TransformationModule::TransformationModule;

private:
	void setPositionJS(const KeyframeData& kfData, const MatrixParameters& params, const double frameNo) const override;
	void setStaticPositionJS(const MatrixParameters& params) const override;
	void processPivotAttribute(CO_OrCommand& curMacro) override;
};

#endif
