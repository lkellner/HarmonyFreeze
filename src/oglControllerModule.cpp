#include "oglControllerModule.h"
#include "elementModule.h"


#include <BaseCore/maths/MT_Matrix4x4.h>
#include <SceneCore/attribute/AT_AttrCmds.h>
#include <SceneCore/attribute/AT_PositionAttrCmds.h>
#include <SceneCore/attribute/AT_Rotation3dAttrCmds.h>
#include <SceneCore/attribute/AT_Scale3dAttrCmds.h>
#include <SceneCore/attribute/AT_ScaleAttrCmds.h>
#include <SceneCore/selectable/SLB_CmdManipulator.h>
#include <SceneCore/selectable/SLB_ManipContext.h>
#include <Util/command/CO_OrCommand.h>
#include <Util/pluginmanager/PLUG_Scripting.h>
#include <Util/pluginmanager/PLUG_Services.h>


OglControllerModule::OglControllerModule(std::shared_ptr<FreezeManager> freezeManager, MO_Module* modulePtr, ModuleType moduleType)
	: ModuleBase(freezeManager, modulePtr, moduleType)
	, m_positionAttr(findAttribute<AT_Position3dAttr>(QStringLiteral("position")))
	, m_textPositionAttr(findAttribute<AT_Position3dAttr>(QStringLiteral("textposition")))
	, m_sizeAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("size")))
	, m_textSizeAttr(findAttribute<AT_DoubleAttr>(QStringLiteral("textsize")))
	, m_isPassOnToDrawings(false)
{
	if (!m_positionAttr)
		throw std::runtime_error("missing attribute: 'position' for " + modulePtr->qualifiedName().toStdString());
	if (!m_textPositionAttr)
		throw std::runtime_error("missing attribute: 'textposition' for " + modulePtr->qualifiedName().toStdString());
	if (!m_sizeAttr)
		throw std::runtime_error("missing attribute: 'size' for " + modulePtr->qualifiedName().toStdString());
	if (!m_textSizeAttr)
		throw std::runtime_error("missing attribute: 'textsize' for " + modulePtr->qualifiedName().toStdString());
}

Math::Matrix4x4 OglControllerModule::getIncomingMatrix(unsigned int port, double frameNo, bool) const
{
	//This might be a bug within Harmony, as it would make sense to expect the ogl controllers to behave similar to pegs
	//when being put underneath a curve deformer,overriding getIncomingMatrix to the parentModelMatrix returns the correct values for now,
	//as all the transformations in the parent's matrix are reflected in the display of the controller.
	//It might be worth keeping an eye on this one in case it could change in the future.

	const MO_BaseContext context{ MO_FrameKey(frameNo) };

	return getModulePtr()->getParentModelMatrix(context, port);
}


Math::Matrix4x4 OglControllerModule::getLocalMatrix(const double frameNo) const
{
	Math::Point3d position;

	if (m_positionAttr)
		m_positionAttr->getValue(frameNo, position);

	double size = 1.0;

	if (m_sizeAttr)
		size = m_sizeAttr->value(frameNo);

	return constructLocalMatrix(position, size);
}

bool OglControllerModule::isDoubleFlipped(const Math::Matrix4x4& matrix) const
{
	//TODO: there might be an easier way
	const MatrixParameters p = matrixParameters(matrix, Math::Point3d(0, 0, 0), Math::Angle3d(0, 0, 0));

	double anglez = std::fmod(p.az, 360);

	if (anglez < 0)
		anglez += 360;

	if (anglez > 90 && anglez < 270)
		return true;

	if (p.skewx > 90 || p.skewx < -90)
		return true;

	return p.sy < 0;
}


