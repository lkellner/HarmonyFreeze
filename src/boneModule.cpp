#include "boneModule.h"

#include <BaseCore/maths/MT_Point4d.h>

#include <GraphicCore/CinematicChain/CC_Transformation.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/attribute/AT_Position3dAttr.h>
#include <SceneCore/attribute/AT_Scale3dAttr.h>
#include <SceneCore/module/MO_PortTransform.h>

#include <limits>
#include <stdexcept>

BoneModule::BoneModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: ModuleBase(std::move(freezeManager), modulePtr, moduleType)
	, m_restOffsetAttr(findAttribute<AT_Position2dAttr>(QStringLiteral("restOffset")))
	, m_restRadiusAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("restRadius")))
	, m_restLengthAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("restLength")))
	, m_restOrientationAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("restOrientation")))
	, m_offsetAttr(findAttribute<AT_Position2dAttr>(QStringLiteral("offset")))
	, m_radiusAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("radius")))
	, m_lengthAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("length")))
	, m_orientationAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("orientation")))
	, m_longitudinalRadiusAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("longitudinalRadius")))
	, m_longitudinalRadiusBeginAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("longitudinalRadiusBegin")))
	, m_transversalRadiusAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("transversalRadius")))
	, m_transversalRadiusRightAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("transversalRadiusRight")))
	, m_influenceFadeAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("influenceFade")))

{
	if (!m_restOffsetAttr)
		throw std::runtime_error("missing attribute: 'm_restOffsetAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_restRadiusAttr)
		throw std::runtime_error("missing attribute: 'm_restRadiusAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_restLengthAttr)
		throw std::runtime_error("missing attribute: 'm_restLengthAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_restOrientationAttr)
		throw std::runtime_error("missing attribute: 'm_restOrientationAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_offsetAttr)
		throw std::runtime_error("missing attribute: 'm_offsetAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_radiusAttr)
		throw std::runtime_error("missing attribute: 'm_radiusAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_lengthAttr)
		throw std::runtime_error("missing attribute: 'm_lengthAttr' for " + modulePtr->qualifiedName().toStdString());
	if (!m_orientationAttr)
		throw std::runtime_error("missing attribute: 'm_orientationAttr' for " + modulePtr->qualifiedName().toStdString());
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


void BoneModule::readjustSecondary()
{
	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	Math::Matrix4x4 changeMatrix = getFreezeManagerPtr()->getFreezeMatrix();
	Math::Matrix4x4 fieldsChangeMatrix = getFieldsModificationMatrix(getModulePtr()->sceneMetrics(),
		getFreezeManagerPtr()->getFreezeMatrix());

	Math::Point2d restPosition;
	m_restOffsetAttr->getLocalValue(restPosition);
	Math::Point3d restPos3d = Math::Point3d(restPosition);

	restPos3d = fieldsChangeMatrix * restPos3d;

	Math::Point2d position;
	m_offsetAttr->getLocalValue(position);
	Math::Point3d pos3d = Math::Point3d(position);

	pos3d = fieldsChangeMatrix * pos3d;

	Math::Matrix4x4 restRotationMatrix = Math::Matrix4x4().rotateDegrees(m_restOrientationAttr->localValue());
	restRotationMatrix = fieldsChangeMatrix * restRotationMatrix;

	Math::Matrix4x4 rotationMatrix = Math::Matrix4x4().rotateDegrees(m_orientationAttr->localValue());
	rotationMatrix = fieldsChangeMatrix * rotationMatrix;

	Math::Matrix4x4 scaleShearChangeMatrix = get2dRotationMatrix(changeMatrix.getTransform2d()).getInverse() * changeMatrix.rotation();
	scaleShearChangeMatrix = Math::Matrix4x4().rotateDegrees(m_orientationAttr->localValue()) * scaleShearChangeMatrix * Math::Matrix4x4().rotateDegrees(m_orientationAttr->localValue()).getInverse();
	//TODO: need to see if 3d rotations need any special treatment
	changeMatrix.rotation().print("change matrix rotation");
	get2dRotationMatrix(changeMatrix.getTransform2d()).print("change matrix z rotation");
	scaleShearChangeMatrix.print("scaleShearMatrix");
	Math::Point3d restLength = Math::Point3d(m_restLengthAttr->localValue(), 0, 0);
	restLength = scaleShearChangeMatrix * restLength;
	
	Math::Point3d length = Math::Point3d(m_lengthAttr->localValue(), 0, 0);
	length = scaleShearChangeMatrix * length;

	setStaticAttributes(restPos3d, pos3d, restLength.toVector().length(), length.toVector().length(), getAngle2d(restRotationMatrix.getTransform2d()), getAngle2d(rotationMatrix.getTransform2d()), *curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}


void BoneModule::setStaticAttributes(Math::Point3d restPosition, Math::Point3d position,
	double restLength, double length, double restOrientation,
	double orientation, CO_OrCommand& curMacro)
{
	clampValues(position);
	//TODO: need to get rotation close to original angle

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(m_restOffsetAttr, restPosition.x(), restPosition.y()));
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(m_offsetAttr, position.x(), position.y()));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_restLengthAttr, restLength));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_lengthAttr, length));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_restOrientationAttr, restOrientation));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_orientationAttr, orientation));
	}
	else
	{
		//JS
		//Similar to transformation module, can't set static value of combined paths
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ QLatin1String("restoffset.x"), restPosition.x() },
			StaticAttrData{ QLatin1String("restoffset.x"), restPosition.x() },
			StaticAttrData{ QLatin1String("offset.x"), position.x() },
			StaticAttrData{ QLatin1String("offset.y"), position.y() },
			StaticAttrData{ QLatin1String("restlength"), restLength },
			StaticAttrData{ QLatin1String("length"), length},
			StaticAttrData{ QLatin1String("restorientation"), restOrientation },
			StaticAttrData{ QLatin1String("orientation"), orientation });
	}
}

