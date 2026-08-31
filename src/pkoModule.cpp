#include "pkoModule.h"

#include <SceneCore/module/MO_PortTransform.h>
#include <GraphicCore/CinematicChain/CC_Transformation.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>


#include <limits>
#include <stdexcept>

PkoModule::PkoModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: ModuleBase(std::move(freezeManager), modulePtr, moduleType)
	, m_pivot01Attr(findAttribute<AT_Position2dAttr>(QStringLiteral("PIVOT1")))
	, m_pivot02Attr(findAttribute<AT_Position2dAttr>(QStringLiteral("PIVOT2")))
	, m_pivot03Attr(findAttribute<AT_Position2dAttr>(QStringLiteral("PIVOT3")))
{
	if (!m_pivot01Attr)
		throw std::runtime_error("missing attribute: 'm_pivot01Attr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_pivot02Attr)
		throw std::runtime_error("missing attribute: 'm_pivot02Attr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_pivot03Attr)
		throw std::runtime_error("missing attribute: 'm_pivot03Attr' for " + modulePtr->qualifiedName().toStdString());
}

void PkoModule::readjustSecondary()
{
	FreezeManager* fm = getFreezeManagerPtr();

	Math::Matrix4x4 oglChangeMatrix = fm->getFreezeMatrix();
	Math::Matrix4x4 fieldsChangeMatrix = getFieldsModificationMatrix(getModulePtr()->sceneMetrics(), oglChangeMatrix);

	

	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	processPivot(fieldsChangeMatrix, m_pivot01Attr, QLatin1String("pivot1"), *curMacro);
	processPivot(fieldsChangeMatrix, m_pivot02Attr, QLatin1String("pivot2"), *curMacro);
	processPivot(fieldsChangeMatrix, m_pivot03Attr, QLatin1String("pivot3"), *curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}


void PkoModule::processPivot(Math::Matrix4x4 changeMatrix, AT_Position2dAttr* pivotAttr, QString pivotKeyword, CO_OrCommand& curMacro)
{
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

void PkoModule::setStaticAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, CO_OrCommand& curMacro)
{
	clampValues(position);

	//Similar to transformation module, can't set local value of combined paths

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(attr, position.x(), position.y()));
	}
	else
	{
		//JS

		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ attributeKeyword + QLatin1String(".x"), position.x() },
			StaticAttrData{ attributeKeyword + QLatin1String(".y"), position.y() });
	}
}


void PkoModule::setAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, CO_OrCommand& curMacro, double frameNo)
{
	clampValues(position);

	Math::Point2d tempPoint;

	bool isPosCtrlPnt = false;
	bool isConstSeg = false;


	attr->getValue(frameNo, tempPoint, &isPosCtrlPnt, &isConstSeg); //FIXME interface change

	//Similar to transformation module, can't set local value of combined paths

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		if (isPosCtrlPnt)
			curMacro.add(Attr::Position2d::createSetValueCmd(attr, frameNo, position.x(), position.y()));
	}
	else
	{
		//JS

		if (attr->useSeparate())
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				AttrData{ attributeKeyword + QLatin1String(".x"), position.x(), frameNo, isPosCtrlPnt },
				AttrData{ attributeKeyword + QLatin1String(".y"), position.y(), frameNo, isPosCtrlPnt });
		}
		else
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				Point2dAttrData{ attributeKeyword, Math::Point2d(position.x(),position.y()) , frameNo, isPosCtrlPnt });
		}
	}
}


FrameRange PkoModule::getFrameRange() const
{
	FrameRange range;

	int key;

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

	return range;
}