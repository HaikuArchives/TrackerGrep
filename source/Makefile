all:
ifeq ($(wildcard /boot/beos/system/lib/libzeta.so), )
	$(MAKE) --makefile=Makefile_beos
else
	$(MAKE) --makefile=Makefile_zeta
endif


clean:
ifeq ($(wildcard /boot/beos/system/lib/libzeta.so), )
	$(MAKE) --makefile=Makefile_beos clean
else
	$(MAKE) --makefile=Makefile_zeta clean
endif
	rm -rf ../release


release:
	@mkdir -p ../release
ifeq ($(wildcard /boot/beos/system/lib/libzeta.so), )
	$(MAKE) --makefile=Makefile_beos release
else
	$(MAKE) --makefile=Makefile_zeta release
endif


install:
ifeq ($(wildcard /boot/beos/system/lib/libzeta.so), )
	$(MAKE) --makefile=Makefile_beos install
else
	$(MAKE) --makefile=Makefile_zeta install
endif
