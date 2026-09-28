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

	Math::Matrix4x4 freezeMatrix = getFreezeManagerPtr()->getFreezeMatrix();
	setMatrixComplexity(defineMatrixComplexity(freezeMatrix, false));

	processAttributeSet(*curMacro, true);
	processAttributeSet(*curMacro, false);
	readjustRegionOfInfluence(*curMacro, freezeMatrix);
	
	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}

double BoneModule::getStaticChainRotation(bool isRest) const
{
	const auto orientationKeyword = isRest
		? QStringLiteral("restOrientation")
		: QStringLiteral("orientation");

	return getRotationImpl(orientationKeyword,
		[](const AT_DoubleAttr& a) { return a.localValue(); });
}


double BoneModule::getChainRotation(const double frameNo) const
{
	return getRotationImpl(QStringLiteral("orientation"),
		[frameNo](const AT_DoubleAttr& a) { return a.value(frameNo); });
}


template <typename ValueFunc>
double BoneModule::getRotationImpl(const QString& orientationKeyword, ValueFunc&& valFunc) const
{
	FreezeManager* fm = getFreezeManagerPtr();

	const MO_Module* mod = getModulePtr();

	const MO_Node* parent = mod->getParentNode();

	double rotation = 0.0;

	while (parent && parent->keyword() == QLatin1String("BendyBoneModule"))
	{
		const AT_AttrList attributes = ::getAttributeList(parent);

		for (const AT_AttrDesc& attribute : std::as_const(attributes))
		{
			if (attribute._pAttr->keyword() == orientationKeyword)
			{
				const auto* a = dynamic_cast<const AT_DoubleAttr*>(attribute._pAttr);
				if (a)
				{
					if (parent->getParentNode() && parent->getParentNode()->keyword() == QLatin1String("BendyBoneModule"))
					{
						//Not the first bone in the chain
						rotation += applyUnitOffset(fm->getUnitOffsetScaleMatrix(), valFunc(*a));
					}
					else
					{
						//First bone in the chain, need to convert rotation from fields to ogl
						double rotationValue = valFunc(*a);
						rotationValue = fieldsToOgl(getModulePtr()->sceneMetrics(), rotationValue);
						rotation += applyUnitOffset(fm->getUnitOffsetScaleMatrix(), rotationValue);
					}
				}
			}
		}

		parent = parent->getParentNode();
	}

	return rotation;
}


