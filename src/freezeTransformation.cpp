#include "freezeTransformation.h"
#include "utils.h"

#include <CelCore/Cel/CEL_Cel.h>
#include <CelCore/Cel/CEL_PixmapRGBA16.h>
#include <CelCore/CelIO/CELIO_Manager.h>
#include <CelCore/CelVector/CELVEC_Tbd.h>

#include <GraphicCore/GraphicLib/GR_ColorDict.h>
#include <GraphicCore/GraphicLib/GR_CompositeDrawingInfo.h>
#include <GraphicCore/GraphicLib/GR_CompositeVectorDrawingObj.h>
#include <GraphicCore/GraphicLib/GR_DrawingAccess.h>

#include <SceneCore/attribute/AT_Attr.h>
#include <SceneCore/celcache/CA_CelPtr.h>
#include <SceneCore/elementmanager/EM_Types.h>
#include <SceneCore/module/MO_BaseContext.h>
#include <SceneCore/module/MO_Module.h>

#include <SDKExtension/SDK_Commands.h>
#include <SDKExtension/SDK_Responder.h>

#include <chrono>
#include <future>
#include <iostream>
#include <memory>

ModuleWrappers createModuleWrappers(const std::vector<MO_Node*>& nodes, std::shared_ptr<FreezeManager> freezeManager)
{
	ModuleWrappers moduleWrappers;
	moduleWrappers.reserve(nodes.size());


	for (MO_Node* node : nodes)
	{
		if (!node->toModule())
		{
			continue;
		}

		if (node->keyword() == QLatin1String("READ"))
		{
			moduleWrappers.push_back(std::make_unique <DrawingTransformationModule>(freezeManager, node->toModule(),
				ModuleType::DRAWING_TRANSFORMATION, false));


			const int curId = getElementId(node->toModule());
			const QString layerAttr = getLayerAttr(node->toModule());

			if (freezeManager->isProcessedElement(curId, layerAttr))
			{
				freezeManager->updateDrawingPivotStatus(curId, layerAttr, hasUsedDrawingPivots(node->toModule()));
				continue;
			}

			freezeManager->addElement(curId, layerAttr, hasUsedDrawingPivots(node->toModule()));
			moduleWrappers.push_back(std::make_unique <ElementModule>(freezeManager, node->toModule(), ModuleType::ELEMENT));

		}

		if (node->keyword() == QLatin1String("PEG"))
		{
			moduleWrappers.push_back(std::make_unique <PegModule>(freezeManager, node->toModule(), ModuleType::PEG, false));
		}

		if (node->keyword() == QLatin1String("OffsetModule"))
		{
			moduleWrappers.push_back(std::make_unique <OffsetModule>(freezeManager, node->toModule(), ModuleType::CURVE_OFFSET));
		}

		if (node->keyword() == QLatin1String("CurveModule"))
		{
			moduleWrappers.push_back(std::make_unique <CurveModule>(freezeManager, node->toModule(), ModuleType::CURVE));
		}

		if (node->keyword() == QLatin1String("StaticConstraint"))
		{
			moduleWrappers.push_back(std::make_unique <StaticTransformationModule>(freezeManager, node->toModule(), ModuleType::STATIC_TRANSFORMATION));
		}

		if (node->keyword() == QLatin1String("OglController"))
		{
			moduleWrappers.push_back(std::make_unique <OglControllerModule>(freezeManager, node->toModule(), ModuleType::OGL_CONTROLLER));
		}

		if (!moduleWrappers.empty())
		{
			freezeManager->updateFrameRange(moduleWrappers.back()->getFrameRange());
		}
	}

	return moduleWrappers;
}

