#include "boneModule.h"

#include <BaseCore/maths/MT_Point4d.h>

#include <GraphicCore/CinematicChain/CC_Transformation.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/attribute/AT_Position3dAttr.h>
#include <SceneCore/attribute/AT_Scale3dAttr.h>
#include <SceneCore/module/MO_PortTransform.h>

#include <limits>
#include <stdexcept>

bool hasOffset(const MO_Node* node)
{
	if (!node)
		return false;

	AT_Position2dAttr* offsetAttr = ::findAttribute<AT_Position2dAttr>(QStringLiteral("offset"), node);
	AT_Position2dAttr* restOffsetAttr = ::findAttribute<AT_Position2dAttr>(QStringLiteral("restOffset"), node);

	if (!offsetAttr || !restOffsetAttr)
		return false;

	//As soon as either the offset or restOffset attributes have a non-zero value, 
	//the bone deformers rotation is set as a fields rotation value instead of ogl
	Math::Point2d position;

	restOffsetAttr->getLocalValue(position);
	if (position != Math::Point2d())
		return true;

	offsetAttr->getLocalValue(position);
	if (position != Math::Point2d())
		return true;

	FrameRange range;
	int key;

	//Main attribute only detects point2d keyframes
	if (offsetAttr->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (offsetAttr->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (offsetAttr->separateX()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (offsetAttr->separateX()->getNextKey(0, &key))
		updateFrameRange(range, key);

	if (offsetAttr->separateY()->getPrevKey(INT_MAX, &key))
		updateFrameRange(range, key);

	if (offsetAttr->separateY()->getNextKey(0, &key))
		updateFrameRange(range, key);

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		double frameNo = curFrame;
		offsetAttr->getValue(frameNo, position);
		if (position != Math::Point2d())
			return true;
	}

	return false;
}


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

	const auto posKeyword = isRest
		? QStringLiteral("restOffset")
		: QStringLiteral("offset");

	return getRotationImpl(orientationKeyword,
		[](const AT_DoubleAttr& a) { return a.localValue(); });
}


double BoneModule::getChainRotation(const double frameNo) const
{
	const auto posKeyword = QStringLiteral("offset");

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
		AT_DoubleAttr* attr = ::findAttribute<AT_DoubleAttr>(orientationKeyword, parent);
		if (!attr)
			continue;

		if (parent->getParentNode() && parent->getParentNode()->keyword() == QLatin1String("BendyBoneModule")
			&& !hasOffset(parent))
		{
			//Not the first bone in the chain
			rotation += applyUnitOffset(fm->getUnitOffsetScaleMatrix(), valFunc(*attr));
		}
		else
		{
			//First bone in the chain, need to convert rotation from fields to ogl
			rotation += fieldsToOgl(mod->sceneMetrics(), valFunc(*attr));
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

	Math::Matrix4x4 changeMatrix = convertToProjectionMatrix(getFreezeManagerPtr()->getFreezeMatrix());
	Math::Matrix4x4 fieldsChangeMatrix = getFieldsModificationMatrix(sm, changeMatrix);
	Math::Matrix4x4 scaleShearChangeMatrix = get2dRotationMatrix(changeMatrix.getTransform2d()).getInverse() * changeMatrix.rotation();
	Math::Matrix4x4 unitOffsetMatrix = getFreezeManagerPtr()->getUnitOffsetScaleMatrix();

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
	double ogOglOrientation;

	bool isHasFieldsOrientation = hasFieldsOrientation();

	if (!isHasFieldsOrientation)
		ogOglOrientation = applyUnitOffset(unitOffsetMatrix, ogOrientation);
	else
		ogOglOrientation = fieldsToOgl(sm, ogOrientation);

	Math::Matrix4x4 rotationMatrix = Math::Matrix4x4().rotateDegrees(ogOglOrientation);
	rotationMatrix = adjChangeMatrix * rotationMatrix;

	double oglOrientation = getAngle2d(rotationMatrix.getTransform2d());
	double orientation;

	if (!isHasFieldsOrientation)
		orientation = applyUnitOffset(unitOffsetMatrix.getInverse(), oglOrientation);
	else
		orientation = getAngle2d(getFieldsModificationMatrix(sm, rotationMatrix).getTransform2d());

	double referenceAngle = getAngle2d(changeMatrix.getTransform2d()) + ogOrientation;
	orientation = matchFullRotations(referenceAngle, orientation);

	//RADIUS
	Math::Point3d radius = Math::Point3d(radiusAttr->localValue(), 0, 0);

	if(!isHasFieldsOrientation)
		radius = adjChangeMatrix * radius;


	//LENGTH
	oldParentRotationMatrix.rotateDegrees(ogOglOrientation);
	newParentRotationMatrix.rotateDegrees(oglOrientation);
	Math::Matrix4x4 adjScaleShearChangeMatrix = newParentRotationMatrix.getInverse() * scaleShearChangeMatrix * oldParentRotationMatrix;
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

		if (!isHasFieldsOrientation)
			ogOglOrientation =  applyUnitOffset(unitOffsetMatrix, ogOrientation);
		else
			ogOglOrientation = fieldsToOgl(sm, ogOrientation);


		rotationMatrix = Math::Matrix4x4().rotateDegrees(ogOglOrientation);
		rotationMatrix = adjChangeMatrix * rotationMatrix;

		oglOrientation = getAngle2d(rotationMatrix.getTransform2d());

		if (!isHasFieldsOrientation)
			orientation = applyUnitOffset(unitOffsetMatrix.getInverse(), oglOrientation);
		else
			orientation = getAngle2d(getFieldsModificationMatrix(sm, rotationMatrix).getTransform2d());

		referenceAngle = getAngle2d(changeMatrix.getTransform2d()) + ogOrientation;
		orientation = matchFullRotations(referenceAngle, orientation);

		//RADIUS
		radius = Math::Point3d(radiusAttr->value(frameNo), 0, 0);

		if (!isHasFieldsOrientation)
			radius = adjChangeMatrix * radius;


		//LENGTH
		oldParentRotationMatrix.rotateDegrees(ogOglOrientation);
		newParentRotationMatrix.rotateDegrees(oglOrientation);
		adjScaleShearChangeMatrix = newParentRotationMatrix.getInverse() * scaleShearChangeMatrix * oldParentRotationMatrix;

		length = Math::Point3d(lengthAttr->value(frameNo), 0, 0);
		length = adjScaleShearChangeMatrix * length;

		setAttributes(pos3d, orientation, radius.toVector().length(), length.toVector().length(),
			curMacro, frameNo, curFrame == range.start);
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
	double length, CO_OrCommand& curMacro, double frameNo, bool isFirst)
{
	clampValues(position);

	FreezeManager* fm = getFreezeManagerPtr();

	BoneKeyframeData kfData = generateKeyframeData(frameNo, isFirst);

	const bool isSetPosition = kfData.position == KeyframeState::Keyframe ||
		(m_prevPosition != position && kfData.position == KeyframeState::PossibleKeyframe);
	const bool isSetLength = kfData.length == KeyframeState::Keyframe ||
		(m_prevLength != length && kfData.length == KeyframeState::PossibleKeyframe);
	const bool isSetOrientation = kfData.orientation == KeyframeState::Keyframe ||
		(m_prevOrientation != orientation && kfData.orientation == KeyframeState::PossibleKeyframe);
	const bool isSetRadius = kfData.radius == KeyframeState::Keyframe ||
		(m_prevRadius != radius && kfData.orientation == KeyframeState::PossibleKeyframe);

	//Saving the current values for the next keyframe
	m_prevPosition = position;
	m_prevLength = length; 
	m_prevOrientation = orientation;
	m_prevRadius = radius;


	if (fm->isExperimentalMode())
	{
		//C++
		if(isSetPosition)
			curMacro.add(Attr::Position2d::createSetValueCmd(m_offsetAttr, frameNo, position.x(), position.y()));
		if(isSetOrientation)
			curMacro.add(Attr::Double::createSetValueCmd(m_orientationAttr, frameNo, orientation));
		if(isSetRadius)
			curMacro.add(Attr::Double::createSetValueCmd(m_radiusAttr, frameNo, radius));
		if(isSetLength)
			curMacro.add(Attr::Double::createSetValueCmd(m_lengthAttr, frameNo, length));
	}
	else
	{
		//JS
		if (m_offsetAttr->useSeparate())
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				AttrData{ QLatin1String("offset.x"), position.x(), frameNo, isSetPosition },
				AttrData{ QLatin1String("offset.y"), position.y(), frameNo, isSetPosition });
		}
		else
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				Point2dAttrData{QLatin1String("offset"), Math::Point2d(position.x(),position.y()) , frameNo, isSetPosition });
		}
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			AttrData{ QLatin1String("orientation"), orientation, frameNo, isSetOrientation },
			AttrData{ QLatin1String("radius"), radius, frameNo, isSetRadius },
			AttrData{ QLatin1String("length"), length, frameNo, isSetLength });
	}
}