void BoneModule::processAttributeSet(CO_OrCommand& curMacro, bool isRest)
{
	AT_Position2dAttr* positionAttr = isRest ? m_restOffsetAttr : m_offsetAttr;
	AT_DoubleAttr* radiusAttr = isRest ? m_restRadiusAttr : m_radiusAttr;
	AT_DoubleAttr* lengthAttr = isRest ? m_restLengthAttr : m_lengthAttr;
	AT_DoubleAttr* orientationAttr = isRest ? m_restOrientationAttr : m_orientationAttr;

	SC_SceneMetrics* sm = getModulePtr()->sceneMetrics();

	Math::Matrix4x4 changeMatrix = getFreezeManagerPtr()->getFreezeMatrix();
	Math::Matrix4x4 fieldsChangeMatrix = getFieldsModificationMatrix(sm, changeMatrix);
	Math::Matrix4x4 scaleShearChangeMatrix = get2dRotationMatrix(changeMatrix.getTransform2d()).getInverse() * changeMatrix.rotation();

	//Matrix only contains translation and rotation values,
	//Only the first bone in the chain will be affected
	if (isIdentity(scaleShearChangeMatrix) && hasBoneParents())
		return;

	Math::Matrix4x4 adjChangeMatrix = changeMatrix;
	Math::Matrix4x4 oldParentRotationMatrix;
	Math::Matrix4x4 newParentRotationMatrix;
	

	if (hasBoneParents())
	{
		oldParentRotationMatrix = Math::Matrix4x4().rotateDegrees(getStaticChainRotation(isRest));
		newParentRotationMatrix = get2dRotationMatrix((changeMatrix * oldParentRotationMatrix).getTransform2d());

		adjChangeMatrix = (newParentRotationMatrix.getInverse() * changeMatrix * oldParentRotationMatrix).rotation();
		fieldsChangeMatrix = getFieldsModificationMatrix(sm, adjChangeMatrix);
	}


	//OFFSET
	Math::Point2d position;
	positionAttr->getLocalValue(position);
	Math::Point3d pos3d = Math::Point3d(position);
	pos3d = fieldsChangeMatrix * pos3d;


	//ORIENTATION
	double ogOrientation = orientationAttr->localValue();
	double ogOglOrientation = hasBoneParents() ? ogOrientation : fieldsToOgl(sm, ogOrientation);

	Math::Matrix4x4 rotationMatrix = Math::Matrix4x4().rotateDegrees(ogOglOrientation);
	rotationMatrix = adjChangeMatrix * rotationMatrix;

	double oglOrientation = getAngle2d(rotationMatrix.getTransform2d());
	double orientation = hasBoneParents() ? oglOrientation : getAngle2d(getFieldsModificationMatrix(sm, rotationMatrix).getTransform2d());


	//RADIUS
	Math::Point3d radius = Math::Point3d(radiusAttr->localValue(), 0, 0);
	radius = adjChangeMatrix * radius;


	//LENGTH
	oldParentRotationMatrix.rotateDegrees(ogOglOrientation);
	newParentRotationMatrix.rotateDegrees(oglOrientation);
	Math::Matrix4x4 adjScaleShearChangeMatrix = newParentRotationMatrix.getInverse() * scaleShearChangeMatrix * oldParentRotationMatrix;
	//TODO: need to see if 3d rotations need any special treatment
	//TODO: see if behaviour changes with different scene settings

	Math::Point3d length = Math::Point3d(lengthAttr->localValue(), 0, 0);
	length = adjScaleShearChangeMatrix * length;
	
	setStaticAttributes(pos3d, orientation, radius.toVector().length(), length.toVector().length(), curMacro, isRest);

	//Rest attributes only contain static values
	if (isRest)
		return;

	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		double frameNo = curFrame;

		oldParentRotationMatrix = Math::Matrix4x4();
		newParentRotationMatrix = Math::Matrix4x4();

		if (hasBoneParents())
		{
			oldParentRotationMatrix = Math::Matrix4x4().rotateDegrees(getChainRotation(frameNo));
			newParentRotationMatrix = get2dRotationMatrix((changeMatrix * oldParentRotationMatrix).getTransform2d());

			adjChangeMatrix = (newParentRotationMatrix.getInverse() * changeMatrix * oldParentRotationMatrix).rotation();
			fieldsChangeMatrix = getFieldsModificationMatrix(sm, adjChangeMatrix);
		}

		//OFFSET
		positionAttr->getValue(frameNo, position);
		Math::Point3d pos3d = Math::Point3d(position);
		pos3d = fieldsChangeMatrix * pos3d;

		//ORIENTATION
		ogOrientation = orientationAttr->value(frameNo);
		ogOglOrientation = hasBoneParents() ? ogOrientation : fieldsToOgl(sm, ogOrientation);

		rotationMatrix = Math::Matrix4x4().rotateDegrees(ogOglOrientation);
		rotationMatrix = adjChangeMatrix * rotationMatrix;

		oglOrientation = getAngle2d(rotationMatrix.getTransform2d());
		orientation = hasBoneParents() ? oglOrientation : getAngle2d(getFieldsModificationMatrix(sm, rotationMatrix).getTransform2d());

		//RADIUS
		radius = Math::Point3d(radiusAttr->value(frameNo), 0, 0);
		radius = adjChangeMatrix * radius;

		//LENGTH
		oldParentRotationMatrix.rotateDegrees(ogOglOrientation);
		newParentRotationMatrix.rotateDegrees(oglOrientation);
		adjScaleShearChangeMatrix = newParentRotationMatrix.getInverse() * scaleShearChangeMatrix * oldParentRotationMatrix;

		length = Math::Point3d(lengthAttr->value(frameNo), 0, 0);
		length = adjScaleShearChangeMatrix * length;

		setAttributes(pos3d, orientation, radius.toVector().length(), length.toVector().length(), curMacro, frameNo);
	}
}


