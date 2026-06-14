#include "pegModule.h"

void PegModule::setPositionJS(const KeyframeData& kfData,
	const MatrixParameters& params,
	const double frameNo) const
{
	//Couldn't find a way to get 3dpath to work with attribute syntax --> following implementation of mc slider
	//Setting 3d path first as otherwise there are problems when executing


	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
									TextAttrData{ QLatin1String("POSITION.3DPATH.X"), params.tx, frameNo, kfData.tx && !kfData.isPosSeparate},
									TextAttrData{ QLatin1String("POSITION.3DPATH.Y"), params.ty, frameNo, kfData.ty && !kfData.isPosSeparate},
									TextAttrData{ QLatin1String("POSITION.3DPATH.Z"), params.tz, frameNo, kfData.tz && !kfData.isPosSeparate},
									AttrData {QLatin1String("position.x"), params.tx, frameNo, kfData.tx && kfData.isPosSeparate},
									AttrData{ QLatin1String("position.y"), params.ty, frameNo, kfData.ty && kfData.isPosSeparate},
									AttrData{ QLatin1String("position.z"), params.tz, frameNo, kfData.tz && kfData.isPosSeparate});
}

void PegModule::setStaticPositionJS(const MatrixParameters& params) const
{
	getFreezeManagerPtr()->applyAttributes(getModulePtr()->qualifiedName(),
									StaticAttrData{ QLatin1String("position.x"), params.tx },
									StaticAttrData{ QLatin1String("position.y"), params.ty },
									StaticAttrData{ QLatin1String("position.z"), params.tz });
}

void PegModule::processPivotAttribute(CO_OrCommand& curMacro)
{
	if (getFreezeManagerPtr()->isExperimentalMode())
		setPivot(curMacro);
	else
		setPivotJS();
}
