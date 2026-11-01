#include "pointModuleBase.h"

#include <BaseCore/maths/MT_Point4d.h>

#include <GraphicCore/CinematicChain/CC_Transformation.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/attribute/AT_Position3dAttr.h>
#include <SceneCore/module/MO_PortTransform.h>

using namespace Qt::StringLiterals;

PointModuleBase::PointModuleBase(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: ModuleBase(std::move(freezeManager), modulePtr, moduleType)
{
}


void PointModuleBase::readjustSecondary()
{
	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	for (const auto& point : m_transformPoints)
		processPoint(point.get(), *curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}


void PointModuleBase::processPoint(TransformPoint* point, CO_OrCommand& curMacro)
{
	Math::Matrix4x4 changeMatrix = getFieldsModificationMatrix(getModulePtr()->sceneMetrics(), 
		getFreezeManagerPtr()->getFreezeMatrix());

	Math::Point2d position;
	point->posAttr->getLocalValue(position);
	Math::Point3d pos3d = Math::Point3d(position);

	pos3d = changeMatrix * pos3d;

	setStaticAttributes(point, pos3d,  curMacro);

	if(!point->isAnimated)
		return;

	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		point->posAttr->getValue(curFrame, position);
		pos3d = Math::Point3d(position);

		pos3d = changeMatrix * pos3d;

		setAttributes(point, pos3d, curMacro, curFrame);
	}
}

void PointModuleBase::setStaticAttributes(TransformPoint* point, Math::Point3d position,
	CO_OrCommand& curMacro) const
{
	clampValues(position);

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(point->posAttr, position.x(), position.y()));
	}
	else
	{
		//JS
		//Similar to transformation module, can't set static value of combined paths
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ point->name + u".position.x"_s, position.x() },
			StaticAttrData{ point->name + u".position.y"_s, position.y() });
	}
}


void PointModuleBase::setAttributes(TransformPoint* point, Math::Point3d position,
	CO_OrCommand& curMacro, double frameNo) const
{
	clampValues(position);

	FreezeManager* fm = getFreezeManagerPtr();


	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetValueCmd(point->posAttr, frameNo, position.x(), position.y()));
	}
	else
	{
		//JS

		if (point->posAttr->useSeparate())
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				AttrData{ point->name + u".position.x"_s, position.x(), frameNo, true },
				AttrData{ point->name + u".position.y"_s, position.y(), frameNo, true });
		}
		else
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				Point2dAttrData{ point->name + u".position"_s, Math::Point2d(position.x(),position.y()) , frameNo, true });
		}
	}
}


FrameRange PointModuleBase::getFrameRange() const
{
	FrameRange range;

	int key;

	//Main attribute only detects point2d keyframes
	for (const auto& point : m_transformPoints)
	{
		//Resting position, etc can't have any keyframes
		if(!point->isAnimated)
			continue;

		if (point->posAttr)
		{
			if (point->posAttr->getPrevKey(INT_MAX, &key))
				updateFrameRange(range, key);

			if (point->posAttr->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (point->posAttr->separateX()->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (point->posAttr->separateX()->getPrevKey(INT_MAX, &key))
				updateFrameRange(range, key);

			if (point->posAttr->separateY()->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (point->posAttr->separateY()->getPrevKey(INT_MAX, &key))
				updateFrameRange(range, key);
		}
	}

	return range;
}

void PointModuleBase::addTransformPoint(std::unique_ptr<TransformPoint> point)
{
	m_transformPoints.push_back(std::move(point));
}