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

		if (attr._pAttr->typeName() == QStringLiteral("FREE-FORM-DEFORMATION-ATTRIBUTE"))
			cAttr = dynamic_cast<AT_ComplexAttr*>(attr._pAttr);

		if (!cAttr)
			continue;
	}
	printAttributes(attrList);

}


void FreeformModule::readjustSecondary()
{
	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	/*
	processPivot(m_pivot01Attr, QLatin1String("pivot1"), *curMacro);
	processPivot(m_pivot02Attr, QLatin1String("pivot2"), *curMacro);
	processPivot(m_pivot03Attr, QLatin1String("pivot3"), *curMacro);
	*/
	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}


void FreeformModule::processPivot(AT_Position2dAttr* pivotAttr, QString pivotKeyword, CO_OrCommand& curMacro)
{
	Math::Matrix4x4 changeMatrix;

	Math::Point2d position;
	pivotAttr->getLocalValue(position);
	Math::Point3d pos3d = Math::Point3d(position);

	pos3d = changeMatrix * pos3d;

	setStaticAttributes(pos3d, pivotAttr, pivotKeyword, curMacro);

	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		pivotAttr->getValue(curFrame, position);
		pos3d = Math::Point3d(position);

		pos3d = changeMatrix * pos3d;

		setAttributes(pos3d, pivotAttr, pivotKeyword, curMacro, curFrame);
	}
}

void FreeformModule::setStaticAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, CO_OrCommand& curMacro)
{
	clampValues(position);

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(attr, position.x(), position.y()));
	}
	else
	{
		//JS
		//Similar to transformation module, can't set static value of combined paths
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ attributeKeyword + QLatin1String(".x"), position.x() },
			StaticAttrData{ attributeKeyword + QLatin1String(".y"), position.y() });
	}
}


void FreeformModule::setAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, 
	CO_OrCommand& curMacro, double frameNo)
{
	clampValues(position);

	FreezeManager* fm = getFreezeManagerPtr();


	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetValueCmd(attr, frameNo, position.x(), position.y()));
	}
	else
	{
		//JS

		if (attr->useSeparate())
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				AttrData{ attributeKeyword + QLatin1String(".x"), position.x(), frameNo, true },
				AttrData{ attributeKeyword + QLatin1String(".y"), position.y(), frameNo, true });
		}
		else
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				Point2dAttrData{ attributeKeyword, Math::Point2d(position.x(),position.y()) , frameNo, true });
		}
	}
}


FrameRange FreeformModule::getFrameRange() const
{
	FrameRange range;

	int key;

	/*
	//Main attribute only detects point2d keyframes
	if (m_pivot01Attr->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_pivot01Attr->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_pivot01Attr->separateX()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_pivot01Attr->separateX()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_pivot01Attr->separateY()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_pivot01Attr->separateY()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);
	
	if (m_pivot02Attr->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_pivot02Attr->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_pivot02Attr->separateX()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_pivot02Attr->separateX()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_pivot02Attr->separateY()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_pivot02Attr->separateY()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);
	
	if (m_pivot03Attr->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_pivot03Attr->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_pivot03Attr->separateX()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_pivot03Attr->separateX()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_pivot03Attr->separateY()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_pivot03Attr->separateY()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);
*/
	return range;
}