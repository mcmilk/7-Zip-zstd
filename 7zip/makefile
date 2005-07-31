DIRS = \
  Compress\~ \
  Crypto\~ \
  Archive\~ \
  UI\~ \
  Bundles\~ \
  
all: $(DIRS) FileManager\~

$(DIRS):
	cd $(@D)
	$(MAKE) -nologo
	cd ..

FileManager\~:
!include "SubBuild.mak"