double OglControllerModule::getScaleFactor(const Math::Matrix4x4& matrix, const double frameNo) const
{
	//the transformation input should always be zero, the visual representation flips, when the port order is changed
	Math::Matrix4x4 parentMatrix;
	if (getModulePtr()->inPort(0)->isMatrixPort())
		parentMatrix = getIncomingMatrix(0, frameNo, false);

	const bool isFlippedBefore = isDoubleFlipped(convertToProjectionMatrix(parentMatrix));
	const bool isFlippedAfter = isDoubleFlipped(convertToProjectionMatrix(parentMatrix * matrix.getInverse()));

	//The displayed size always uses the highest value between x and y scale of the incoming parent
	const Math::Point3d scaleBefore = getScale(parentMatrix);
	const double chosenScaleBefore = abs(scaleBefore.x()) > abs(scaleBefore.y()) ? abs(scaleBefore.x()) : abs(scaleBefore.y());

	const Math::Point3d scale = getScale(parentMatrix * getFreezeManagerPtr()->getFreezeMatrix().getInverse());

	const double chosenScale = abs(scale.x()) > abs(scale.y()) ? abs(scale.x()) : abs(scale.y());

	double scaleFactor = chosenScaleBefore / chosenScale;

	if (isFlippedBefore != isFlippedAfter)
		scaleFactor *= -1;

	return scaleFactor;
}

Math::Matrix4x4 OglControllerModule::constructLocalMatrix(Math::Point3d position, const double size) const
{
	const Math::Point3d oglPosition = getModulePtr()->sceneMetrics()->toOGL(position);

	Math::Matrix4x4 matrix = Math::Matrix4x4();

	matrix.translate(oglPosition.x(), oglPosition.y(), oglPosition.z());
	matrix.scale(size, size, size);

	return matrix;
}


void OglControllerModule::initInputModules()
{
	m_inputModules = getAllInputModules(getModulePtr());

	if (m_inputModules.size() > 0)
		m_isPassOnToDrawings = true;

	for (auto& inputModule : m_inputModules)
	{
		int curId = getElementId(inputModule);

		if (curId == -1)
		{
			m_isPassOnToDrawings = false;
			return;
		}


		QString layerAttr = getLayerAttr(inputModule);

		if (layerAttr == QLatin1String(""))
		{
			m_isPassOnToDrawings = false;
			return;
		}

		if (getFreezeManagerPtr()->isProcessedElement(curId, layerAttr))
		{
			//Can only pass on to drawings if none of them have already been frozen
			m_isPassOnToDrawings = false;
			return;
		}
	}
}


void OglControllerModule::readjustSecondary()
{
	FreezeManager* fm = getFreezeManagerPtr();

	//If the general setting is to not pass on any values to the drawings
	//this step can be skipped entirely
	if (fm->isPassOnOgl())
		initInputModules();


	Math::Matrix4x4 oglocalMatrix = getLocalMatrix(fm->getSelFrame()); //TO be used later for elements

	Math::Matrix4x4 oglChangeMatrix = fm->getFreezeMatrix();

	double scaleFactor = getScaleFactor(oglChangeMatrix, fm->getSelFrame());

	Math::Matrix4x4 fieldsChangeMatrix = getFieldsModificationMatrix(getModulePtr()->sceneMetrics(), oglChangeMatrix);


	//Static attributes

	double size = m_sizeAttr->localValue();


	Math::Point3d position;
	m_positionAttr->getLocalValue(position);

	Math::Point3d textPosition;
	m_textPositionAttr->getLocalValue(textPosition);


	if (m_isPassOnToDrawings)
	{
		printf("is pass on to drawings!\n");
		//Applying the change in position to the text
		position = fieldsChangeMatrix.rotation() * position;
		textPosition = fieldsChangeMatrix * textPosition;
	}
	else
	{
		//Change in position is already applied to the position attribute
		position = fieldsChangeMatrix * position;
		textPosition = fieldsChangeMatrix.rotation() * textPosition;
		size *= std::abs(scaleFactor); //Size only gets adjusted if there are no input modules to pass the values on to
	}

	double textSize = m_textSizeAttr->localValue() * scaleFactor;

	std::shared_ptr<CO_OrCommand> curMacro = std::make_shared<CO_OrCommand>();

	setStaticAttributes(position, textPosition, size, textSize, *curMacro);



	//Keyframed attributes

	FrameRange range = getFrameRange();

	for (int curFrame = range.start; curFrame <= range.end; curFrame++)
	{
		m_positionAttr->getValue(curFrame, position);

		m_textPositionAttr->getValue(curFrame, textPosition);

		scaleFactor = getScaleFactor(oglChangeMatrix, curFrame);

		if (m_isPassOnToDrawings)
		{
			//Applying the change in position to the text
			position = fieldsChangeMatrix.rotation() * position;
			textPosition = fieldsChangeMatrix * textPosition;
		}
		else
		{
			//Change in position is already applied to the position attribute
			position = fieldsChangeMatrix * position;
			textPosition = fieldsChangeMatrix.rotation() * textPosition;
			size = m_sizeAttr->value(curFrame) * scaleFactor;
		}

		textSize = m_textSizeAttr->value(curFrame) * scaleFactor;

		setAttributes(position, textPosition, size, textSize, *curMacro, curFrame);
	}

	getFreezeManagerPtr()->addCommand(std::move(curMacro));

	if (m_isPassOnToDrawings)
	{
		FreezeManager* fm = getFreezeManagerPtr();

		Math::Matrix4x4 updatedLocalMatrix = Math::Matrix4x4().translate(fm->getFreezeMatrix().rotation() * oglocalMatrix.origin().toVector());
		updatedLocalMatrix *= oglocalMatrix.rotation();
		Math::Matrix4x4 inputChangeMatrix = updatedLocalMatrix.getInverse() * fm->getFreezeMatrix() * oglocalMatrix;

		modifyInputModules(inputChangeMatrix);
	}
}


