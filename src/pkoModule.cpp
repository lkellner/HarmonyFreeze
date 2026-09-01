#include "pkoModule.h"

#include <SceneCore/module/MO_PortTransform.h>
#include <GraphicCore/CinematicChain/CC_Transformation.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
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
	const MO_Node::InPorts inPorts = getModulePtr()->getInPorts();

	if (inPorts.size() < 2)
	{
		printf("invalid port for incoming matrix\n");
		return;
	}

	MO_Module* port1srcModule = inPorts[1]->realSrcNode();

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

	MO_Module* port0srcModule = inPorts[1]->realSrcNode();

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

void PkoModule::readjustSecondary()
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
	//curve deformers
	Math::Matrix4x4 offsetMatrix = getIncomingMatrix(1, 1, true);
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

	//There have been changes to the getValue function between H24 and H27.
	//When making changes here, all supported versions need to be taken into account

	attr->getValue(frameNo, tempPoint, &isPosCtrlPnt); 

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