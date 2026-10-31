#include "drawingTransformationModule.h"

using namespace Qt::StringLiterals;

void DrawingTransformationModule::setPositionJS(const KeyframeData& kfData,
	const MatrixParameters& params,
	const double frameNo) const
{
	//Couldn't find a way to get 3d path to work with attribute syntax --> following implementation of mc slider
	//Setting 3d path first as otherwise there are problems when executing

	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
			TextAttrData{ u"OFFSET.3DPATH.X"_s, params.tx, frameNo, kfData.tx && !kfData.isPosSeparate },
			TextAttrData{ u"OFFSET.3DPATH.Y"_s, params.ty, frameNo, kfData.ty && !kfData.isPosSeparate },
			TextAttrData{ u"OFFSET.3DPATH.Z"_s, params.tz, frameNo, kfData.tz && !kfData.isPosSeparate },
			AttrData{ u"offset.x"_s, params.tx, frameNo, kfData.tx && kfData.isPosSeparate },
			AttrData{ u"offset.y"_s, params.ty, frameNo, kfData.ty && kfData.isPosSeparate },
			AttrData{ u"offset.z"_s, params.tz, frameNo, kfData.tz && kfData.isPosSeparate });
}


void DrawingTransformationModule::setStaticPositionJS(const MatrixParameters& params) const
{
	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ u"offset.x"_s, params.tx },
			StaticAttrData{ u"offset.y"_s, params.ty },
			StaticAttrData{ u"offset.z"_s, params.tz });
}

void DrawingTransformationModule::processPivotAttribute(CO_OrCommand& curMacro)
{
	FreezeManager* fm = getFreezeManagerPtr();

	Math::Point3d curPivot;
	getPivotAttr()->getLocalValue(curPivot);

	const AT_BoolAttr* canAnimate = findAttribute<AT_BoolAttr>("CAN_ANIMATE"_L1);

	if (!fm->isMoveUnusedPivots()
		&& curPivot == Math::Point3d()
		&& getFrameRange().end == -1
		&& getStaticLocalMatrixNoPivot().isIdentity()
		&& canAnimate
		&&!canAnimate->localValue())
	{
		return;
	}

	if (fm->isExperimentalMode())
		setPivot(curMacro);
	else
		setPivotJS();
}