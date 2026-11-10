#include "pkoModule.h"

#include <SceneCore/attribute/AT_Position3dAttr.h>
#include <SceneCore/attribute/AT_Rotation3dAttr.h>
#include <SceneCore/attribute/AT_Scale3dAttr.h>

using namespace Qt::StringLiterals;

PkoModule::PkoModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: PointModuleBase(std::move(freezeManager), modulePtr, moduleType)
{
	const AT_AttrList attrList = getAttributeList();

	for (auto& attr : attrList)
	{
		if (attr._pAttr->typeName() == "POSITION_2D"_L1)
		{
			AT_Position2dAttr* pointAttr = dynamic_cast<AT_Position2dAttr*>(attr._pAttr);

			if (!pointAttr)
				continue;

			addTransformPoint(std::make_unique<TransformPoint>(pointAttr->xmlKeyword(), pointAttr, true));
		}
	}

	identifyTransformationType();
}

void PkoModule::identifyTransformationType()
{
	MO_Module* port1srcModule = getModulePtr()->inputModule(1);

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

	MO_Module* port0srcModule = getModulePtr()->inputModule(0);

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

Math::Matrix4x4 PkoModule::calculateChangeMatrix(const double frameNo) const
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

	//This module depends on all the attributes being set at once in the end
	//if this is no longer true, the offsetMatrix would need to be saved in the
	//constructor

	//Passing true or false doesn't seem to matter for curve deformers. Quadmaps are
	//ignoring the point kinematic output alltogether.
	const Math::Matrix4x4 offsetMatrix = getIncomingMatrix(1, frameNo, true);
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

	return oglToFieldsModificationMatrix(getModulePtr()->sceneMetrics(), oglChangeMatrix);
}

void PkoModule::readjustSecondary()
{
	setMatrixComplexity();

	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	for (const auto& point : m_transformPoints)
		processPivot(point.get(), *curMacro);

	getFreezeManagerPtr()->addCommand(std::move(curMacro));

}

void PkoModule::processPivot(TransformPoint* point, CO_OrCommand& curMacro)
{
	//In the case of DoubleFreeze or SingleFreeze this might not be accurate for the static values. 
	//However in those cases frame 1 will have a forced keyframe
	Math::Matrix4x4 changeMatrix = calculateChangeMatrix(1);

	Math::Point2d position;
	point->posAttr->getLocalValue(position);
	Math::Point3d pos3d = Math::Point3d(position);

	pos3d = changeMatrix * pos3d;

	setStaticAttributes(point, pos3d, curMacro);
	m_prevPos = pos3d;

	FrameRange range = getFrameRange();
	
	//A larger range of keyframes needs to be considered when there is an offset matrix
	if (m_transformationType != TransformationType::Simple)
	{
		//getIncomingFrameRange() can only be used with H27 and higher
		if constexpr (supportsKeysRange())
		{
			getIncomingFrameRange(getModulePtr()->inputModule(1), range);
		}
		else
		{
			//This might not detect all keyframes if port 1 not connected to the Freeze Peg
			range = getFreezeManagerPtr()->getFrameRange();
		}
	}


	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		KeyframeData kfData = generateKeyframeData(point, curFrame, curFrame == range.start);

		if (!kfData.isProcessFrame)
			continue;

		changeMatrix = calculateChangeMatrix(curFrame);
		point->posAttr->getValue(curFrame, position);
		pos3d = Math::Point3d(position);

		pos3d = changeMatrix * pos3d;

		setAttributes(point, pos3d, kfData, curMacro, curFrame);
		
		//This can only be changed after the keyframe data has been generated
		m_prevPos = pos3d;
	}
}