void FreezeResponder::onActionFreezeTransformation()
{
	printf("********Freeze Transformation Start********\n");
	
	//Temporarily deactivated undoScope to be able to isolate bitmap tests
	/*
	const QString s = QStringLiteral("Freeze Transformation");

	SDK_Drawing::UndoScope undoScope(s);
	*/

	SDK_Selection::NodeCol_t nodes;
	SDK_Selection::getSelectedNodes(nodes);


	if (nodes.size() != 1)
	{
		printf("Please select exactly one peg node\n");
		return;
	}

	if (nodes[0]->keyword() != QLatin1String("PEG"))
	{
		printf("The selected node is not a peg");
		return;
	}

	std::shared_ptr<FreezeManager> freezeManager = std::make_shared<FreezeManager>();

	std::unique_ptr<PegModule> freezeModule;

	try
	{
		freezeModule = std::make_unique<PegModule>(freezeManager, nodes[0]->toModule(), ModuleType::PEG, true);
	}
	catch (std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return;
	}

	const std::vector<MO_Node*> children = getAllChildren(freezeModule->getModulePtr());

	ModuleWrappers moduleWrappers;

	try
	{
		moduleWrappers = createModuleWrappers(children, freezeManager);
	}
	catch (std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return;
	}

	const Math::Matrix4x4 freezeMatrix = freezeModule->getLocalMatrix(freezeManager->getSelFrame());

	//Need to check the original matrix, before being converted to 2d inside freezeManager
	if (isScaleZero(freezeMatrix) || freezeMatrix.isIdentity())
	{
		//Either won't be able to invert freeze matrix, everything underneath is invisible,
		//or the freeze matrix is the identity matrix. In either case, there is nothing to process
		return;
	}

	//It is important to use the freeze Modules actual matrix into account here, not the one without pivot values
	freezeManager->setMatrices(freezeMatrix, freezeModule->getModulePtr()->sceneMetrics());

	if (!freezeManager->isExperimentalMode())
		freezeManager->initializeFileJS();

	freezeModule->freeze();

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	for (auto& child : moduleWrappers)
	{
		printf("Now processing %s\n", qPrintable(child->getModulePtr()->instanceName()));
		child->readjustSecondary();
	}

	std::future<void> future;

	if(freezeManager->isExperimentalMode())
	{
		//This is the only case where multithreading seems to be worth it.
		//For readjust secondary, it tends to take longer in most cases and while
		//it would be rather efficient to modify all the elements in parallel to each other
		//there seems to be something under the hood (potentially GR_DrawingAccess, or SDK_Drawing::ChangeScope)
		//that isn't completely threadsafe

		if (freezeManager->isMultithreadingMode())
			future = std::future<void>(std::async(std::launch::async,
			&FreezeManager::executeCommands,
			freezeManager));
		else
			freezeManager->executeCommands();
	}
	else
	{
		//Multithreading JS leads to crashes
		freezeManager->finalizeFileJS();
	}


	for (auto& child : moduleWrappers)
	{
		if (child->getModuleType() == ModuleType::ELEMENT)
		{
			printf("Now processing drawings for %s\n", qPrintable(child->getModulePtr()->instanceName()));
				child->readjustTertiary();
		}
	}

	if (future.valid())
	{
		try
		{
			future.get();
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
		}
		catch (...)
		{
			std::cerr << "Unknown exception" << std::endl;
		}
	}

	const auto end = std::chrono::steady_clock::now();

	if (freezeManager->isDebugMode())
	{
		std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[s]" << std::endl;
		std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
		std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
	}

	printf("********Freeze Transformation Completed********\n");
}

constexpr char copyrightHeader[] = "===== HarmonyFreeze - "
"v0.5.0 \n"
"Copyright 2026 Laura Kellner <laura@roquetto.com>. All rights reserved. =====\n"
" SPDX - License - Identifier: GPL-3.0-only\n"
"\n"
"This software uses the Qt Toolkit\n"
"This software uses Qt licensed under the GNU General Public License (GPL).\n"
"Qt source code and licensing information: https://www.qt.io\n";

extern "C"
{
	// called when plugin is loaded.
	MISC_DYLIB_EXPORT void PLUG_Init()
	{
		printf("%s\n", copyrightHeader);
	}

	// this function is called when about to present the UI.
	MISC_DYLIB_EXPORT void PLUG_Polish()
	{
		static bool shortcutAdded = false;

		if (!shortcutAdded)
		{
			shortcutAdded = true;

			const QString shortcutsFilename = PLUG_Services::getPluginPath(QLatin1String("HarmonyFreeze"))
				+ QLatin1String(NTR("/resources/shortcuts.xml"));

			SDK_Responder::addShortcutsFromFile(shortcutsFilename);
		}

		// load the menu XML file and attach this new menu to the 'Animation" menu.
		const QString menusFilename = PLUG_Services::getPluginPath(QLatin1String("HarmonyFreeze"))
			+ QLatin1String("/resources/menu.xml");

		PLUG_Services::getMenuService()->addMenuFromFile(QLatin1String(NTR("Animation")),  menusFilename);

		static FreezeResponder responder;
		responder.setObjectName(QLatin1String("FreezeResponder") );

		SDK_Responder::registerResponder(&responder);
	}
}