/*
void BoneModule::processPoint(FreeformPoint* point, CO_OrCommand& curMacro)
{
	//It was decided to not change the rotation attribute for now as it would only be effected
	//by skews and non-uniform scales. However, these would also lead to modified non-uniform scale
	//and skew attributes, which the freeform module is not able to represent. 
	//The resulting transformation did therefor not lead to a significantly closer match than keeping
	//the original value.

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

	setStaticAttributes(point, pos3d, restingPos3d, curMacro);

	
	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		point->posAttr->getValue(curFrame, position);
		pos3d = Math::Point3d(position);

		pos3d = changeMatrix * pos3d;

		setAttributes(point, pos3d, curMacro, curFrame);
	}
}

void BoneModule::setStaticAttributes(FreeformPoint* point, Math::Point3d position,
	Math::Point3d restingPos, CO_OrCommand& curMacro)
{
	clampValues(position);
	clampValues(restingPos);

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(point->posAttr, position.x(), position.y()));
		curMacro.add(Attr::Position2d::createSetLocalValueCmd(point->restingPosAttr, restingPos.x(), restingPos.y()));
	}
	else
	{
		//JS
		//Similar to transformation module, can't set static value of combined paths
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ point->name + QLatin1String(".restingPosition.x"), restingPos.x() },
			StaticAttrData{ point->name + QLatin1String(".restingPosition.y"), restingPos.y() },
			StaticAttrData{ point->name + QLatin1String(".position.x"), position.x() },
			StaticAttrData{ point->name + QLatin1String(".position.y"), position.y() });
	}
}


void BoneModule::setAttributes(FreeformPoint* point, Math::Point3d position,
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
	}
}
*/

FrameRange BoneModule::getFrameRange() const
{
	FrameRange range;

	int key;
	/*
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
	}
	//Resting position can't have any keyframes 
	*/
	return range;
}