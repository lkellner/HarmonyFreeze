#include "elementModule.h"

#include <GraphicCore/ColorManagerLib/CM_Texture.h>

#include <GraphicCore/GraphicLib/GR_ColorDict.h>
#include <GraphicCore/GraphicLib/GR_BitmapLayer.h>
#include <GraphicCore/GraphicLib/GR_StrokeAccess.h>
#include <GraphicCore/GraphicLib/GR_BitmapAccess.h>
#include <GraphicCore/GraphicLib/GR_ColorTransform.h>

// workaround for 24.1 SDK
namespace Assertion
{
	void AssertFunction(char const*, char const*, char const*, int, bool) {}
}

void BitmapListener::Notify(const GR_VectorDrawingObj&, const UT_Node& i_node, const Event& i_message)
{
	printf("notified node address: %p\n", &i_node);
	printf("type: %d \n", i_message.m_type);
}


ElementModule::ElementModule(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr, ModuleType moduleType)
	: ModuleBase(freezeManager, modulePtr, moduleType)
	, m_isConvertDrawingPivots(true)
{
	//Drawing pivots need to be saved regardless of if they're being used or not, as it is possible that
	//there is another clone that might use them
	saveDrawingPivots();
}


void ElementModule::modifyTimings()
{
	const std::vector<CelInfo> celInfoVec = getElementTimings(getModulePtr());

	for (const auto& cel : celInfoVec)
	{
		applyChangeMatrix(cel.celPtr, m_changeMatrix);
	}
}


void ElementModule::processBitmaps(GR_CompositeVectorDrawingObj* compDrawing,const Math::Matrix4x4& matrix) const
{
	//This part of the code is currently only available in experimental mode as there hasn't been found
	//a way to undo bitmap transformations for now.

	if (!getFreezeManagerPtr()->isExperimentalMode())
		return;

	for (int i = 0; i <= 3; i++)
	{
		GR_VectorDrawingObj* drawingLayer = compDrawing->GetArt(i);

		if (!drawingLayer->hasBitmapLayers())
			continue;

		GR_BitmapAccess bitmapAccess;
		bitmapAccess.SetCurrentDrawing(drawingLayer);
		bitmapAccess.beginOperations();

		GR_BitmapNode* curBitmapNode = drawingLayer->topBitmapNode();
		GR_BitmapNode* prevBitmapNode = nullptr;

		while (curBitmapNode)
		{
			prevBitmapNode = curBitmapNode->previousBitmapNode();

			GR_BitmapLayer& bitmapLayer = curBitmapNode->bitmapLayer();

			bitmapAccess.transformBitmap(&bitmapLayer, matrix.getTransform2d());


			curBitmapNode = prevBitmapNode;
		}

		bitmapAccess.endOperations();
	}
}


void ElementModule::applyChangeMatrix(CA_CelPtr celPtr, Math::Matrix4x4 matrix) const
{
	if (!celPtr.isValid())
		return;


	CELVEC_Tbd* tbd = getPermTbd(celPtr);


	if (!tbd)
		return;


	matrix = getVectorModificationMatrix(getModulePtr()->sceneMetrics(), matrix, tbd);


	GR_CompositeVectorDrawingObj* compDrawing = tbd->getDrawingObject();

	if (!compDrawing)
		return;

	processVectorData(celPtr, compDrawing, matrix);

	processBitmaps(compDrawing, matrix);
}


void ElementModule::transformColor(GR_VectorStroke* ogStroke, GR_StrokeData & data, std::map<GR_Color*, GR_StrokeData::GR_ColorPtr_t> & colorLookUp,
	const Math::Matrix4x4& matrix, GR_StrokeData::StrokeSide sideData, const GR_VectorStroke::StrokeSide sideVector) const
{
	GR_Color* color = ogStroke->GetColor(sideVector);

	if (color)
	{
		if (colorLookUp.count(color))
		{
			//An updated colour already exists, need to use it, so that colours remain shared between paths
			data.Color(sideData) = colorLookUp[color];
		}
		else
		{
			//No updated colour exists yet
			data.Color(sideData) = GR_Color::CopyMovedVersion(ogStroke->GetColor(sideVector), matrix.getTransform2d());
			colorLookUp[color] = data.Color(sideData);
		}
	}
	else
	{
		data.Color(sideData) = color;
	}
}


