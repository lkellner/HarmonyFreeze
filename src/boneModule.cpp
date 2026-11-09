#include "boneModule.h"

using namespace Qt::StringLiterals;

bool hasOffset(const MO_Node* node)
{
	if (!node)
		return false;

	auto offsetAttr = ::findAttribute<AT_Position2dAttr>("offset"_L1, node);
	auto restOffsetAttr = ::findAttribute<AT_Position2dAttr>("restOffset"_L1, node);

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
	, m_restOffsetAttr(findAttribute<AT_Position2dAttr>("restOffset"_L1))
	, m_restRadiusAttr(findAttribute<AT_DoubleAttr>("restRadius"_L1))
	, m_restLengthAttr(findAttribute<AT_DoubleAttr>("restLength"_L1))
	, m_restOrientationAttr(findAttribute<AT_DoubleAttr>("restOrientation"_L1))
	, m_offsetAttr(findAttribute<AT_Position2dAttr>("offset"_L1))
	, m_radiusAttr(findAttribute<AT_DoubleAttr>("radius"_L1))
	, m_lengthAttr(findAttribute<AT_DoubleAttr>("length"_L1))
	, m_orientationAttr(findAttribute<AT_DoubleAttr>("orientation"_L1))
{
	m_regionOfInfluence = std::make_unique<RegionOfInfluence>(getFreezeManager(), modulePtr);

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
	if (!m_regionOfInfluence)
		throw std::runtime_error("missing attribute set: 'm_regionOfInfluence' for " + modulePtr->qualifiedName().toStdString());
}

void BoneModule::readjustSecondary()
{
	auto curMacro = std::make_shared<CO_OrCommand>();

	const Math::Matrix4x4 freezeMatrix = getFreezeManagerPtr()->getFreezeMatrix();
	m_freezeMatrixComplexity = getFreezeManagerPtr()->getMatrixComplexity(false);

	processAttributeSet(*curMacro, true);
	processAttributeSet(*curMacro, false);
	m_regionOfInfluence->readjustSecondary(*curMacro, freezeMatrix);
	
	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}

double BoneModule::getStaticChainRotation(const bool isRest) const
{
	const auto orientationKeyword = isRest
		? "restOrientation"_L1
		: "orientation"_L1;

	return getRotationImpl(orientationKeyword,
		[](const AT_DoubleAttr& a) { return a.localValue(); });
}

double BoneModule::getChainRotation(const double frameNo) const
{
	return getRotationImpl("orientation"_L1,
		[frameNo](const AT_DoubleAttr& a) { return a.value(frameNo); });
}

template <typename ValueFunc>
double BoneModule::getRotationImpl(const QLatin1StringView orientationKeyword, ValueFunc&& valFunc) const
{
	const FreezeManager* fm = getFreezeManagerPtr();

	const MO_Module* mod = getModulePtr();

	const MO_Node* parent = mod->getParentNode();

	double rotation = 0.0;

	while (parent && parent->keyword() == "BendyBoneModule"_L1)
	{
		AT_DoubleAttr* attr = ::findAttribute<AT_DoubleAttr>(orientationKeyword, parent);
		if (!attr)
			continue;

		if (parent->getParentNode() && parent->getParentNode()->keyword() == "BendyBoneModule"_L1
			&& !hasOffset(parent))
		{
			//Not the first bone in the chain
			rotation += convertRotation(fm->getUnitOffsetScaleMatrix(), valFunc(*attr));
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

void BoneModule::processAttributeSet(CO_OrCommand& curMacro, const bool isRest)
{
	const AT_Position2dAttr* positionAttr = isRest ? m_restOffsetAttr : m_offsetAttr;
	const AT_DoubleAttr* radiusAttr = isRest ? m_restRadiusAttr : m_radiusAttr;
	const AT_DoubleAttr* lengthAttr = isRest ? m_restLengthAttr : m_lengthAttr;
	const AT_DoubleAttr* orientationAttr = isRest ? m_restOrientationAttr : m_orientationAttr;

	SC_SceneMetrics* sm = getModulePtr()->sceneMetrics();

	const Math::Matrix4x4 changeMatrix = convertToProjectionMatrix(getFreezeManagerPtr()->getFreezeMatrix());
	Math::Matrix4x4 fieldsChangeMatrix = oglToFieldsModificationMatrix(sm, changeMatrix);
	Math::Matrix4x4 scaleShearChangeMatrix = get2dRotationMatrix(changeMatrix.getTransform2d()).getInverse() * changeMatrix.rotation();
	const Math::Matrix4x4 unitOffsetMatrix = getFreezeManagerPtr()->getUnitOffsetScaleMatrix();

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
		fieldsChangeMatrix = oglToFieldsModificationMatrix(sm, adjChangeMatrix);
	}


	//OFFSET
	Math::Point2d position;
	positionAttr->getLocalValue(position);
	auto pos3d = Math::Point3d(position);
	pos3d = fieldsChangeMatrix * pos3d;

	if(!hasBoneParents() || !hasOffset(getModulePtr()))
		clampValues(pos3d);

	//ORIENTATION
	double ogOrientation = orientationAttr->localValue();
	double ogOglOrientation;

	bool isHasFieldsOrientation = hasFieldsOrientation();

	if (!isHasFieldsOrientation)
		ogOglOrientation = convertRotation(unitOffsetMatrix, ogOrientation);
	else
		ogOglOrientation = fieldsToOgl(sm, ogOrientation);

	auto rotationMatrix = Math::Matrix4x4().rotateDegrees(ogOglOrientation);
	rotationMatrix = adjChangeMatrix * rotationMatrix;

	double oglOrientation = getAngle2d(rotationMatrix.getTransform2d());
	double orientation;

	if (!isHasFieldsOrientation)
		orientation = convertRotation(unitOffsetMatrix.getInverse(), oglOrientation);
	else
		orientation = getAngle2d(oglToFieldsModificationMatrix(sm, rotationMatrix).getTransform2d());

	double referenceAngle = getAngle2d(changeMatrix.getTransform2d()) + ogOrientation;
	orientation = matchFullRotations(referenceAngle, orientation);

	//RADIUS
	auto radius = Math::Point3d(radiusAttr->localValue(), 0, 0);

	if(!isHasFieldsOrientation)
		radius = adjChangeMatrix * radius;


	//LENGTH
	oldParentRotationMatrix.rotateDegrees(ogOglOrientation);
	newParentRotationMatrix.rotateDegrees(oglOrientation);
	Math::Matrix4x4 adjScaleShearChangeMatrix = newParentRotationMatrix.getInverse() * scaleShearChangeMatrix * oldParentRotationMatrix;

	Math::Point3d length = Math::Point3d(lengthAttr->localValue(), 0, 0);
	length = adjScaleShearChangeMatrix * length;
	
	setStaticAttributes({ pos3d, orientation, radius.toVector().length(), length.toVector().length() }, curMacro, isRest);

	//Rest attributes only contain static values
	if (isRest)
		return;

	const FrameRange range = getFrameRange();

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
			fieldsChangeMatrix = oglToFieldsModificationMatrix(sm, adjChangeMatrix);
		}

		//OFFSET
		positionAttr->getValue(frameNo, position);
		pos3d = Math::Point3d(position);
		pos3d = fieldsChangeMatrix * pos3d;

		if (!hasBoneParents() || !hasOffset(getModulePtr()))
			clampValues(pos3d);

		//ORIENTATION
		ogOrientation = orientationAttr->value(frameNo);

		if (!isHasFieldsOrientation)
			ogOglOrientation = convertRotation(unitOffsetMatrix, ogOrientation);
		else
			ogOglOrientation = fieldsToOgl(sm, ogOrientation);


		rotationMatrix = Math::Matrix4x4().rotateDegrees(ogOglOrientation);
		rotationMatrix = adjChangeMatrix * rotationMatrix;

		oglOrientation = getAngle2d(rotationMatrix.getTransform2d());

		if (!isHasFieldsOrientation)
			orientation = convertRotation(unitOffsetMatrix.getInverse(), oglOrientation);
		else
			orientation = getAngle2d(oglToFieldsModificationMatrix(sm, rotationMatrix).getTransform2d());

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

		setAttributes({ pos3d, orientation, radius.toVector().length(), length.toVector().length() },
			curMacro, frameNo, curFrame == range.start);
	}
}

void BoneModule::setStaticAttributes(const BoneData boneData, CO_OrCommand& curMacro, const bool isRest) const
{
	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		AT_Position2dAttr* positionAttr = isRest ? m_restOffsetAttr : m_offsetAttr;
		AT_DoubleAttr* radiusAttr = isRest ? m_restRadiusAttr : m_radiusAttr;
		AT_DoubleAttr* lengthAttr = isRest ? m_restLengthAttr : m_lengthAttr;
		AT_DoubleAttr* orientationAttr = isRest ? m_restOrientationAttr : m_orientationAttr;

		curMacro.add(Attr::Position2d::createSetLocalValueCmd(positionAttr, boneData.position.x(), boneData.position.y()));
		curMacro.add(Attr::Double::createSetLocalValueCmd(orientationAttr, boneData.orientation));
		curMacro.add(Attr::Double::createSetLocalValueCmd(radiusAttr, boneData.radius));
		curMacro.add(Attr::Double::createSetLocalValueCmd(lengthAttr, boneData.length));
	}
	else
	{
		//JS

		const QString offsetJS = isRest ? u"restoffset"_s : u"offset"_s;
		const QString orientationJS = isRest ? u"restorientation"_s : u"orientation"_s;
		const QString radiusJS = isRest ? u"restradius"_s : u"radius"_s;
		const QString lengthJS = isRest ? u"restlength"_s : u"length"_s;

		//Similar to transformation module, can't set static value of combined paths
		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ offsetJS + u".x"_s, boneData.position.x() },
			StaticAttrData{ offsetJS + u".y"_s, boneData.position.y() },
			StaticAttrData{ orientationJS, boneData.orientation },
			StaticAttrData{ radiusJS, boneData.radius},
			StaticAttrData{ lengthJS, boneData.length });
	}
}

