#include "freeformModule.h"

#include <BaseCore/maths/MT_Point4d.h>

#include <GraphicCore/CinematicChain/CC_Transformation.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/attribute/AT_Position3dAttr.h>
#include <SceneCore/attribute/AT_Rotation3dAttr.h>
#include <SceneCore/attribute/AT_Scale3dAttr.h>
#include <SceneCore/module/MO_PortTransform.h>

#include <limits>
#include <stdexcept>

FreeformModule::FreeformModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: ModuleBase(std::move(freezeManager), modulePtr, moduleType)
{
	AT_AttrList attrList = getAttributeList();

	for (auto& attr : attrList)
	{
		AT_ComplexAttr* cAttr;

		if (attr._pAttr->typeName() != QStringLiteral("FREE-FORM-DEFORMATION-ATTRIBUTE"))
			continue;
		cAttr = dynamic_cast<AT_ComplexAttr*>(attr._pAttr);

		if (!cAttr)
			continue;

		AT_Position2dAttr* posAttr = findSubAttribute<AT_Position2dAttr>(cAttr, QStringLiteral("POSITION"), modulePtr);
		AT_Position2dAttr* restingPosAttr = findSubAttribute<AT_Position2dAttr>(cAttr, QStringLiteral("RESTING_POSITION"), modulePtr);
		AT_DoubleAttr* rotAttr = findSubAttribute<AT_DoubleAttr>(cAttr, QStringLiteral("Rotation"), modulePtr);

		//Need to use xmlKeyword() as this corresponds to the keyword used in the JS syntax
		if(posAttr && restingPosAttr && rotAttr)
			m_freeformPoints.push_back(std::make_unique <FreeformPoint>(cAttr->xmlKeyword(), posAttr, restingPosAttr, rotAttr));
	}
}


void FreeformModule::readjustSecondary()
{
	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	for (const auto& point : m_freeformPoints)
		processPoint(point.get(), *curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}


void FreeformModule::processPoint(FreeformPoint* point, CO_OrCommand& curMacro)
{
	Math::Matrix4x4 changeMatrix = getFieldsModificationMatrix(getModulePtr()->sceneMetrics(), 
		getFreezeManagerPtr()->getFreezeMatrix());

	Math::Point2d restingPos;
	point->restingPosAttr->getLocalValue(restingPos);
	Math::Point3d restingPos3d = Math::Point3d(restingPos);

	restingPos3d = changeMatrix * restingPos3d;

	Math::Point2d position;
	point->posAttr->getLocalValue(position);
	Math::Point3d pos3d = Math::Point3d(position);

	pos3d = changeMatrix * pos3d;

	double rotation = 0;

	setStaticAttributes(point, pos3d, restingPos3d, rotation, curMacro);

	
	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		point->posAttr->getValue(curFrame, position);
		pos3d = Math::Point3d(position);

		pos3d = changeMatrix * pos3d;

		setAttributes(point, pos3d, rotation, curMacro, curFrame);
	}
}

void FreeformModule::setStaticAttributes(FreeformPoint* point, Math::Point3d position, Math::Point3d restingPos, 
	double rotation, CO_OrCommand& curMacro)
{
	clampValues(position);
	clampValues(restingPos);

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(point->posAttr, position.x(), position.y()));
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(point->restingPosAttr, restingPos.x(), restingPos.y()));
		curMacro.add(Attr::Double::createSetLocalValueCmd(point->rotAttr, rotation));
	}
	else
	{
		//JS
		//Similar to transformation module, can't set static value of combined paths
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ point->name + QLatin1String(".restingPosition.x"), restingPos.x() },
			StaticAttrData{ point->name + QLatin1String(".restingPosition.y"), restingPos.y() },
			StaticAttrData{ point->name + QLatin1String(".position.x"), position.x() },
			StaticAttrData{ point->name + QLatin1String(".position.y"), position.y() },
			StaticAttrData{ point->name + QLatin1String(".rotation"), rotation });
	}
}


void FreeformModule::setAttributes(FreeformPoint* point, Math::Point3d position, double rotation,
	CO_OrCommand& curMacro, double frameNo)
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
				AttrData{ point->name + QLatin1String(".position.x"), position.x(), frameNo, true },
				AttrData{ point->name + QLatin1String(".position.y"), position.y(), frameNo, true });
		}
		else
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				Point2dAttrData{ point->name + QLatin1String(".position"), Math::Point2d(position.x(),position.y()) , frameNo, true });
		}

		fm->applyAttributes(getModulePtr()->qualifiedName(),
			AttrData{ point->name + QLatin1String(".rotation"), rotation, frameNo, true });
	}
}


FrameRange FreeformModule::getFrameRange() const
{
	FrameRange range;

	int key;

	//Main attribute only detects point2d keyframes
	for (const auto& point : m_freeformPoints)
	{
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

		if (point->rotAttr)
		{
			if (point->rotAttr->getNextKey(0, &key))
				updateFrameRange(range, key);

			if (point->rotAttr->getPrevKey(INT_MAX, &key))
				updateFrameRange(range, key);
		}
	}
	//Resting position can't have any keyframes 

	return range;
}