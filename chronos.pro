# Root build project.
TEMPLATE = subdirs
SUBDIRS = camApp \
          camUpdate

camApp.file = src/camApp.pro
camUpdate.file = update/camUpdate.pro
