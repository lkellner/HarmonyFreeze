#include "pkoModule.h"

#include <SceneCore/module/MO_PortTransform.h>
#include <GraphicCore/CinematicChain/CC_Transformation.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/attribute/AT_Position3dAttr.h>
#include <SceneCore/attribute/AT_Rotation3dAttr.h>
#include <SceneCore/attribute/AT_Scale3dAttr.h>

#include <BaseCore/maths/MT_Point4d.h>


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

	identifyTransformationType();
}



void PkoModule::identifyTransformationType()
{
	MO_Module* port1srcModule = getSourceModule(getModulePtr(), 1);

	if (!port1srcModule)
	{
		//Port 1 has no input
		m_transformationType = TransformationType::Simple;
		return;
	}

	MO_Module* freezeModule = getFreezeManagerPtr()->getFreezePegPtr();
	if (!freezeModule)
		return;

	if (!freezeModule->isLinkedTo(port1srcModule)) //Order matters
	{
		//Port 1 has an input but is not connected to the freezePeg
		m_transformationType = TransformationType::SingleFreeze;
		return;
	}

	MO_Module* port0srcModule = getSourceModule(getModulePtr(), 0);

	if (port0srcModule && freezeModule->isLinkedTo(port0srcModule))
	{
		//Both ports are connected to the freezePeg
		m_transformationType = TransformationType::DoubleFreeze;
		return;
	}

	//Only port 1's input is connected to the freezePeg
	//In the case of port 0 not having any input at all the point kinematic output is disregarded by Harmony
	m_transformationType = TransformationType::Simple;
}

Math::Matrix4x4 PkoModule::calculateChangeMatrix(double frameNo)
{
	/*
	How point kinematic outputs work:
	O := port 1 matrix (offset)
	D := port 0 matrix (only one in case of a quadmap,
		for other deformers consider it the transformation matrix that will be applied to the pivot to be transformed)
	pOgl := any pivot of the point kinematic output in ogl coordinates
	pWorld := the pivot's world ogl coordinates, position on screen

	pWorld = D*O*pOgl

	After applying the freeze peg, the pivots world position is supposed to be the identical with its prior position.
	C := Change matrix that needs to be applied to the pivot
	Onew := the offset matrix after applying the freeze
	Dnew := the deformation matrix after applying the freeze
	F := the freeze matrix

	Case 0: There is no port 1
		Onew = O * F.getInverse();
		pWorld = O * pOgl = O * F.getInverse() * C * pOgl;
		C = F;

	Case 1: There is a port 1 but only port 0 is connected to the freeze peg
		Onew = O;
		Dnew = D * F.getInverse();

		pWorld = D * O * pOgl = D * F.getInverse() * O * C * pOgl;
		C = O.getInverse() * F * O;

	Case 2: There is a port 1 and both ports are connected to the freeze peg

		Onew = O * F.getInverse();
		Dnew = D * F.getInverse();

		pWorld = D * O * pOgl = D * F.getInverse() * O * F.getInverse() * C * pOgl;
		C = O.getInverse() * F * O;

	Case 3: There is a port 1 and it's the only port connected to the freeze peg

		Onew = O * F.getInverse();
		Dnew = D;

		pWorld = D * O * pOgl = D * O * F.getInverse() * C * pOgl;
		C = F;

	*/


	FreezeManager* fm = getFreezeManagerPtr();

	//TODO: Will need to check if "true" is the correct parameter to pass here 
	//after quadmaps have been implemented. (It doesn't seem to matter with 
	//curve deformers)

	//This module depends on all the attributes being set at once in the end
	//if this is no longer true, the offsetMatrix would need to be saved in the
	//constructor
	Math::Matrix4x4 offsetMatrix = getIncomingMatrix(1, frameNo, true);
	Math::Matrix4x4 oglChangeMatrix;

	switch (m_transformationType)
	{
	case TransformationType::Simple:
		//Either there is no port 1 input OR port 1 is connected to the freeze peg and port 0 isn't
		oglChangeMatrix = fm->getFreezeMatrix();
		break;

	case TransformationType::SingleFreeze:
		//There is a port 1 input but it's not connected to the freeze peg
		oglChangeMatrix = offsetMatrix.getInverse() * fm->getFreezeMatrix() * offsetMatrix;
		break;

	case TransformationType::DoubleFreeze:
		//Both in ports are connected to the freeze peg
		oglChangeMatrix = fm->getFreezeMatrix() * offsetMatrix.getInverse() * fm->getFreezeMatrix() * offsetMatrix;
		break;
	}

	return getFieldsModificationMatrix(getModulePtr()->sceneMetrics(), oglChangeMatrix);
}


void PkoModule::readjustSecondary()
{
	setComplexTransform(defineMatrixComplexity(getFreezeManagerPtr()->getFreezeMatrix(), false));

	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	processPivot(m_pivot01Attr, QLatin1String("pivot1"), *curMacro);
	processPivot(m_pivot02Attr, QLatin1String("pivot2"), *curMacro);
	processPivot(m_pivot03Attr, QLatin1String("pivot3"), *curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));
}


void PkoModule::processPivot(AT_Position2dAttr* pivotAttr, QString pivotKeyword, CO_OrCommand& curMacro)
{
	//In the case of DoubleFreeze or SingleFreeze this might not be accurate for the static values. 
	//However in those cases frame 1 will have a forced keyframe
	Math::Matrix4x4 changeMatrix = calculateChangeMatrix(1);

	Math::Point2d position;
	pivotAttr->getLocalValue(position);
	Math::Point3d pos3d = Math::Point3d(position);

	pos3d = changeMatrix * pos3d;

	setStaticAttributes(pos3d, pivotAttr, pivotKeyword, curMacro);

	FrameRange range = getFrameRange();
	//TODO: in case of non simple transformtype use freeze managers range~!

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		changeMatrix = calculateChangeMatrix(curFrame);
		pivotAttr->getValue(curFrame, position);
		pos3d = Math::Point3d(position);

		pos3d = changeMatrix * pos3d;

		KeyframeState keyframeState = generateKeyframeData(pivotAttr, curFrame, curFrame == range.start);
		setAttributes(pos3d, pivotAttr, pivotKeyword, curMacro, keyframeState, curFrame);
	}
}