void BoneModule::setAttributes(const BoneData boneData, CO_OrCommand& curMacro,
	const double frameNo, const bool isFirst)
{
	FreezeManager* fm = getFreezeManagerPtr();

	BoneModule::KeyframeData kfData = generateKeyframeData(boneData, frameNo, isFirst);

	if (fm->isExperimentalMode())
	{
		//C++

		//Attributes are being split to be able to set the proper isConstSeg value
		if (m_offsetAttr->useSeparate())
		{
			if(kfData.posX)
				curMacro.add(Attr::Double::createSetValueCmd(m_offsetAttr->separateX(), frameNo,
					boneData.position.x(), kfData.isConstSegPosX, true));
			if(kfData.posY)
				curMacro.add(Attr::Double::createSetValueCmd(m_offsetAttr->separateY(), frameNo,
					boneData.position.y(), kfData.isConstSegPosY, true));
		}
		else if(kfData.pos2D)
		{
			curMacro.add(Attr::Position2d::createSetValueCurrentFrameCmd(getModulePtr()->sceneCommon(), m_offsetAttr,
				frameNo, boneData.position.x(), boneData.position.y(), { kfData.isConstSegPos2D, true, false, false }));
		}

		if (kfData.orientation)
			curMacro.add(Attr::Double::createSetValueCmd(m_orientationAttr, frameNo,
				boneData.orientation, kfData.isConstSegOrientation, true));
		if (kfData.radius)
			curMacro.add(Attr::Double::createSetValueCmd(m_radiusAttr, frameNo, boneData.radius, kfData.isConstSegRadius, true));
		if (kfData.length)
			curMacro.add(Attr::Double::createSetValueCmd(m_lengthAttr, frameNo, boneData.length, kfData.isConstSegLength, true));
	}
	else
	{
		//JS
		if (kfData.posX || kfData.posY || kfData.pos2D)
		{
			if (m_offsetAttr->useSeparate())
			{
				fm->applyAttributes(getModulePtr()->qualifiedName(),
					AttrData{ u"offset.x"_s, boneData.position.x(), frameNo, kfData.posX },
					AttrData{ u"offset.y"_s, boneData.position.y(), frameNo, kfData.posY });
			}
			else
			{
				fm->applyAttributes(getModulePtr()->qualifiedName(),
					Point2dAttrData{ u"offset"_s, Math::Point2d(boneData.position.x(),
						boneData.position.y()) , frameNo, kfData.pos2D });
			}
		}
		if (kfData.orientation || kfData.radius || kfData.length)
		{
			fm->applyAttributes(getModulePtr()->qualifiedName(),
				AttrData{ u"orientation"_s, boneData.orientation, frameNo, kfData.orientation },
				AttrData{ u"radius"_s, boneData.radius, frameNo, kfData.radius },
				AttrData{ u"length"_s, boneData.length, frameNo, kfData.length });
		}
	}
}

