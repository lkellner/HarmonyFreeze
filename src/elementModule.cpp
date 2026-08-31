#include "elementModule.h"

#include <GraphicCore/ColorManagerLib/CM_Texture.h>

#include <GraphicCore/GraphicLib/GR_ColorDict.h>
#include <GraphicCore/GraphicLib/GR_BitmapLayer.h>
#include <GraphicCore/GraphicLib/GR_StrokeAccess.h>
#include <GraphicCore/GraphicLib/GR_BitmapAccess.h>
#include <GraphicCore/GraphicLib/GR_ColorTransform.h>

#include <SceneCore/attribute/AT_Enums.h>

#include <optional>

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


struct AlignmentContext
{
	Math::Matrix4x4 alignmentMatrix;
	double aspectRatioDifference;
	double fieldChartRatio;
	double imageAspectRatio;
	double aspectRatioQuotient;
	double designAspectRatio;
	bool isTurnBefore;
	bool forTvg;
};

static std::optional<AlignmentContext> buildAlignmentContext(MO_Module* modulePtr)
{
	const auto fieldChart = findSubAttribute<AT_DoubleAttr>(QStringLiteral("CUSTOM_NAME"), QStringLiteral("FIELD_CHART"), modulePtr);
	AT_BoolAttr* turnBeforeAttr = findAttribute<AT_BoolAttr>(QLatin1String("TURN_BEFORE_ALIGNMENT"), modulePtr);

	if (!fieldChart || !turnBeforeAttr)
		return std::nullopt;

	//Currently only tvg drawings are being supported.
	//If support for non-tvg drawings ends up being included,
	//the following changes need to be made:
	//-Identify whether the drawing is a tvg (and tvgo?) or not (e.g. via AT_ElementAttr -> CA_CelKey)
	//-Get individual imageAspectRatio instead of the tvg default 3:4 (e.g. via CEL_Cel)
	//	(might need to recalculate the aligmentMatrix for each drawing)
	//-Retrieve the scaleFactor, potentially via AT_ElementAttr
	const bool forTvg = true;
	const double scaleFactor = 1.0;

	double fieldChartRatio = fieldChart->localValue() / modulePtr->sceneMetrics()->designFieldChartY();

	if (!forTvg)
		fieldChartRatio *= scaleFactor;

	const double designAspectRatio = modulePtr->sceneMetrics()->designAspectRatio();
	const bool isTurnBefore = turnBeforeAttr->localValue();
	const double imageAspectRatio = (isTurnBefore ? 3.0 / 4.0 : 4.0 / 3.0);
	const double aspectRatioDifference = imageAspectRatio - designAspectRatio;
	const double aspectRatioQuotient = designAspectRatio / imageAspectRatio;

	return AlignmentContext{ Math::Matrix4x4{}, aspectRatioDifference, fieldChartRatio, imageAspectRatio,
		aspectRatioQuotient, designAspectRatio, isTurnBefore, forTvg };
}

static void alignLeft(AlignmentContext& ctx)
{
	ctx.alignmentMatrix.translate(ctx.aspectRatioDifference, 0.0, 0.0);
	ctx.alignmentMatrix.translate(-(1 - ctx.fieldChartRatio) * ctx.imageAspectRatio, 0.0, 0.0);

	if(ctx.isTurnBefore)
		ctx.alignmentMatrix.scale(ctx.imageAspectRatio, ctx.imageAspectRatio);
}

static void alignRight(AlignmentContext& ctx)
{
	ctx.alignmentMatrix.translate(-ctx.aspectRatioDifference, 0.0, 0.0);
	ctx.alignmentMatrix.translate((1 - ctx.fieldChartRatio) * ctx.imageAspectRatio, 0.0, 0.0);

	if(ctx.isTurnBefore)
		ctx.alignmentMatrix.scale(ctx.imageAspectRatio, ctx.imageAspectRatio);
}

static void alignTop(AlignmentContext& ctx)
{
	ctx.alignmentMatrix.translate(0.0, 1 - ctx.aspectRatioQuotient, 0.0);
	ctx.alignmentMatrix.translate(0.0, (1 - ctx.fieldChartRatio) * ctx.aspectRatioQuotient, 0.0);

	if (ctx.isTurnBefore)
		ctx.alignmentMatrix.scale(ctx.designAspectRatio, ctx.designAspectRatio);
	else
		ctx.alignmentMatrix.scale(ctx.aspectRatioQuotient, ctx.aspectRatioQuotient);
}

static void alignBottom(AlignmentContext& ctx)
{
	ctx.alignmentMatrix.translate(0.0, -1 + ctx.aspectRatioQuotient, 0.0);
	ctx.alignmentMatrix.translate(0.0, -(1 - ctx.fieldChartRatio) * ctx.aspectRatioQuotient, 0.0);

	if(ctx.isTurnBefore)
		ctx.alignmentMatrix.scale(ctx.designAspectRatio, ctx.designAspectRatio);
	else
		ctx.alignmentMatrix.scale(ctx.aspectRatioQuotient, ctx.aspectRatioQuotient);
}

