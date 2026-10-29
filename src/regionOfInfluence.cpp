#include "regionOfInfluence.h"

#include <stdexcept>


RegionOfInfluence::RegionOfInfluence(std::shared_ptr<FreezeManager> freezeManager,
	MO_Module* modulePtr)
	: m_freezeManager(std::move(freezeManager))
	, m_modulePtr(modulePtr)
	, m_longitudinalRadiusAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("longitudinalRadius"), modulePtr))
	, m_longitudinalRadiusBeginAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("longitudinalRadiusBegin"), modulePtr))
	, m_transversalRadiusAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("transversalRadius"), modulePtr))
	, m_transversalRadiusRightAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("transversalRadiusRight"), modulePtr))
	, m_influenceFadeAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("influenceFade"), modulePtr))

{
	if (!m_longitudinalRadiusAttr)
		throw std::runtime_error("missing attribute: 'm_longitudinalRadiusAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_longitudinalRadiusBeginAttr)
		throw std::runtime_error("missing attribute: 'm_longitudinalRadiusBeginAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_transversalRadiusAttr)
		throw std::runtime_error("missing attribute: 'm_transversalRadiusAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_transversalRadiusRightAttr)
		throw std::runtime_error("missing attribute: 'm_transversalRadiusRightAttr' for " + modulePtr->qualifiedName().toStdString());	
	if (!m_influenceFadeAttr)
		throw std::runtime_error("missing attribute: 'm_influenceFadeAttr' for " + modulePtr->qualifiedName().toStdString());
}


void RegionOfInfluence::readjustSecondary(CO_OrCommand& curMacro, const Math::Matrix4x4& matrix)
{
	const Math::Point3d scale = getScale(matrix);
	const double scaleFactor = (abs(scale.x()) + abs(scale.y())) / 2;

	const double transversalRadius = m_transversalRadiusAttr->localValue() * scaleFactor;
	const double transversalRadiusRight = m_transversalRadiusRightAttr->localValue() * scaleFactor;
	const double longitudinalRadiusBegin = m_longitudinalRadiusBeginAttr->localValue() * scaleFactor;
	const double longitudinalRadius = m_longitudinalRadiusAttr->localValue() * scaleFactor;
	const double influenceFadeRadius = m_influenceFadeAttr->localValue() * scaleFactor;


	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_transversalRadiusAttr, transversalRadius));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_transversalRadiusRightAttr, transversalRadiusRight));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_longitudinalRadiusBeginAttr, longitudinalRadiusBegin));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_longitudinalRadiusAttr, longitudinalRadius));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_influenceFadeAttr, influenceFadeRadius));
	}
	else
	{
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ QLatin1String("transversalradius"), transversalRadius },
			StaticAttrData{ QLatin1String("transversalradiusright"), transversalRadiusRight },
			StaticAttrData{ QLatin1String("longitudinalradius"), longitudinalRadius },
			StaticAttrData{ QLatin1String("longitudinalradiusbegin"), longitudinalRadiusBegin },
			StaticAttrData{ QLatin1String("influencefade"), influenceFadeRadius });
	}
}
