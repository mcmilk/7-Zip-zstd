DIRS = \
  UI\~ \
  Bundles\~ \
  
all: $(DIRS) FileManager\~

$(DIRS):
	cd $(@D)
	$(MAKE) -nologo
	cd ..

FileManager\~:
!include "SubBuild.mak"
