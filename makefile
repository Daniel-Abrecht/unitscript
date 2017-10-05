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


all: $(BIN)

$(BIN): $(FILES) $(TMP)/lib/usparser.a
	@mkdir -p "$(shell dirname "$@")"
	gcc $(LIBS) $^ -o $(BIN)

$(TMP)/%.o: %.c
	@mkdir -p "$(shell dirname "$@")"
	gcc $(OPTIONS) -c $< -o $@

clean:
	rm -rf $(TMP) $(BIN)

$(TMP)/lib/usparser.a:
	rm -f $(TMP)/lib/usparser.a
	mkdir -p $(TMP)/lib/
	for template in $(shell find $(TEMPLATE) -type f -name "*.template"); do \
	  gcc -x c -D US_GENERATE_CODE $(OPTIONS) $$template -c -o $(TMP)/lib/usparser.o; \
	  ar cq $(TMP)/lib/usparser.a $(TMP)/lib/usparser.o; \
	  rm -f $(TMP)/lib/usparser.o; \
	done