bool BoneModule::hasBoneParents() const
{
	const MO_Node* parent = getModulePtr()->getParentNode();

	if (!parent)
		return false;

	return parent->keyword() == "BendyBoneModule"_L1;
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

	while (parent && (parent->keyword() == "BendyBoneModule"_L1))
	{
		AT_DoubleAttr* attr = ::findAttribute<AT_DoubleAttr>("orientation"_L1, parent);
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

BoneModule::KeyframeData BoneModule::generateKeyframeData(const BoneData& boneData, const double frameNo, const bool isFirst)
{
	BoneModule::KeyframeData kfData;

	bool isCtrlPntPos2D = false;
	bool isCtrlPntPosX = false;
	bool isCtrlPntPosY = false;
	bool isCtrlPntLength = false;
	bool isCtrlPntOrientation = false;
	bool isCtrlPntRadius = false;

	Math::Point2d tempPoint;

	//There have been changes to the getValue function between H24 and H27.
	//When making changes here, all supported versions need to be taken into account
	m_offsetAttr->getValue(frameNo, tempPoint, &isCtrlPntPos2D, &kfData.isConstSegPos2D);
	//Getting the separate values only for the const Seg positions
	m_offsetAttr->separateX()->value(frameNo, &isCtrlPntPosX, &kfData.isConstSegPosX);
	m_offsetAttr->separateY()->value(frameNo, &isCtrlPntPosY, &kfData.isConstSegPosY);


	m_lengthAttr->value(frameNo, &isCtrlPntLength, &kfData.isConstSegLength);
	m_orientationAttr->value(frameNo, &isCtrlPntOrientation, &kfData.isConstSegOrientation);
	m_radiusAttr->value(frameNo, &isCtrlPntRadius, &kfData.isConstSegRadius);

	bool isForceKeyframe = isFirst && isComplexTransform();
	bool isAdjustKeyframe = isCtrlPntOrientation || isParentKeyframe(frameNo)
		|| getFreezeManagerPtr()->isSetInbetweenKfMode();

	isAdjustKeyframe = isAdjustKeyframe && isComplexTransform();

	kfData.pos2D = isCtrlPntPos2D || isForceKeyframe || (isAdjustKeyframe && boneData.position != m_prevPosition);
	kfData.posX = isCtrlPntPosX || isForceKeyframe || 
		((isAdjustKeyframe || kfData.posY) && boneData.position.x() != m_prevPosition.x());
	kfData.posY = isCtrlPntPosY || isForceKeyframe || 
		((isAdjustKeyframe || kfData.posX) && boneData.position.y() != m_prevPosition.y());
	
	kfData.orientation = isCtrlPntOrientation || isForceKeyframe ||
		(isAdjustKeyframe && boneData.orientation != m_prevOrientation);
	kfData.length = isCtrlPntLength || isForceKeyframe || (isAdjustKeyframe && boneData.length != m_prevLength);
	kfData.radius = isCtrlPntRadius || isForceKeyframe || (isAdjustKeyframe && boneData.radius != m_prevRadius);


	//Saving the current values for the next keyframe
	m_prevPosition = boneData.position;
	m_prevLength = boneData.length;
	m_prevOrientation = boneData.orientation;
	m_prevRadius = boneData.radius;

	return kfData;
}

bool BoneModule::isParentKeyframe(const double frameNo) const
{
	bool isKeyframe = false;

	MO_Node* parent = getModulePtr()->getParentNode();

	while (parent && parent->keyword() == "BendyBoneModule"_L1)
	{
		AT_DoubleAttr* attr = ::findAttribute<AT_DoubleAttr>("orientation"_L1, parent);
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