void ElementModule::processVectorDrawingLayers(GR_DrawingAccess &drawingAccess,const Math::Matrix4x4& matrix, GR_LayerNode* curLayerNode) const
{
	if (!curLayerNode)
		return;

	GR_Layer& curLayer = curLayerNode->GetLayer();

	std::map<GR_Color*, GR_StrokeData::GR_ColorPtr_t> colorLookUp;

	for (GR_Layer::StrokeConstIterator strokeIt = curLayer.GetStrokeBegin(); strokeIt != curLayer.GetStrokeEnd(); ++strokeIt)
	{
		GR_VectorStroke* ogStroke = curLayer.FindStroke(*strokeIt);

		if (!ogStroke)
			continue;

		GR_StrokeData newData;

		newData.BezierPath() = ogStroke->GetBezierPath() * matrix.getTransform2d();

		transformColor(ogStroke, newData, colorLookUp, matrix, GR_StrokeData::STROKE_LEFT, GR_VectorStroke::STROKE_LEFT);
		transformColor(ogStroke, newData, colorLookUp, matrix, GR_StrokeData::STROKE_RIGHT, GR_VectorStroke::STROKE_RIGHT);

		newData.LineStyle() = ogStroke->GetLineStyle();
		newData.CopyThicknessBinder(ogStroke->thicknessBinder());

		drawingAccess.Erase(ogStroke);
		drawingAccess.Insert(newData, &curLayer);
	}
}


void ElementModule::processVectorData(const CA_CelPtr celPtr, GR_CompositeVectorDrawingObj* compDrawing,const Math::Matrix4x4& matrix) const
{
	GR_DrawingAccess drawingAccess;
	GR_ColorDict dict;

	drawingAccess.SetColorDict(&dict);

	for (int i = 0; i <= 3; i++)
	{
		GR_VectorDrawingObj* drawingLayer = compDrawing->GetArt(i);

		if (!drawingLayer)
			continue;

		drawingAccess.SetCurrentDrawing(drawingLayer);
		SDK_Drawing::ChangeScope changeScope;
		changeScope.listenForChanges(drawingAccess, celPtr, i);

		drawingAccess.BeginOperations();

		if (drawingLayer->HasLayers())
		{
			for (GR_LayerNode* curLayerNode = drawingLayer->GetBottomLayerNode(); curLayerNode; curLayerNode = curLayerNode->GetNextLayerNode())
			{
				processVectorDrawingLayers(drawingAccess, matrix, curLayerNode);
			}
		}

		if (drawingLayer->HasTextLayers())
		{
			for (GR_TextNode* curTextNode = drawingLayer->GetBottomTextNode(); curTextNode; curTextNode = curTextNode->GetNextTextNode())
			{
				GR_TextLayer& textLayer = curTextNode->GetTextLayer();
				textLayer.transform(matrix.getTransform2d());
			}
		}

		drawingAccess.EndOperations();
		changeScope.stopListeningForChanges(drawingAccess);
	}
}


void ElementModule::updateDrawingPivotConversionStatus()
{
	int curId = getElementId(getModulePtr());
	QString layerAttr = getLayerAttr(getModulePtr());

	FreezeManager* fm = getFreezeManagerPtr();

	if (!fm->isMoveUnusedPivots() && !fm->getDrawingPivotStatus(curId, layerAttr))
		m_isConvertDrawingPivots = false;
}


void ElementModule::readjustSecondary()
{
	updateDrawingPivotConversionStatus();

	FreezeManager* fm = getFreezeManagerPtr();
	m_changeMatrix = fm->getFreezeMatrix();

	Math::Matrix4x4 flipMatrix = getElementFlipMatrix(getModulePtr());

	//Usually one would be flipMatrix.getInverse() however this one is inverse to itself
	m_changeMatrix = flipMatrix * m_changeMatrix * flipMatrix;
	m_changeMatrix = fm->getSceneSettingsMatrix().getInverse() * m_changeMatrix * fm->getSceneSettingsMatrix();

	if (m_isConvertDrawingPivots)
	{
		recalculateDrawingPivots(m_changeMatrix);

		if (!fm->isExperimentalMode())
			fm->applyAttributes(getModulePtr()->qualifiedName(), m_drawingPivots);
		else
			setDrawingPivots();
	}
}

void ElementModule::saveDrawingPivots()
{
	m_drawingPivots = getElementTimings(getModulePtr());

	for (auto& cel : m_drawingPivots)
	{
		cel.pivot = Math::Point3d(getDrawingPivot(cel.celPtr), 0);
	}
}

void ElementModule::recalculateDrawingPivots(const Math::Matrix4x4& matrix)
{
	const Math::Matrix4x4 vectorMatrix = oglToVector(matrix);

	for (auto& cel : m_drawingPivots)
	{
		cel.pivot = vectorMatrix * cel.pivot;
		clampValues(cel.pivot);
	}
}

void ElementModule::setDrawingPivots()
{
	if (!m_isConvertDrawingPivots)
		return;

	for (auto& cel : m_drawingPivots)
	{
		setDrawingPivot(cel.celPtr, cel.pivot);
	}
	//TODO: had a setDirty() here, removed because of multithreading, but need to make sure its not needed. Could call it from elsewhere
}

void ElementModule::readjustTertiary()
{
	modifyTimings();
}