void OglControllerModule::modifyInputModules(Math::Matrix4x4 inputChangeMatrix)
{
	std::shared_ptr<FreezeManager> fm = getFreezeManager();

	for (auto& inputModule : m_inputModules)
	{
		int curId = getElementId(inputModule);
		QString layerAttr = getLayerAttr(inputModule);

		Math::Matrix4x4 inputMatrix = inputModule->getModelMatrix(MO_FrameKey(fm->getSelFrame()));
		Math::Matrix4x4 flipMatrix = getElementFlipMatrix(inputModule);

		inputMatrix = inputMatrix * flipMatrix * fm->getSceneSettingsMatrix();

		inputChangeMatrix = inputMatrix.getInverse() * inputChangeMatrix * inputMatrix;

		fm->addElement(curId, layerAttr, hasUsedDrawingPivots(inputModule));

		std::shared_ptr<ElementModule> elementModule = std::make_shared<ElementModule>(fm, inputModule, ModuleType::ELEMENT);

		elementModule->setChangeMatrix(inputChangeMatrix);
		elementModule->modifyTimings();
	}
}


void OglControllerModule::setStaticAttributes(Math::Point3d position, Math::Point3d textPosition, double size, double textSize, CO_OrCommand& curMacro)
{
	clampControllerValues(position, textPosition, size, textSize);

	//Similar to transformation module, can't set local value of combined paths

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++
		curMacro.add(Attr::Position3d::createSetLocalValueCmd(m_positionAttr, position.x(), position.y(), position.z()));
		curMacro.add(Attr::Position3d::createSetLocalValueCmd(m_textPositionAttr, textPosition.x(), textPosition.y(), textPosition.z()));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_sizeAttr, size));
		curMacro.add(Attr::Double::createSetLocalValueCmd(m_textSizeAttr, textSize));
	}
	else
	{
		//JS

		fm->applyAttributes(getModulePtr()->qualifiedName(),
			StaticAttrData{ QLatin1String("position.x"), position.x() },
			StaticAttrData{ QLatin1String("position.y"), position.y() },
			StaticAttrData{ QLatin1String("position.z"), position.z() },
			StaticAttrData{ QLatin1String("size"), size },
			StaticAttrData{ QLatin1String("textposition.x"), textPosition.x() },
			StaticAttrData{ QLatin1String("textposition.y"), textPosition.y() },
			StaticAttrData{ QLatin1String("textposition.z"), textPosition.z() },
			StaticAttrData{ QLatin1String("textsize"), textSize });
	}

}


void OglControllerModule::clampControllerValues(Math::Point3d& position, Math::Point3d& textPosition, double& size, double& textSize) const
{
	clampValues(position);
	clampValues(textPosition);
	clampValue(size, 1);
	clampValue(size, -1);
	clampValue(textSize, 1);
	clampValue(textSize, -1);
}


