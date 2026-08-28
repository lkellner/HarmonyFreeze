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


class PkoModule : public ModuleBase
{
public:
	explicit PkoModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr,
			ModuleType moduleType);

	void readjustSecondary();


private:
	void setStaticAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, CO_OrCommand& curMacro);

	AT_Position2dAttr* m_pivot01Attr;
	AT_Position2dAttr* m_pivot02Attr;
	AT_Position2dAttr* m_pivot03Attr;
};

#endif
