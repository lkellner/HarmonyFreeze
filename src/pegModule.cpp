#include "pegModule.h"

using namespace Qt::StringLiterals;

void PegModule::setPositionJS(const KeyframeData& kfData,
	const MatrixParameters& params,
	const double frameNo) const
{
	//Couldn't find a way to get 3dpath to work with attribute syntax --> following implementation of mc slider
	//Setting 3d path first as otherwise there are problems when executing


	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
									TextAttrData{ u"POSITION.3DPATH.X"_s, params.tx, frameNo, kfData.tx && !kfData.isPosSeparate},
									TextAttrData{ u"POSITION.3DPATH.Y"_s, params.ty, frameNo, kfData.ty && !kfData.isPosSeparate},
									TextAttrData{ u"POSITION.3DPATH.Z"_s, params.tz, frameNo, kfData.tz && !kfData.isPosSeparate},
									AttrData { u"position.x"_s, params.tx, frameNo, kfData.tx && kfData.isPosSeparate},
									AttrData{ u"position.y"_s, params.ty, frameNo, kfData.ty && kfData.isPosSeparate},
									AttrData{ u"position.z"_s, params.tz, frameNo, kfData.tz && kfData.isPosSeparate});
}

void PegModule::setStaticPositionJS(const MatrixParameters& params) const
{
	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
									StaticAttrData{ u"position.x"_s, params.tx },
									StaticAttrData{ u"position.y"_s, params.ty },
									StaticAttrData{ u"position.z"_s, params.tz });
}

void PegModule::processPivotAttribute(CO_OrCommand& curMacro)
{
	if (getFreezeManagerPtr()->isExperimentalMode())
		setPivot(curMacro);
	else
		setPivotJS();
}
