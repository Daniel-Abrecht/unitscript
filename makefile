CC  = gcc
BUILD = build
BIN = bin/unitscript

LIBS += -ldpaparser -lyaml -lbsd

DPAPARSER = dpaparser
TEMPLATE = template
TEMPLATE_A = $(BUILD)/libtemplates.a

INCLUDES += -Isrc/header/
INCLUDES += -I$(TEMPLATE)

OPTIONS = -std=c99 -Wall -Werror -Wextra -pedantic
OPTIONS += $(INCLUDES)

SOURCES = $(wildcard src/*.c)
OBJECTS = $(addsuffix .o,$(addprefix $(BUILD)/, $(SOURCES)))


all: $(BIN)

$(BIN): $(OBJECTS) $(TEMPLATE_A)
	@mkdir -p "$(shell dirname "$@")"
	$(CC) $^ $(LIBS) -o $(BIN)

$(BUILD)/%.c.o: %.c
	@mkdir -p "$(dir $@)"
	$(CC) $(OPTIONS) -c $< -o "$@"

$(TEMPLATE_A):
	$(DPAPARSER) CC="$(CC)" BUILD="$(BUILD)" TEMPLATE="$(TEMPLATE)" TEMPLATE_A="$(TEMPLATE_A)"

clean:
	rm -rf $(BUILD) $(BIN)

