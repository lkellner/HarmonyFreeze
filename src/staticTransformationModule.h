/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef STATICTRANSFORMATIONMODULE_H
#define STATICTRANSFORMATIONMODULE_H

#include "moduleBase.h"

#include <SceneCore/attribute/AT_DoubleAttr.h>
#include <SceneCore/attribute/AT_Position3dAttr.h>
#include <SceneCore/attribute/AT_Rotation3dAttr.h>
#include <SceneCore/attribute/AT_Scale3dAttr.h>

class StaticTransformationModule : public ModuleBase
{
public:
	explicit StaticTransformationModule(std::shared_ptr<FreezeManager> freezeManager,
			MO_Module* modulePtr, ModuleType moduleType);

	void readjustSecondary() override;

private:
	MatrixParameters convertMatrixToParams(const Math::Matrix4x4& matrix) const;

	Math::Matrix4x4 getLocalMatrix(const double frameNo) const;

	void setMatrixAsAttributes(const Math::Matrix4x4& matrix) const;
	void setMatrixAsAttributesJS(const Math::Matrix4x4& matrix) const;

	AT_Position3dAttr* m_positionAttr;
	AT_Scale3dAttr* m_scaleAttr;
	AT_Rotation3dAttr* m_rotationAttr;
	AT_DoubleAttr* m_skewXAttr;
	AT_DoubleAttr* m_skewYAttr;
	AT_DoubleAttr* m_skewZAttr;
	AT_BoolAttr* m_invertedAttr;
};


#endif
