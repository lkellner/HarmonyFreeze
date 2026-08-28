#include "pkoModule.h"

#include <SceneCore/module/MO_PortTransform.h>
#include <GraphicCore/CinematicChain/CC_Transformation.h>

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

	printf("attributes found\n");
}

void PkoModule::readjustSecondary()
{
	FreezeManager* fm = getFreezeManagerPtr();

	Math::Matrix4x4 oglChangeMatrix = fm->getFreezeMatrix();
	Math::Matrix4x4 fieldsChangeMatrix = getFieldsModificationMatrix(getModulePtr()->sceneMetrics(), oglChangeMatrix);

	Math::Point2d position;
	m_pivot01Attr->getLocalValue(position);
	Math::Point3d pos3d = Math::Point3d(position);

	pos3d = fieldsChangeMatrix * pos3d;

	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	setStaticAttributes(pos3d, m_pivot01Attr, QLatin1String("pivot1"), *curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
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