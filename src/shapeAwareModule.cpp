#include "shapeAwareModule.h"
#include "elementModule.h"

using namespace Qt::StringLiterals;

ShapeAwareModule::ShapeAwareModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: PointModuleBase(std::move(freezeManager), modulePtr, moduleType)
{
	const AT_AttrList attrList = getAttributeList();

	//It was decided to not change the rotation attribute for now as it would only be effected
	//by skews and non-uniform scales. However, these would also lead to modified non-uniform scale
	//and skew attributes, which the freeform module is not able to represent. 
	//The resulting transformation did therefor not lead to a significantly closer match than keeping
	//the original value.

	for (auto& attr : attrList)
	{
		AT_ComplexAttr* cAttr;

		if (attr._pAttr->typeName() != "FREE-FORM-DEFORMATION-ATTRIBUTE"_L1)
			continue;
		cAttr = dynamic_cast<AT_ComplexAttr*>(attr._pAttr);

		if (!cAttr)
			continue;

		AT_Position2dAttr* posAttr = findSubAttribute<AT_Position2dAttr>(cAttr, "POSITION"_L1);

		if(posAttr)
			addTransformPoint(std::make_unique<TransformPoint>(cAttr->xmlKeyword()+ u".position"_s, posAttr, true));

		AT_Position2dAttr* restingPosAttr = findSubAttribute<AT_Position2dAttr>(cAttr, "RESTING_POSITION"_L1);

		if(restingPosAttr)
			addTransformPoint(std::make_unique<TransformPoint>(cAttr->xmlKeyword()+ u".restingPosition"_s, restingPosAttr, false));
	}
}

void ShapeAwareModule::readjustSecondary()
{
	processeInputModuls();
	PointModuleBase::readjustSecondary();
}

void ShapeAwareModule::processeInputModuls() const
{
	const std::vector<MO_Module*> inputModules = getAllInputModules(getModulePtr());

	const std::shared_ptr<FreezeManager> fm = getFreezeManager();
	const Math::Matrix4x4 freezeMatrix = fm->getFreezeMatrix();

	for (auto& inputModule : inputModules)
	{
		//This module's output is connected to other nodes as well
		if (!isShapeAwareMatteOnly(inputModule))
			continue;

		const int curId = getElementId(inputModule);

		if (curId == -1)
			continue;


		const QString layerAttr = getLayerAttr(inputModule);

		if (layerAttr == ""_L1)
			continue;

		//The drawing has an additional connection to the freeze peg
		//and is already part of the process queue
		if (fm->isProcessedElement(curId, layerAttr))
			continue;

		fm->addElement(curId, layerAttr, hasUsedDrawingPivots(inputModule));

		std::shared_ptr<ElementModule> elementModule = std::make_shared<ElementModule>(fm, inputModule, ModuleType::ELEMENT);

		const Math::Matrix4x4 alignmentMatrix = elementModule->getAlignmentMatrix();

		elementModule->setChangeMatrix(alignmentMatrix.getInverse() * freezeMatrix * alignmentMatrix);
		elementModule->modifyTimings();
	}
}