bool BoneModule::hasBoneParents() const
{
	MO_Node* parent = getModulePtr()->getParentNode();

	if (!parent)
		return false;

	return parent->keyword() == QLatin1String("BendyBoneModule");
}

bool BoneModule::hasFieldsOrientation() const
{
	return !hasBoneParents() || hasOffset(getModulePtr());
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


	//Need to include the whole chain's range for bone modules as sometimes it's necessary
	//to force keyframes
	MO_Node* parent = getModulePtr()->getParentNode();

	while (parent && (parent->keyword() == QLatin1String("BendyBoneModule")))
	{
		AT_DoubleAttr* attr = ::findAttribute<AT_DoubleAttr>(QStringLiteral("orientation"), parent);
		if (!attr)
			continue;

		if (attr->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (attr->getPrevKey(std::numeric_limits<int>::max(), &key))
			updateFrameRange(range, key);
		

		parent = parent->getParentNode();
	}

	return range;
}


BoneKeyframeData BoneModule::generateKeyframeData(double frameNo, bool isFirst)
{
	BoneKeyframeData kfData;
	//Check if there is a keyframe on the attributes

	if (isFirst && isComplexTransform())
	{
		kfData.position = KeyframeState::Keyframe;
		kfData.length = KeyframeState::Keyframe;
		kfData.orientation = KeyframeState::Keyframe;
		kfData.radius = KeyframeState::Keyframe;
		return kfData;
	}

	bool isCtrlPntPosition = false;
	bool isCtrlPntLength = false;
	bool isCtrlPntOrientation = false;
	bool isCtrlPntRadius = false;

	Math::Point2d tempPoint;

	//There have been changes to the getValue function between H24 and H27.
	//When making changes here, all supported versions need to be taken into account
	m_offsetAttr->getValue(frameNo, tempPoint, &isCtrlPntPosition);

	m_lengthAttr->value(frameNo, &isCtrlPntLength);
	m_orientationAttr->value(frameNo, &isCtrlPntOrientation);
	m_radiusAttr->value(frameNo, &isCtrlPntRadius);

	bool isAdjustKeyframe = isCtrlPntOrientation || isParentKeyframe(frameNo)
		|| getFreezeManagerPtr()->isSetInbetweenKfMode();

	isAdjustKeyframe = isAdjustKeyframe && isComplexTransform();


	if (isCtrlPntLength)
		kfData.length = KeyframeState::Keyframe;
	else if (isAdjustKeyframe)
		kfData.length = KeyframeState::PossibleKeyframe;
	else
		kfData.length = KeyframeState::NoKeyframe;

	if (isCtrlPntOrientation)
		kfData.orientation = KeyframeState::Keyframe;
	else if (isAdjustKeyframe)
		kfData.orientation = KeyframeState::PossibleKeyframe;
	else
		kfData.orientation = KeyframeState::NoKeyframe;

	//Radius and position are not being influenced by current bone's orientation
	if (isCtrlPntRadius)
		kfData.radius = KeyframeState::Keyframe;
	else if (isParentKeyframe(frameNo) || getFreezeManagerPtr()->isSetInbetweenKfMode())
		kfData.radius = KeyframeState::PossibleKeyframe;
	else
		kfData.radius = KeyframeState::NoKeyframe;

	if (isCtrlPntPosition)
		kfData.position = KeyframeState::Keyframe;
	else if (isParentKeyframe(frameNo) || getFreezeManagerPtr()->isSetInbetweenKfMode())
		kfData.position = KeyframeState::PossibleKeyframe;
	else
		kfData.position = KeyframeState::NoKeyframe;

	return kfData;
}


bool BoneModule::isParentKeyframe(double frameNo)
{
	bool isKeyframe = false;

	MO_Node* parent = getModulePtr()->getParentNode();

	while (parent && parent->keyword() == QLatin1String("BendyBoneModule"))
	{
		AT_DoubleAttr* attr = ::findAttribute<AT_DoubleAttr>(QStringLiteral("orientation"), parent);
		if (!attr)
			continue;

		if (attr)
			attr->value(frameNo, &isKeyframe);

		if (isKeyframe)
			return true;

		parent = parent->getParentNode();
	}

	return false;
}