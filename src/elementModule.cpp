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


void ElementModule::processBitmaps(const CA_CelPtr celPtr, GR_CompositeVectorDrawingObj* compDrawing,const Math::Matrix4x4& matrix) const
{
	//This part of the code is currently only available in experimental mode as there hasn't been found
	//a way to undo bitmap transformations for now.

	if (!getFreezeManagerPtr()->isExperimentalMode())
		return;

	const QString s = QStringLiteral("Modify bitmaps");

	for (int i = 0; i <= 3; i++)
	{
		GR_VectorDrawingObj* drawingLayer = compDrawing->GetArt(i);

		if (!drawingLayer->hasBitmapLayers())
			continue;

		GR_BitmapAccess * bitmapAccess = new GR_BitmapAccess();
		bitmapAccess->SetCurrentDrawing(drawingLayer);

		GR_DrawingAccess* drawingAccess = reinterpret_cast<GR_DrawingAccess*>(bitmapAccess);
		SDK_Drawing::ChangeScope bitmapScope;

		//Custom GR_Listener to test if the cast GR_DrawingAccess is still working
		BitmapListener listener = BitmapListener(*drawingAccess);

		bitmapScope.listenForChanges(*drawingAccess, celPtr, i);
		bitmapAccess->beginOperations();

		GR_BitmapNode* curBitmapNode = drawingLayer->topBitmapNode();
		GR_BitmapNode* prevBitmapNode = nullptr;

		while (curBitmapNode)
		{
			prevBitmapNode = curBitmapNode->previousBitmapNode();

			GR_BitmapLayer& bitmapLayer = curBitmapNode->bitmapLayer();

			bitmapAccess->transformBitmap(&bitmapLayer, matrix.getTransform2d());

			curBitmapNode = prevBitmapNode;
		}

		bitmapAccess->endOperations();
		bitmapScope.stopListeningForChanges(*drawingAccess);
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

	processBitmaps(celPtr, compDrawing, matrix);
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
	//Temporarily moved undo scope here to isolate bitmap tests
	const QString s = QStringLiteral("Modify vector drawings");
	SDK_Drawing::UndoScope undoScope(s);

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


Math::Matrix4x4 ElementModule::getAlignmentMatrix()
{
	const auto fieldChart = findSubAttribute<AT_DoubleAttr>(QStringLiteral("CUSTOM_NAME"), QStringLiteral("FIELD_CHART"), getModulePtr());
	const double fieldChartVal = fieldChart->localValue();
	const double fieldChartRatio = fieldChartVal / getModulePtr()->sceneMetrics()->designFieldChartY();

	const double designAspectRatio = getModulePtr()->sceneMetrics()->designAspectRatio();

	const bool isTurnBefore = findAttribute<AT_BoolAttr>(QLatin1String("TURN_BEFORE_ALIGNMENT"))->localValue();

	const double imageAspectRatio = (isTurnBefore ? 3.0 / 4.0 : 4.0 / 3.0);
	const double aspectRatioDifference = imageAspectRatio - designAspectRatio;
	const double aspectRatioQuotient = designAspectRatio / imageAspectRatio;

	bool forTvg = true; //TODO: implement for false, might have to go image by image?
	/*
	if (!forTvg)
		sf *= scaleFactor;
		*/


	//TODO: can probably simplify this
	AT_Enums::AlignmentRule alignment = AT_Enums::ASIS;
	
	AT_Attr* att = getModulePtr()->findAttributeByKeyword(QStringLiteral("ALIGNMENT_RULE"));
	if (att)
	{
		AT_EnumAttrBase* base = dynamic_cast<AT_EnumAttrBase*>(att);
		if (base)
		{
			alignment = AT_Enums::AlignmentRule(base->localValueInt());
		}
	}

	Math::Matrix4x4 alignmentMatrix;


	switch (alignment)
	{
	case(AT_Enums::LEFT):
	{
		alignmentMatrix.translate(aspectRatioDifference, 0.0, 0.0);
		alignmentMatrix.translate(-(1 - fieldChartRatio) * imageAspectRatio, 0.0, 0.0);

		if(isTurnBefore)
			alignmentMatrix.scale(imageAspectRatio, imageAspectRatio);

		break;
	}
	case(AT_Enums::RIGHT):
	{
		alignmentMatrix.translate(-aspectRatioDifference, 0.0, 0.0);
		alignmentMatrix.translate((1 - fieldChartRatio) * imageAspectRatio, 0.0, 0.0);

		if(isTurnBefore)
			alignmentMatrix.scale(imageAspectRatio, imageAspectRatio);

		break;
	}
	case(AT_Enums::TOP):
	{
		alignmentMatrix.translate(0.0, 1 - aspectRatioQuotient, 0.0);
		alignmentMatrix.translate(0.0, (1 - fieldChartRatio) * aspectRatioQuotient, 0.0);

		if (isTurnBefore)
			alignmentMatrix.scale(designAspectRatio, designAspectRatio);
		else
			alignmentMatrix.scale(aspectRatioQuotient, aspectRatioQuotient);

		break;
	}
	case(AT_Enums::BOTTOM):
	{
		alignmentMatrix.translate(0.0, -1 + aspectRatioQuotient, 0.0);
		alignmentMatrix.translate(0.0, -(1 - fieldChartRatio) * aspectRatioQuotient, 0.0);

		if(isTurnBefore)
			alignmentMatrix.scale(designAspectRatio, designAspectRatio);
		else
			alignmentMatrix.scale(aspectRatioQuotient, aspectRatioQuotient);

		break;
	}
	case(AT_Enums::CENTER_FILL):
	{
		if (imageAspectRatio < designAspectRatio)
		{
			//Narrow
			if(isTurnBefore)
				alignmentMatrix.scale(designAspectRatio, designAspectRatio);
			else
				alignmentMatrix.scale(aspectRatioQuotient, aspectRatioQuotient);
		}
		else if (isTurnBefore)
		{
			//Wide
			alignmentMatrix.scale(imageAspectRatio, imageAspectRatio);
		}
		
		break;
	}
	case(AT_Enums::CENTER_FIT):
	{
		if (imageAspectRatio < designAspectRatio && isTurnBefore)
		{
			//Narrow
			alignmentMatrix.scale(imageAspectRatio, imageAspectRatio);
		}
		else
		{
			//Wide
			if(!isTurnBefore)
				alignmentMatrix.scale(aspectRatioQuotient, aspectRatioQuotient);
			else
				alignmentMatrix.scale(designAspectRatio, designAspectRatio);
		}

		break;
	}
	case(AT_Enums::CENTER_LR):
	{
		if(isTurnBefore)
			alignmentMatrix.scale(imageAspectRatio, imageAspectRatio);
		//Only needs to be adjusted in case of "isTurnBefore". Otherwise it's already "CENTER_LR"
		break;
	}
	case(AT_Enums::CENTER_TB):
	{
		if (isTurnBefore)
			alignmentMatrix.scale(designAspectRatio, designAspectRatio);
		else
			alignmentMatrix.scale(aspectRatioQuotient, aspectRatioQuotient);

		break;
	}
	case(AT_Enums::STRETCH):
	{
		if(isTurnBefore)
			alignmentMatrix.scale(designAspectRatio, imageAspectRatio);
		else
			alignmentMatrix.scale(aspectRatioQuotient, 1.0);

		break;
	}
	case(AT_Enums::CENTER_FIRST_PAGE):
	{
		if (imageAspectRatio < designAspectRatio && !forTvg)
		{
			// Bottom align.
			alignmentMatrix.translate(0.0, -1 + aspectRatioQuotient, 0.0);
			alignmentMatrix.translate(0.0, -(1 - fieldChartRatio) * (aspectRatioQuotient - 1), 0.0);

			alignmentMatrix.scale(aspectRatioQuotient, aspectRatioQuotient);

			if(isTurnBefore)
				alignmentMatrix.scale(designAspectRatio, designAspectRatio);
		}
		else
		{
			// Left align.
			alignmentMatrix.translate(aspectRatioDifference, 0, 0);
			alignmentMatrix.translate(-(1 - fieldChartRatio) * aspectRatioDifference, 0, 0);
			
			if (isTurnBefore)
				alignmentMatrix.scale(imageAspectRatio, imageAspectRatio);
		}

		break;
	}
	default:
	;
	}
	
	if (isTurnBefore)
	{
		alignmentMatrix.rotateDegrees(90);
	}


	if (forTvg)
		alignmentMatrix.scale(fieldChartRatio, fieldChartRatio);
	else
		alignmentMatrix.scale(fieldChartRatio, fieldChartRatio, fieldChartRatio);

	return getElementFlipMatrix(getModulePtr()) * alignmentMatrix;
}


void ElementModule::readjustSecondary()
{
	updateDrawingPivotConversionStatus();

	FreezeManager* fm = getFreezeManagerPtr();
	m_changeMatrix = fm->getFreezeMatrix();


	const Math::Matrix4x4 alignmentMatrix = getAlignmentMatrix();

	m_changeMatrix = alignmentMatrix.getInverse() * m_changeMatrix * alignmentMatrix;

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
