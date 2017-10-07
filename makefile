CC  = gcc
TMP = tmp
SRC = src
BIN = bin/unitscript
TEMPLATE = template

LIBS += -lyaml

INCLUDES += -I$(SRC)/header/
INCLUDES += -I$(TEMPLATE)

OPTIONS = -std=c99 -Og -g -Wall -Werror -Wextra -pedantic
OPTIONS += $(INCLUDES)

FILES_TMP = $(shell find $(SRC) -type f -name "*.c")
FILES = $(addprefix tmp/, $(FILES_TMP:.c=.o))

TEMPLATE_FILES_TMP = $(shell find $(TEMPLATE) -type f -name "*.template")
TEMPLATE_FILES = $(addprefix tmp/, $(TEMPLATE_FILES_TMP:.template=.o))

all: $(BIN)

$(BIN): $(FILES) $(TMP)/lib/usparser.a
	@mkdir -p "$(shell dirname "$@")"
	gcc $(LIBS) $^ -o $(BIN)

$(TMP)/%.o: %.c
	@mkdir -p "$(shell dirname "$@")"
	gcc $(OPTIONS) -c $< -o $@

$(TMP)/%.o: %.template
	@mkdir -p "$(shell dirname "$@")"
	gcc -x c -D US_GENERATE_CODE $(OPTIONS) -c $< -o $@

clean:
	rm -rf $(TMP) $(BIN)

$(TMP)/lib/usparser.a: $(TEMPLATE_FILES)
	rm -f $(TMP)/lib/usparser.a
	mkdir -p $(TMP)/lib/
	ar crs $(TMP)/lib/usparser.a $^
