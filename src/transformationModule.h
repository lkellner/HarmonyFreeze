/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef TRANSFORMATIONMODULE_H
#define TRANSFORMATIONMODULE_H

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

#include <Util/pluginmanager/PLUG_Scripting.h>

#include <unordered_map>

class TransformationModule : public ModuleBase
{
public:
	struct KeyframeData;

	explicit TransformationModule(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr,
			ModuleType moduleType, bool isFreeze);

	void freeze();
	void readjustSecondary() override;

	Math::Matrix4x4 getLocalMatrixNoPivot(const double frameNo) const;
	Math::Matrix4x4 getLocalMatrix(const double frameNo) const;

	FrameRange getFrameRange() const override;

protected:
	AT_Position3dAttr* getPivotAttr() const { return m_pivotAttr; };
	void setPivot(CO_OrCommand& curMacro);
	void setPivotJS() const;

	Math::Matrix4x4 getStaticLocalMatrixNoPivot();

private:

	Math::Angle3d getFreezeAngle(bool isStatic, const double frameNo = 1) const;

	KeyframeData generateKeyframeData(const MatrixParameters& params, const double frameNo, const bool isFirst) const;
	MatrixParameters convertMatrixToParams(const Math::Matrix4x4 matrix, const Math::Angle3d angle) const;

	void processMatrix(const Math::Matrix4x4& matrix, CO_OrCommand& curMacro, const double frameNo, const bool isFirst);
	void processStaticMatrix(const Math::Matrix4x4& matrix, CO_OrCommand& curMacro);
	void setAttributes(const MatrixParameters& params, const KeyframeData& kfData, CO_OrCommand& curMacro, const double frameNo);
	void setStaticAttributes(const MatrixParameters& params, CO_OrCommand& curMacro);
	void setAttributesJS(const MatrixParameters& params, const KeyframeData& kfData, const double frameNo) const;
	void setStaticAttributesJS(const MatrixParameters& params) const;

	virtual void setPositionJS(const KeyframeData & kfData, const MatrixParameters& params, const double frameNo) const = 0;
	virtual void setStaticPositionJS(const MatrixParameters & params) const = 0;

	virtual void processPivotAttribute(CO_OrCommand& curMacro) = 0;
	void initDrawingPivotSources();
	void initElementDrawingPivotSources();
	void initPegDrawingPivotSources();
	void initPivots();
	void recalculateAllPivots();
	void saveWorldCombinedPivots();

	MatrixParameters m_prevFrameParams;

	std::vector<CelInfo> m_individualPivots;
	std::vector<MO_Module*> m_drawingPivotSources;
	std::unordered_map<int, Math::Point3d> m_combinedPivots;

	AT_Position3dAttr* m_positionAttr;
	AT_Scale3dAttr* m_scaleAttr;
	AT_Rotation3dAttr* m_rotationAttr;
	AT_DoubleAttr* m_skewAttr;
	AT_Position3dAttr* m_pivotAttr;

	Math::Point3d m_pivotAttrSurrogate;
	Math::Point3d m_pivotOffset;

	MatrixComplexity m_freezeMatrixComplexity;

	bool m_is3D;
	bool m_isFreeze;
};


struct TransformationModule::KeyframeData
{
	bool tx = false;
	bool ty = false;
	bool tz = false;
	bool sx = false;
	bool sy = false;
	bool sz = false;
	bool ax = false;
	bool ay = false;
	bool az = false;
	bool skew = false;

	bool isPosSeparate = false;
	bool isRotSeparate = false;
	bool isScaleSeparate = false;
};

#endif