void PkoModule::setStaticAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, CO_OrCommand& curMacro)
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


void PkoModule::setAttributes(Math::Point3d position, AT_Position2dAttr* attr, QString attributeKeyword, CO_OrCommand& curMacro, 
	KeyframeState keyframeState, double frameNo)
{
	clampValues(position);

	FreezeManager* fm = getFreezeManagerPtr();


	if (keyframeState == KeyframeState::NoKeyframe || (keyframeState == KeyframeState::PossibleKeyframe && m_prevPos == position))
		return;

	m_prevPos = position;

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


KeyframeState PkoModule::generateKeyframeData(AT_Position2dAttr* attr, double frameNo, bool isFirst)
{
	Math::Point2d tempPoint; 
	bool isPosCtrlPnt = false;

	//There have been changes to the getValue function between H24 and H27.
	//When making changes here, all supported versions need to be taken into account
	attr->getValue(frameNo, tempPoint, &isPosCtrlPnt);

	if (isPosCtrlPnt)
	{
		printf("isPosCtrlPnt %f\n", frameNo);
		return KeyframeState::Keyframe;
	}
		


	//Values don't depend on port 1 input
	if (!isComplexTransform() || m_transformationType == TransformationType::Simple)
	{
		printf("early no keyframe %f\n", frameNo);
		return KeyframeState::NoKeyframe;
	}
		

	//The cases below are either TransformationType::DoubleFreeze or TransformationType::SingleFreeze
	// as well as isComplexTransform()
	if (isFirst)
	{
		printf("is first %f\n", frameNo);
		return KeyframeState::Keyframe;
	}
		

	//Keyframes will be set if the value changes compared to the previous frame
	if (hasComplexPort1Parent() || !hasNoParentKeyframe(frameNo) || getFreezeManagerPtr()->isSetInbetweenKfMode())
	{
		printf("possible keyframe %f\n", frameNo);
		return KeyframeState::PossibleKeyframe;
	}
		

	//The point kinematic output doesn't have a complex parent chain, neither the port 1 parent nor the ptk itself have keyframes
	//InbetweenKfMode is turned off
	return KeyframeState::NoKeyframe;
}

bool PkoModule::hasComplexPort1Parent()
{
	MO_Module* port1srcModule = getSourceModule(getModulePtr(), 1);

	if (!port1srcModule)
	{
		//There is no parent on port 1
		return false;
	}

	if (port1srcModule->keyword() != QLatin1String("PEG"))
	{
		//Only pegs are considered simple for now
		//Might be changed in the future
		return true;
	}

	const MO_Node::InPorts parentInPorts = port1srcModule->getInPorts();

	return (parentInPorts.size() == 0 ? false : true);
}

bool PkoModule::hasNoParentKeyframe(double frameNo)
{
	//TODO: there is quite a bit of intersection with transformationModule's generateKeyframeData
	//It might be worth consolidating the two

	MO_Module* parent = getSourceModule(getModulePtr(), 1);

		if (!parent)
		{
			//There is no parent on port 1
			return true;
		}

	if (parent->keyword() != QLatin1String("PEG"))
	{
		//Only pegs will be checked for now
		//Might be changed in the future
		return false;
	}

	bool isConstSeg;


	//POSITION

	Math::Point3d tempPos;
	bool isPosCtrlPnt;

	AT_Position3dAttr* posAttr = ::findAttribute<AT_Position3dAttr>(QLatin1String("POSITION"), parent);

	if(posAttr)
		posAttr->getValue(frameNo, tempPos, &isPosCtrlPnt, &isConstSeg);


	//ROTATION

	Math::Angle3d tempAngle;
	double tempV;
	bool isRotCtrlPnt;

	AT_Rotation3dAttr*  rotAttr = ::findAttribute<AT_Rotation3dAttr>(QLatin1String("ROTATION"), parent);

	if(rotAttr)
		rotAttr->getValue(frameNo, tempAngle, &tempV, &isRotCtrlPnt, &isConstSeg);


	//SCALE


	bool isConstSegX = false;
	bool isConstSegY = false;
	bool isConstSegZ = false;

	bool isKeyframeX = false;
	bool isKeyframeY = false;
	bool isKeyframeZ = false;

	double tempScaleX;
	double tempScaleY;
	double tempScaleZ;

	AT_Scale3dAttr* scaleAttr = ::findAttribute<AT_Scale3dAttr>(QLatin1String("SCALE"), parent);

	if(scaleAttr)
		scaleAttr->getValue(frameNo, tempScaleX, tempScaleY, tempScaleZ, &isKeyframeX, &isKeyframeY, &isKeyframeZ,
			&isConstSegX, &isConstSegY, &isConstSegZ);


	//SKEW
	
	bool isSkewCtrlPnt = false;

	AT_DoubleAttr* skewAttr = ::findAttribute<AT_DoubleAttr>(QLatin1String("SKEW"), parent);
	
	if(skewAttr)
		skewAttr->value(frameNo, &isSkewCtrlPnt, &isConstSeg);


	return isPosCtrlPnt || isRotCtrlPnt || isKeyframeX || isKeyframeY || isKeyframeZ || isSkewCtrlPnt;
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