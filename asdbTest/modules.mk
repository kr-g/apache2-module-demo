mod_asdbTest.la: mod_asdbTest.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_asdbTest.lo
DISTCLEAN_TARGETS = modules.mk
shared =  mod_asdbTest.la
