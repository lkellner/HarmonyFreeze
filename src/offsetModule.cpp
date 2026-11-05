#include "offsetModule.h"

#include <GraphicCore/CinematicChain/CC_Transformation.h>

#include <Util/pluginmanager/PLUG_Services.h>

#include <SceneCore/module/MO_PortTransform.h>

#include <limits>

using namespace Qt::StringLiterals;

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
	m_restingOrientationAttr = findAttribute<AT_DoubleAttr>("restingOrientation"_L1);
	m_orientationAttr = findAttribute<AT_DoubleAttr>("orientation"_L1);

	const MO_Module* modulePtr = getModulePtr();

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

	setMatrixComplexity(fm->getMatrixComplexity(false));

	readjustOffsets(*curMacro, changeMatrix);
	readjustOrientation(*curMacro, changeMatrix);

	fm->addCommand(std::move(curMacro));
}

double OffsetModule::recalculateOrientation(const Math::Matrix4x4 changeMatrix, const double angle) const
{
	Math::Matrix4x4 rotMatrix = Math::Matrix4x4().rotateDegrees(angle);
	rotMatrix = recalculateIndependentHandleMatrix(getModulePtr()->sceneMetrics(), rotMatrix, changeMatrix);

	//Rougly estimated angle used as a reference know whether to add n*360 degrees
	//to final angle. This is to make sure that animations don't break
	const double referenceAngle = getAngle2d(changeMatrix.getTransform2d()) + angle;

	return matchFullRotations(referenceAngle, getAngle2d(rotMatrix.getTransform2d()));
}

void OffsetModule::readjustOrientation(CO_OrCommand& curMacro, const Math::Matrix4x4& changeMatrix)
{
	//Resting and local (static) values

	const double restingAngle = recalculateOrientation(changeMatrix, m_restingOrientationAttr->localValue());
	const double staticAngle = recalculateOrientation(changeMatrix, m_orientationAttr->localValue());

	double prevOrientation = staticAngle;

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_restingOrientationAttr, restingAngle));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_orientationAttr, staticAngle));
	}
	else
	{
		//JS
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ u"restingorientation"_s, restingAngle },
			StaticAttrData{ u"orientation"_s, staticAngle });
	}


	//Animated values

	const FrameRange range = getFrameRange();


	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		const double frameNo = curFrame;

		const double angle = recalculateOrientation(changeMatrix, m_orientationAttr->value(frameNo));

		bool isCtrlPnt = false;

		const bool isAdjustKeyframe = prevOrientation != angle && isComplexTransform() && fm->isSetInbetweenKfMode();

		prevOrientation = angle;

		m_orientationAttr->value(frameNo, &isCtrlPnt);

		if (isCtrlPnt || isAdjustKeyframe)
		{
			if (fm->isExperimentalMode()) //C++
				curMacro.add(Attr::Double::createSetValueCmd(m_orientationAttr, frameNo, angle));
			else //JS
				fm->applyAttributes(getModulePtr()->qualifiedName(),
						AttrData{ u"orientation"_s, angle, frameNo, true });
		}
	}
}
