/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright 2026 Laura Kellner
 */


#ifndef ELEMENTMODULE_H
#define ELEMENTMODULE_H

#include "moduleBase.h"
#include "utils.h"

#include <GraphicCore/GraphicLib/GR_DrawingAccess.h>
#include <SceneCore/attribute/AT_BoolAttr.h>
#include <SceneCore/module/MO_PortTransform.h>

class BitmapListener : public GR_Listener
{
public:
	using GR_Listener::GR_Listener;
	void Notify(const GR_VectorDrawingObj& i_drawing, const UT_Node& i_node, const Event& i_message);
};

class ElementModule : public ModuleBase
{
public:
	explicit ElementModule(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr, ModuleType moduleType);

	void modifyTimings() const;
	void readjustSecondary() override;
	void readjustTertiary() const override;
	void setDrawingPivots() const;
	void setChangeMatrix(const Math::Matrix4x4 matrix) { m_changeMatrix = matrix; };
	Math::Matrix4x4 getAlignmentMatrix() const;

private:
	void saveDrawingPivots();
	void recalculateDrawingPivots(const Math::Matrix4x4& matrix);
	void updateDrawingPivotConversionStatus();

	void applyChangeMatrix(const CA_CelPtr celPtr, Math::Matrix4x4 matrix) const;
	void processBitmaps(const CA_CelPtr celPtr, const GR_CompositeVectorDrawingObj* compDrawing, const Math::Matrix4x4& matrix) const;
	void processVectorData(const CA_CelPtr celPtr, const GR_CompositeVectorDrawingObj* compDrawing,const Math::Matrix4x4& matrix) const;
	void processVectorDrawingLayers(GR_DrawingAccess& drawingAccess, const Math::Matrix4x4& matrix, GR_LayerNode* curLayerNode) const;
	void transformColor(GR_VectorStroke* ogStroke, GR_StrokeData& data, std::map<GR_Color*, GR_StrokeData::GR_ColorPtr_t>& colorLookUp,
			const Math::Matrix4x4& matrix, const GR_StrokeData::StrokeSide sideData, const GR_VectorStroke::StrokeSide sideVector) const;

	std::vector<CelInfo> m_drawingPivots;

	bool m_isConvertDrawingPivots;
	Math::Matrix4x4 m_changeMatrix;
};


#endif
