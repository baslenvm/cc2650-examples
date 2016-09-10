DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = cc2650-seg7

all: $(CONTIKI_PROJECT)
CONTIKI_SOURCEFILES += seg7.c

CONTIKI_WITH_IPV6 = 1
BSL_FLAGS += -b 115200

CONTIKI = ../../../contiki
include $(CONTIKI)/Makefile.include
