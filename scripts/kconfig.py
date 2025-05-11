import kconfiglib

kconf = kconfiglib.Kconfig("Kconfig")
kconf.load_config(".config")
kconf.write_autoconf("src/kernel/autoconf.h")