#include "freeformModule.h"

#include <BaseCore/maths/MT_Point4d.h>

#include <GraphicCore/CinematicChain/CC_Transformation.h>
#include <SceneCore/attribute/AT_Position2dAttr.h>
#include <SceneCore/module/MO_PortTransform.h>

using namespace Qt::StringLiterals;

FreeformModule::FreeformModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: PointModuleBase(std::move(freezeManager), modulePtr, moduleType)
{
	AT_AttrList attrList = getAttributeList();

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
