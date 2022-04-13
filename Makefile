# Compilador
CC = gcc

# Flags de compilação
CFLAGS   = -std=gnu99 -Wall -Wextra -O2 -Wundef -Wshadow -Wstrict-prototypes -Wwrite-strings -Wunreachable-code -Werror

# Flags de linking
LDFLAGS_C   =  -lm
LDFLAGS_S = -lm $(shell pkg-config --libs libcpuid) $(shell pkg-config --libs glib-2.0)

# Variáveis
SRC_DIR = src
INC_DIR = include
BIN_DIR = obj

SRC = $(shell find src -type f \( -iname "*.c" ! -iname "server.c" \))
OBJ = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRC))
DEP = $(OBJ:%.o=%.d)

SRC_S = $(shell find src -type f \( -iname "*.c" ! -iname "client.c" \))
OBJ_S = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRC_S))
DEP_S = $(OBJ_S:%.o=%.d)

NAME   = client
NAME_C = client

NAME_S   = server
NAME_S_S = server

# Cliente
$(NAME): cleant
$(NAME): $(BIN_DIR)/$(NAME)

$(BIN_DIR)/$(NAME): $(OBJ)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ $(LDFLAGS_C) -o $@
	ln -sf $@ $(NAME_C)
	mkdir -p entrada saida

-include $(DEP)
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INC_DIR) -MMD -c $< -o $@

# Servidor
$(NAME_S): clean
$(NAME_S): $(BIN_DIR)/$(NAME_S)

$(BIN_DIR)/$(NAME_S): $(OBJ_S)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ $(LDFLAGS_S) -o $@
	ln -sf $@ $(NAME_S_S)

-include $(DEP)
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INC_DIR) -MMD -c $< -o $@

.PHONY: clean
clean:
	-rm -rf obj/* $(NAME_C)
	-rm client
	-rm saida/*

.PHONY: cleant
cleant:
	-rm -rf obj/* $(NAME_S_S)
	-rm saida/*.txt