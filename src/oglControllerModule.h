/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef OGLCONTROLLERMODULE_H
#define OGLCONTROLLERMODULE_H


#include "moduleBase.h"
#include "utils.h"

#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/attribute/AT_Path3dAttr.h>
#include <SceneCore/attribute/AT_Position3dAttr.h>
#include <SceneCore/attribute/AT_QuaternionPathAttr.h>
#include <SceneCore/attribute/AT_Rotation3dAttr.h>
#include <SceneCore/attribute/AT_Scale3dAttr.h>
#include <SceneCore/module/MO_Module.h>
#include <SceneCore/module/MO_Port.h>

class OglControllerModule : public ModuleBase
{
public:
	explicit OglControllerModule(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr, ModuleType moduleType);

	void readjustSecondary() override;

private:
	FrameRange getFrameRange() const override;

	void setStaticAttributes(Math::Point3d position, Math::Point3d textPosition,
		double size,double textSize, CO_OrCommand& curMacro);
	void setAttributes(Math::Point3d position, Math::Point3d textPosition,
		double size, double textSize, CO_OrCommand& curMacro, const double frameNo);

	void clampControllerValues(Math::Point3d& position, Math::Point3d& textPosition, double& size, double& textSize) const;

	Math::Matrix4x4 getIncomingMatrix(unsigned int port, double frameNo, bool useDeformationuseDeformation) const override;
	Math::Matrix4x4 constructLocalMatrix(Math::Point3d position,const double size) const;
	Math::Matrix4x4 getLocalMatrix(const double frameNo) const;

	double getScaleFactor(const Math::Matrix4x4 & matrix, const double frameNo) const;
	bool isDoubleFlipped(const Math::Matrix4x4& matrix) const;

	void initInputModules();
	void modifyInputModules(Math::Matrix4x4 inputChangeMatrix);

	std::vector<MO_Module*> m_inputModules;

	AT_Position3dAttr* m_positionAttr;
	AT_Position3dAttr* m_textPositionAttr;

	AT_DoubleAttr* m_sizeAttr;
	AT_DoubleAttr* m_textSizeAttr;

	bool m_isPassOnToDrawings;
};

#endif
