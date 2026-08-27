#include "pkoModule.h"

#include <SceneCore/module/MO_PortTransform.h>
#include <GraphicCore/CinematicChain/CC_Transformation.h>

#include <limits>
#include <stdexcept>

PkoModule::PkoModule(std::shared_ptr<FreezeManager> freezeManager,
		MO_Module* modulePtr,
		ModuleType moduleType)
	: ModuleBase(std::move(freezeManager), modulePtr, moduleType)
	//, m_offsetAttr(nullptr)
	//, m_restingOffsetAttr(nullptr)
	//, m_isIndependent(true)
{
	//initAttributes();
	AT_AttrList list;
	modulePtr->getFullAttrList(list);
	printAttributes(list);
}

void PkoModule::readjustSecondary()
{

}