static void centerFill(AlignmentContext& ctx)
{
	if (ctx.imageAspectRatio < ctx.designAspectRatio)
	{
		//Narrow
		if(ctx.isTurnBefore)
			ctx.alignmentMatrix.scale(ctx.designAspectRatio, ctx.designAspectRatio);
		else
			ctx.alignmentMatrix.scale(ctx.aspectRatioQuotient, ctx.aspectRatioQuotient);
	}
	else if (ctx.isTurnBefore)
	{
		//Wide
		ctx.alignmentMatrix.scale(ctx.imageAspectRatio, ctx.imageAspectRatio);
	}
}

static void centerFit(AlignmentContext& ctx)
{
	if (ctx.imageAspectRatio < ctx.designAspectRatio && ctx.isTurnBefore)
	{
		//Narrow
		ctx.alignmentMatrix.scale(ctx.imageAspectRatio, ctx.imageAspectRatio);
	}
	else
	{
		//Wide
		if(!ctx.isTurnBefore)
			ctx.alignmentMatrix.scale(ctx.aspectRatioQuotient, ctx.aspectRatioQuotient);
		else
			ctx.alignmentMatrix.scale(ctx.designAspectRatio, ctx.designAspectRatio);
	}
}

static void centerLR(AlignmentContext& ctx)
{
	if(ctx.isTurnBefore)
		ctx.alignmentMatrix.scale(ctx.imageAspectRatio, ctx.imageAspectRatio);
	//Only needs to be adjusted in case of "isTurnBefore". Otherwise it's already "CENTER_LR"
}

static void centerTB(AlignmentContext& ctx)
{
	if (ctx.isTurnBefore)
		ctx.alignmentMatrix.scale(ctx.designAspectRatio, ctx.designAspectRatio);
	else
		ctx.alignmentMatrix.scale(ctx.aspectRatioQuotient, ctx.aspectRatioQuotient);
}

static void stretch(AlignmentContext& ctx)
{
	if(ctx.isTurnBefore)
		ctx.alignmentMatrix.scale(ctx.designAspectRatio, ctx.imageAspectRatio);
	else
		ctx.alignmentMatrix.scale(ctx.aspectRatioQuotient, 1.0);
}

static void centerFirstPage(AlignmentContext& ctx)
{
	if (ctx.imageAspectRatio < ctx.designAspectRatio && !ctx.forTvg)
	{
		//Bottom align
		ctx.alignmentMatrix.translate(0.0, -1 + ctx.aspectRatioQuotient, 0.0);
		ctx.alignmentMatrix.translate(0.0, -(1 - ctx.fieldChartRatio) * (ctx.aspectRatioQuotient - 1), 0.0);

		ctx.alignmentMatrix.scale(ctx.aspectRatioQuotient, ctx.aspectRatioQuotient);

		if(ctx.isTurnBefore)
			ctx.alignmentMatrix.scale(ctx.designAspectRatio, ctx.designAspectRatio);
	}
	else
	{
		//Left align
		ctx.alignmentMatrix.translate(ctx.aspectRatioDifference, 0, 0);
		ctx.alignmentMatrix.translate(-(1 - ctx.fieldChartRatio) * ctx.aspectRatioDifference, 0, 0);

		if (ctx.isTurnBefore)
			ctx.alignmentMatrix.scale(ctx.imageAspectRatio, ctx.imageAspectRatio);
	}
}


Math::Matrix4x4 ElementModule::getAlignmentMatrix()
{
	//The aligmentMatrix depends on a variety of factors: Alignment Scene Settings, 
	//Alignment Settings on the element modules themselves, as well as the Scene Settings (Number of Units) that
	//were in effect during the time, the element module was being created (saved as the "FIELD_CHART" sub attribute)

	AT_EnumAttrBase* alignmentAttr = findAttribute<AT_EnumAttrBase>(QLatin1String("ALIGNMENT_RULE"));

	if (!alignmentAttr)
		return {};

	std::optional<AlignmentContext> alignmentContext = buildAlignmentContext(getModulePtr());

	if (!alignmentContext)
		return {};

	const AT_Enums::AlignmentRule alignment = AT_Enums::AlignmentRule(alignmentAttr->localValueInt());

	AlignmentContext& ctx = *alignmentContext;

	switch (alignment)
	{
	case(AT_Enums::LEFT):
		alignLeft(ctx);
		break;
	case(AT_Enums::RIGHT):
		alignRight(ctx);
		break;
	case(AT_Enums::TOP):
		alignTop(ctx);
		break;
	case(AT_Enums::BOTTOM):
		alignBottom(ctx);
		break;
	case(AT_Enums::CENTER_FILL):
		centerFill(ctx);
		break;
	case(AT_Enums::CENTER_FIT):
		centerFit(ctx);
		break;
	case(AT_Enums::CENTER_LR):
		centerLR(ctx);
		break;
	case(AT_Enums::CENTER_TB):
		centerTB(ctx);
		break;
	case(AT_Enums::STRETCH):
		stretch(ctx);
		break;
	case(AT_Enums::CENTER_FIRST_PAGE):
		centerFirstPage(ctx);
		break;
	case(AT_Enums::ASIS):
		break;
	}
	
	if (ctx.isTurnBefore)
		ctx.alignmentMatrix.rotateDegrees(90);

	if (ctx.forTvg)
		ctx.alignmentMatrix.scale(ctx.fieldChartRatio, ctx.fieldChartRatio);
	else
		ctx.alignmentMatrix.scale(ctx.fieldChartRatio, ctx.fieldChartRatio, ctx.fieldChartRatio);

	return getElementFlipMatrix(getModulePtr()) * ctx.alignmentMatrix;
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