void BoneModule::readjustRegionOfInfluence(CO_OrCommand& curMacro, const Math::Matrix4x4& matrix)
{
	Math::Point3d scale = getScale(matrix);
	double scaleFactor = (abs(scale.x()) + abs(scale.y())) / 2;

	double transversalRadius = m_transversalRadiusAttr->localValue() * scaleFactor;
	double transversalRadiusRight = m_transversalRadiusRightAttr->localValue() * scaleFactor;
	double longitudinalRadiusBegin = m_longitudinalRadiusBeginAttr->localValue() * scaleFactor;
	double longitudinalRadius = m_longitudinalRadiusAttr->localValue() * scaleFactor;
	double influenceFadeRadius = m_influenceFadeAttr->localValue() * scaleFactor;


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


void BoneModule::setStaticAttributes(Math::Point3d position, double orientation,
	double radius, double length, CO_OrCommand& curMacro, bool isRest)
{
	clampValues(position);
	//TODO: need to get rotation close to original angle

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		AT_Position2dAttr* positionAttr = isRest ? m_restOffsetAttr : m_offsetAttr;
		AT_DoubleAttr* radiusAttr = isRest ? m_restRadiusAttr : m_radiusAttr;
		AT_DoubleAttr* lengthAttr = isRest ? m_restLengthAttr : m_lengthAttr;
		AT_DoubleAttr* orientationAttr = isRest ? m_restOrientationAttr : m_orientationAttr;

		curMacro.add(Attr::Position2d::createSetLocalValueCmd(positionAttr, position.x(), position.y()));
		curMacro.add(Attr::Double::createSetLocalValueCmd(orientationAttr, orientation));
		curMacro.add(Attr::Double::createSetLocalValueCmd(radiusAttr, radius));
		curMacro.add(Attr::Double::createSetLocalValueCmd(lengthAttr, length));
	}
	else
	{
		//JS

		QString offsetJS = isRest ? QLatin1String("restoffset") : QLatin1String("offset");
		QString orientationJS = isRest ? QLatin1String("restorientation") : QLatin1String("orientation");
		QString radiusJS = isRest ? QLatin1String("restradius") : QLatin1String("radius");
		QString lengthJS = isRest ? QLatin1String("restlength") : QLatin1String("length");

		//Similar to transformation module, can't set static value of combined paths
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ offsetJS + QLatin1String(".x"), position.x() },
			StaticAttrData{ offsetJS + QLatin1String(".y"), position.y() },
			StaticAttrData{ orientationJS, orientation },
			StaticAttrData{ radiusJS, radius},
			StaticAttrData{ lengthJS, length });
	}
}


void BoneModule::setAttributes(Math::Point3d position, double orientation, double radius,
	double length, CO_OrCommand& curMacro, double frameNo)
{
	clampValues(position);

	FreezeManager* fm = getFreezeManagerPtr();


	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position2d::createSetValueCmd(m_offsetAttr, frameNo, position.x(), position.y()));
		curMacro.add(Attr::Double::createSetValueCmd(m_orientationAttr, frameNo, orientation));
		curMacro.add(Attr::Double::createSetValueCmd(m_radiusAttr, frameNo, radius));
		curMacro.add(Attr::Double::createSetValueCmd(m_lengthAttr, frameNo, length));
	}
	else
	{
		//JS
		if (m_offsetAttr->useSeparate())
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				AttrData{ QLatin1String("offset.x"), position.x(), frameNo, true },
				AttrData{ QLatin1String("offset.y"), position.y(), frameNo, true });
		}
		else
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				Point2dAttrData{QLatin1String("offset"), Math::Point2d(position.x(),position.y()) , frameNo, true });
		}
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			AttrData{ QLatin1String("orientation"), orientation, frameNo, true },
			AttrData{ QLatin1String("radius"), radius, frameNo, true },
			AttrData{ QLatin1String("length"), length, frameNo, true });
	}
}


bool BoneModule::hasBoneParents() const
{
	MO_Node* parent = getModulePtr()->getParentNode();

	if (!parent)
		return false;

	return parent->keyword() == QLatin1String("BendyBoneModule");
}


FrameRange BoneModule::getFrameRange() const
{
	FrameRange range;

	int key;
	
	//Main attribute only detects point2d keyframes
	if (m_offsetAttr->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_offsetAttr->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_offsetAttr->separateX()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_offsetAttr->separateX()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_offsetAttr->separateY()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_offsetAttr->separateY()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_radiusAttr->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_radiusAttr->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_lengthAttr->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_lengthAttr->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (m_orientationAttr->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (m_orientationAttr->getNextKey(0, &key))
		updateFrameRange(range, key);

	return range;
}