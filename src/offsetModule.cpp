#include "offsetModule.h"

#include <GraphicCore/CinematicChain/CC_Transformation.h>

#include <Util/pluginmanager/PLUG_Services.h>

#include <SceneCore/module/MO_PortTransform.h>

#include <limits>

OffsetModule::OffsetModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: CurveModuleBase(freezeManager, modulePtr, moduleType)
	, m_restingOrientationAttr(nullptr)
	, m_orientationAttr(nullptr)
{
	loadAttributes();
}

FrameRange OffsetModule::getFrameRange() const
{
	FrameRange range = CurveModuleBase::getFrameRange();

	int key;

	if (m_orientationAttr->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_orientationAttr->getPrevKey(std::numeric_limits<int>::max(), &key))
		updateFrameRange(range, key);

	return range;
}


void OffsetModule::loadAttributes()
{
	m_restingOrientationAttr = findAttribute<AT_DoubleAttr>(QStringLiteral("restingOrientation"));
	m_orientationAttr = findAttribute<AT_DoubleAttr>(QStringLiteral("orientation"));

	MO_Module* modulePtr = getModulePtr();

	if (!m_restingOrientationAttr)
		throw std::runtime_error("missing attribute: 'restingOrientation' for " + modulePtr->qualifiedName().toStdString());
	if (!m_orientationAttr)
		throw std::runtime_error("missing attribute: 'orientation' for " + modulePtr->qualifiedName().toStdString());
}

void OffsetModule::readjustSecondary()
{
	auto curMacro = std::make_shared<CO_OrCommand>();

	FreezeManager* fm = getFreezeManagerPtr();

	const Math::Matrix4x4 changeMatrix = fm->getFreezeMatrix();

	setComplexTransform(defineMatrixComplexity(changeMatrix, false));

	readjustOffsets(*curMacro, changeMatrix);
	readjustOrientation(*curMacro, changeMatrix);

	fm->addCommand(std::move(curMacro));
}


void OffsetModule::readjustOrientation(CO_OrCommand& curMacro, const Math::Matrix4x4& changeMatrix)
{
	//Resting and local (static) values

	BezierHandleInfo restingHandleInfo = getRecalculatedHandleInfo(getModulePtr()->sceneMetrics(), changeMatrix,
			0, m_restingOrientationAttr->localValue(), true);
	BezierHandleInfo staticHandleInfo = getRecalculatedHandleInfo(getModulePtr()->sceneMetrics(), changeMatrix,
			0, m_orientationAttr->localValue(), true);

	double prevOrientation = staticHandleInfo.angle;

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_restingOrientationAttr, restingHandleInfo.angle));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_orientationAttr, staticHandleInfo.angle));
	}
	else
	{
		//JS
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ QLatin1String("restingorientation"), restingHandleInfo.angle },
			StaticAttrData{ QLatin1String("orientation"), staticHandleInfo.angle });
	}


	//Animated values

	const FrameRange range = getFrameRange();


	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		const double frameNo = curFrame;

		const BezierHandleInfo handleInfo = getRecalculatedHandleInfo(getModulePtr()->sceneMetrics(),
				changeMatrix,
				0,
				m_orientationAttr->value(frameNo),
				true);


		bool isCtrlPnt = false;

		bool isAdjustKeyframe = prevOrientation != handleInfo.angle && isComplexTransform() && fm->isSetInbetweenKfMode();

		prevOrientation = handleInfo.angle;

		m_orientationAttr->value(frameNo, &isCtrlPnt);

		if (isCtrlPnt || isAdjustKeyframe)
		{
			if (fm->isExperimentalMode()) //C++
				curMacro.add(Attr::Double::createSetValueCmd(m_orientationAttr, frameNo, handleInfo.angle));
			else //JS
				fm->applyAttributes(getModulePtr()->qualifiedName(),
						AttrData{ QLatin1String("orientation"), handleInfo.angle, frameNo, true });
		}
	}
}
