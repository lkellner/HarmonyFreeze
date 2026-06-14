#include "drawingTransformationModule.h"

void DrawingTransformationModule::setPositionJS(const KeyframeData& kfData,
	const MatrixParameters& params,
	const double frameNo) const
{
	//Couldn't find a way to get 3d path to work with attribute syntax --> following implementation of mc slider
	//Setting 3d path first as otherwise there are problems when executing

	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
			TextAttrData{ QLatin1String("OFFSET.3DPATH.X"), params.tx, frameNo, kfData.tx && !kfData.isPosSeparate },
			TextAttrData{ QLatin1String("OFFSET.3DPATH.Y"), params.ty, frameNo, kfData.ty && !kfData.isPosSeparate },
			TextAttrData{ QLatin1String("OFFSET.3DPATH.Z"), params.tz, frameNo, kfData.tz && !kfData.isPosSeparate },
			AttrData{ QLatin1String("offset.x"), params.tx, frameNo, kfData.tx && kfData.isPosSeparate },
			AttrData{ QLatin1String("offset.y"), params.ty, frameNo, kfData.ty && kfData.isPosSeparate },
			AttrData{ QLatin1String("offset.z"), params.tz, frameNo, kfData.tz && kfData.isPosSeparate });
}


void DrawingTransformationModule::setStaticPositionJS(const MatrixParameters& params) const
{
	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ QLatin1String("offset.x"), params.tx },
			StaticAttrData{ QLatin1String("offset.y"), params.ty },
			StaticAttrData{ QLatin1String("offset.z"), params.tz });
}

void DrawingTransformationModule::processPivotAttribute(CO_OrCommand& curMacro)
{
	FreezeManager* fm = getFreezeManagerPtr();

	Math::Point3d curPivot;
	getPivotAttr()->getLocalValue(curPivot);

	const AT_BoolAttr* canAnimate = findAttribute<AT_BoolAttr>(QStringLiteral("CAN_ANIMATE"));

	if (!fm->isMoveUnusedPivots()
		&& curPivot == Math::Point3d()
		&& getFrameRange().end == -1
		&& getStaticLocalMatrixNoPivot().isIdentity()
		&& !canAnimate->localValue())
	{
		return;
	}

	if (fm->isExperimentalMode())
		setPivot(curMacro);
	else
		setPivotJS();
}