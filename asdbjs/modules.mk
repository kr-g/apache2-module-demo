mod_asdbjs.la: mod_asdbjs.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_asdbjs.lo
DISTCLEAN_TARGETS = modules.mk
shared =  mod_asdbjs.la
