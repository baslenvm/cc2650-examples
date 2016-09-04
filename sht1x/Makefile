DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = cc2650-sht1x

all: $(CONTIKI_PROJECT)
CONTIKI_SOURCEFILES += sht11.c

CONTIKI_WITH_IPV6 = 1

BSL_FLAGS += -b 115200

CONTIKI = ../../../contiki
include $(CONTIKI)/Makefile.include