PointModuleBase::KeyframeData PkoModule::generateKeyframeData(const TransformPoint* point,
	const double frameNo, const bool isFirst) const
{
	KeyframeData kfData;
	kfData.isSeparate = point->posAttr->useSeparate();

	Math::Point2d tempPos;

	//There have been changes to the getValue function between H24 and H27.
	//When making changes here, all supported versions need to be taken into account
	if (kfData.isSeparate)
	{
		point->posAttr->separateX()->value(frameNo, &kfData.posX, &kfData.isConstSegX);
		point->posAttr->separateY()->value(frameNo, &kfData.posY, &kfData.isConstSegY);
	}
	else
	{
		getPos2DValues(*(point->posAttr), frameNo, tempPos, kfData.pos2D, kfData.isConstSeg2D);
	}

	kfData.isForceKeyframe = isFirst && isComplexTransform();
	kfData.isAdjustKeyframe = getFreezeManagerPtr()->isSetInbetweenKfMode()
		|| (m_transformationType != TransformationType::Simple && (hasComplexPort1Parent() || !hasNoParentKeyframe(frameNo)));

	kfData.isAdjustKeyframe = kfData.isAdjustKeyframe && isComplexTransform();

	kfData.isProcessFrame = kfData.isForceKeyframe || kfData.isAdjustKeyframe || kfData.pos2D || kfData.posX || kfData.posY;
	/*Keyframes are being set if
	*- there already is a keyframe
	*-it's the first frame and the freeze is complex
	*-the freeze is complex and the transformation type is not simple, the value doesn't match the previous one and
	*	-there is either a compexPort1Parent, the parent has a keyframe or inbetweenmode is turned on
	*-the attribute is set to separate, and the partner attribute has a keyframe and the value doesn't match the previous one
	*/

	return kfData;
}

bool PkoModule::hasComplexPort1Parent() const
{
	MO_Module* port1srcModule = getModulePtr()->inputModule(1);

	if (!port1srcModule)
	{
		//There is no parent on port 1
		return false;
	}

	if (port1srcModule->keyword() != "PEG"_L1)
	{
		//Only pegs are considered simple for now
		//Might be changed in the future
		return true;
	}


	return (port1srcModule->getParentNode() ? true : false);
}

bool PkoModule::hasNoParentKeyframe(double frameNo) const
{
	//This is somewhat similar to generateKeyframeData in TransformationModule.
	//However for now the decision was made not to consolidate the two as this is
	//a somewhat special case for the PKO module that would create a lot of overhead
	//for the far more used TransformationModule

	MO_Module* parent = getModulePtr()->inputModule(1);

	if (!parent)
	{
		//There is no parent on port 1
		return true;
	}

	if (parent->keyword() != "PEG"_L1)
	{
		//Only pegs will be checked for now
		//Might be changed in the future
		return false;
	}

	//POSITION

	Math::Point3d tempPos;
	bool isPosCtrlPnt = false;

	AT_Position3dAttr* posAttr = ::findAttribute<AT_Position3dAttr>("POSITION"_L1, parent);

	if (posAttr)
		posAttr->getValue(frameNo, tempPos);


	//ROTATION

	Math::Angle3d tempAngle;
	bool isRotCtrlPnt = false;
	bool isRotXCtrlPnt = false;
	bool isRotYCtrlPnt = false;
	bool isRotZCtrlPnt = false;

	AT_Rotation3dAttr* rotAttr = ::findAttribute<AT_Rotation3dAttr>("ROTATION"_L1, parent);

	if (rotAttr)
	{
		if (rotAttr->useSeparate())
		{
			if (rotAttr->separateX())
				rotAttr->separateX()->value(frameNo, &isRotXCtrlPnt);

			if (rotAttr->separateY())
				rotAttr->separateY()->value(frameNo, &isRotYCtrlPnt);

			if (rotAttr->separateZ())
				rotAttr->separateZ()->value(frameNo, &isRotZCtrlPnt);
		}
		else
		{
			isRotCtrlPnt = isQuarternionKeyframe(*rotAttr, frameNo, tempAngle);
		}
	}


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

	AT_Scale3dAttr* scaleAttr = ::findAttribute<AT_Scale3dAttr>("SCALE"_L1, parent);

	if(scaleAttr)
		scaleAttr->getValue(frameNo, tempScaleX, tempScaleY, tempScaleZ, &isKeyframeX, &isKeyframeY, &isKeyframeZ,
			&isConstSegX, &isConstSegY, &isConstSegZ);


	//SKEW
	
	bool isSkewCtrlPnt = false;

	AT_DoubleAttr* skewAttr = ::findAttribute<AT_DoubleAttr>("SKEW"_L1, parent);
	
	if(skewAttr)
		skewAttr->value(frameNo, &isSkewCtrlPnt);


	return !(isPosCtrlPnt || isRotCtrlPnt || isRotXCtrlPnt || isRotYCtrlPnt || isRotZCtrlPnt 
		|| isKeyframeX || isKeyframeY || isKeyframeZ || isSkewCtrlPnt);
}