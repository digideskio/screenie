DEPENDPATH += $$PWD/../Utils/src \
              $$PWD/GeneratedFiles \
              $$PWD/src/Dao \
              $$PWD/src/Dao/Xml \
              $$PWD/src

HEADERS += $$PWD/src/ModelLib.h \
           $$PWD/src/AbstractScreenieModel.h \
           $$PWD/src/DefaultScreenieModel.h \
           $$PWD/src/ScreenieFilePathModel.h \
           $$PWD/src/ScreenieModelInterface.h \
           $$PWD/src/ScreeniePixmapModel.h \
           $$PWD/src/ScreenieTemplateModel.h \
           $$PWD/src/ScreenieScene.h \
           $$PWD/src/Dao/ScreenieSceneDao.h \
           $$PWD/src/Dao/ScreenieFilePathModelDao.h \
           $$PWD/src/Dao/ScreeniePixmapModelDao.h \
           $$PWD/src/Dao/ScreenieTemplateModelDao.h \
           $$PWD/src/Dao/Xml/XmlScreenieSceneDao.h \           
           $$PWD/src/Dao/Xml/XmlScreenieFilePathModelDao.h \
           $$PWD/src/Dao/Xml/XmlScreenieTemplateModelDao.h \
           $$PWD/src/Dao/Xml/AbstractXmlScreenieModelDao.h \
           $$PWD/src/Dao/Xml/XmlScreeniePixmapModelDao.h


SOURCES += $$PWD/src/AbstractScreenieModel.cpp \
           $$PWD/src/DefaultScreenieModel.cpp \
           $$PWD/src/ScreenieFilePathModel.cpp \
           $$PWD/src/ScreenieModelInterface.cpp \
           $$PWD/src/ScreeniePixmapModel.cpp \
           $$PWD/src/ScreenieTemplateModel.cpp \
           $$PWD/src/ScreenieScene.cpp \
           $$PWD/src/Dao/Xml/XmlScreenieSceneDao.cpp \
           $$PWD/src/Dao/Xml/XmlScreenieFilePathModelDao.cpp \
           $$PWD/src/Dao/Xml/XmlScreenieTemplateModelDao.cpp \
           $$PWD/src/Dao/Xml/AbstractXmlScreenieModelDao.cpp \
           $$PWD/src/Dao/Xml/XmlScreeniePixmapModelDao.cpp
