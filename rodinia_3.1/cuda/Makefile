SUBDIRS := $(wildcard */)

all: $(SUBDIRS)
$(SUBDIRS): 
	echo $@: `date +%s` >> time.log
	-$(MAKE) -C $@ drano 
	echo $@: `date +%s` >> time.log
	echo >> time.log


.PHONY: all $(SUBDIRS)
