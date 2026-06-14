TEMPLATE = lib
TARGET =  HarmonyFreeze
DEPENDPATH += .
INCLUDEPATH += .
			..

CONFIG += c++2a

QT += core gui xml widgets

greaterThan(QT_MAJOR_VERSION, 5) {
	QT += core5compat
}

SDKROOT=$$(HARMONY_SDK_ROOT)

isEmpty(SDKROOT) {
	error("HARMONY_SDK_ROOT not set")
}

INCLUDEPATH += $$SDKROOT/src/SDK \
	$$SDKROOT/src/Plugins \

win32 {
	DEFINES += TU_WINDOWS

	QMAKE_LIBDIR += $$SDKROOT/lib\
	$$SDKROOT/src/Plugins/SDKExtension/win64\
}

macx {
	TB_ROOT=$$(TB_ROOT)

	isEmpty(TB_ROOT) {
		error("TB_ROOT not set")
	}

	SDK_LIB = $$TB_ROOT/Contents/tba/macosx/lib
	SDK_EXT_LIB = $$TB_ROOT/Contents/tba/Plugins/SDKExtension/macosx

	QMAKE_LFLAGS += '-L$$SDK_LIB'
	QMAKE_LFLAGS += '-L$$SDK_EXT_LIB'
	QMAKE_LFLAGS_SHLIB += '-e PLUG_Init'

	LIBS += \
		-lToonBoomBaseCore \
		-lToonBoomGraphicCore \
		-lToonBoomSceneCore \
		-lToonBoomCelCore \
		-lSDKExtension \
		-lToonBoomPlugInManager \
		-lToonBoomCommand \
}

DEFINES += QT_NO_CAST_FROM_ASCII \
           QT_NO_CAST_TO_ASCII

HEADERS += \
	src/moduleBase.h\
	src/elementModule.h\
	src/freezeTransformation.h\
	src/transformationModule.h\
	src/staticTransformationModule.h\
	src/utils.h\
	src/offsetModule.h\
	src/curveModule.h\
	src/curveModuleBase.h\
	src/freezeManager.h\
	src/oglControllerModule.h\
	src/drawingTransformationModule.h\
	src/pegModule.h\


# Input
SOURCES += src/freezeTransformation.cpp\
	src/transformationModule.cpp\
	src/staticTransformationModule.cpp\
	src/utils.cpp\
	src/moduleBase.cpp\
	src/elementModule.cpp\
	src/curveModule.cpp\
	src/offsetModule.cpp\
	src/curveModuleBase.cpp\
	src/freezeManager.cpp\
	src/oglControllerModule.cpp\
	src/drawingTransformationModule.cpp\
	src/pegModule.cpp\

win32 {
	LIBS += \
		ToonBoomBaseCore.lib \
		ToonBoomGraphicCore.lib \
		ToonBoomSceneCore.lib \
		ToonBoomCelCore.lib \
		SDKExtension.lib \
		ToonBoomPlugInManager.lib \
		ToonBoomCommand.lib \
}

macx {
	DYLIB = $$OUT_PWD/lib$${TARGET}.dylib

	QMAKE_POST_LINK += \
		/bin/sh "$$PWD/fix_install_names.sh" "$$DYLIB";
}
