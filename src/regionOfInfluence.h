/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef REGIONOFINFLUENCE_H
#define REGIONOFINFLUENCE_H

#include "utils.h"
#include "freezeManager.h"

#include "SceneCore/attribute/AT_AttrCmds.h"
#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/module/MO_Module.h>
#include <Util/command/CO_OrCommand.h>


class RegionOfInfluence
{
public:
	explicit RegionOfInfluence(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr);

	void readjustSecondary(CO_OrCommand& curMacro, const Math::Matrix4x4& matrix);

private:
	FreezeManager* getFreezeManagerPtr() { return m_freezeManager.get(); };
	MO_Module* getModulePtr() { return m_modulePtr; };
	MO_Module* m_modulePtr;
	std::shared_ptr<FreezeManager> m_freezeManager;

	AT_DoubleAttr* m_longitudinalRadiusAttr;
	AT_DoubleAttr* m_longitudinalRadiusBeginAttr;
	AT_DoubleAttr* m_transversalRadiusAttr;
	AT_DoubleAttr* m_transversalRadiusRightAttr;
	AT_DoubleAttr* m_influenceFadeAttr;
};
#endif