void OglControllerModule::setAttributes( Math::Point3d position, Math::Point3d textPosition, double size, double textSize, CO_OrCommand& curMacro, const double frameNo)
{
	Math::Point3d tempPoint;

	bool isPosCtrlPnt = false;
	bool isSizeCtrlPnt = false;
	bool isTextPosCtrlPnt = false;
	bool isTextSizeCtrlPnt = false;
	bool isConstSeg = false;

	m_positionAttr->getValue(frameNo, tempPoint, &isPosCtrlPnt, &isConstSeg); //FIXME interface change
	m_sizeAttr->value(frameNo, &isSizeCtrlPnt, &isConstSeg);
	m_textPositionAttr->getValue(frameNo, tempPoint, &isTextPosCtrlPnt, &isConstSeg); //FIXME interface change
	m_textSizeAttr->value(frameNo, &isTextSizeCtrlPnt, &isConstSeg);

	clampControllerValues(position, textPosition, size, textSize);

	FreezeManager* fm = getFreezeManagerPtr();

	if (fm->isExperimentalMode())
	{
		//C++

		if (isPosCtrlPnt)
			curMacro.add(Attr::Position3d::createSetValueCmd(m_positionAttr, frameNo, position.x(), position.y(), position.z()));

		if (isSizeCtrlPnt)
			curMacro.add(Attr::Double::createSetValueCmd(m_sizeAttr, frameNo, size));

		if (isTextPosCtrlPnt)
			curMacro.add(Attr::Position3d::createSetValueCmd(m_textPositionAttr, frameNo, textPosition.x(), textPosition.y(), textPosition.z()));

		if (isTextSizeCtrlPnt)
			curMacro.add(Attr::Double::createSetValueCmd(m_textSizeAttr, frameNo, textSize));
	}
	else
	{
		//JS

		fm->applyAttributes(getModulePtr()->qualifiedName(),
			TextAttrData{ QLatin1String("position.3DPATH.X"), position.x(), frameNo, isPosCtrlPnt },
			TextAttrData{ QLatin1String("position.3DPATH.Y"), position.y(), frameNo, isPosCtrlPnt },
			TextAttrData{ QLatin1String("position.3DPATH.Z"), position.z(), frameNo, isPosCtrlPnt },
			AttrData{ QLatin1String("position.x"), position.x(), frameNo, isPosCtrlPnt },
			AttrData{ QLatin1String("position.y"), position.y(), frameNo, isPosCtrlPnt },
			AttrData{ QLatin1String("position.z"), position.z(), frameNo, isPosCtrlPnt },
			AttrData{ QLatin1String("size"), size, frameNo, isSizeCtrlPnt },
			TextAttrData{ QLatin1String("textposition.3DPATH.X"), textPosition.x(), frameNo, isTextPosCtrlPnt },
			TextAttrData{ QLatin1String("textposition.3DPATH.Y"), textPosition.y(), frameNo, isTextPosCtrlPnt },
			TextAttrData{ QLatin1String("textposition.3DPATH.Z"), textPosition.z(), frameNo, isTextPosCtrlPnt },
			AttrData{ QLatin1String("textposition.x"), textPosition.x(), frameNo, isTextPosCtrlPnt },
			AttrData{ QLatin1String("textposition.y"), textPosition.y(), frameNo, isTextPosCtrlPnt },
			AttrData{ QLatin1String("textposition.z"), textPosition.z(), frameNo, isTextPosCtrlPnt },
			AttrData{ QLatin1String("textsize"), textSize, frameNo, isTextSizeCtrlPnt });
	}
}


FrameRange OglControllerModule::getFrameRange() const
{
	FrameRange range;

	int key;
	if (m_positionAttr)
	{
		if (m_positionAttr->separateX()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateX()->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateY()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateY()->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateZ()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->separateZ()->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->path3D()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_positionAttr->path3D()->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);
	}

	if (m_textPositionAttr)
	{
		if (m_textPositionAttr->separateX()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_textPositionAttr->separateX()->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);

		if (m_textPositionAttr->separateY()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_textPositionAttr->separateY()->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);

		if (m_textPositionAttr->separateZ()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_textPositionAttr->separateZ()->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);

		if (m_textPositionAttr->path3D()->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_textPositionAttr->path3D()->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);
	}


	//Size
	if (m_sizeAttr)
	{
		if (m_sizeAttr->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_sizeAttr->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);
	}

	if (m_textSizeAttr)
	{
		if (m_textSizeAttr->getNextKey(0, &key))
			updateFrameRange(range, key);

		if (m_textSizeAttr->getPrevKey(INT_MAX, &key))
			updateFrameRange(range, key);
	}

	return range;
}
