
define allow-override
  $(if $(or $(findstring environment,$(origin $(1))),\
            $(findstring command line,$(origin $(1)))),,\
    $(eval $(1) = $(2)))
endef

TOOL_PREFIX=
CC=$(TOOL_PREFIX)gcc
LD=$(TOOL_PREFIX)ld

ifdef TOOL_PREFIX
$(call allow-override,CC,$(CC))
$(call allow-override,LD,$(LD))
endif

CDEBUG=-g
CFLAGS=-g

BINARY=main

.PHONY: all clean tests


%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: clean $(BINARY)

$(BINARY): main.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	find . -type d \( -path include \) -prune -false -o -iname '*.o' -o -iname '*.so' | xargs rm -f
	rm -f $(BINARY) 2>/dev/null
