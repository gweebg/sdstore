# Compilador
CC = gcc

# Flags de compilação
CFLAGS   = -std=gnu99 -Wall -Wextra -O2 -Wunreachable-code -g

# Flags de linking
LDFLAGS_C = -lm
LDFLAGS_S = -lm 

# Variáveis
SRC_DIR = src
INC_DIR = includes
BIN_DIR = obj

SRC = $(shell find src -type f \( -iname "*.c" ! -iname "server.c" \))
OBJ = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRC))
DEP = $(OBJ:%.o=%.d)

SRC_S = $(shell find src -type f \( -iname "*.c" ! -iname "client.c" \))
OBJ_S = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRC_S))
DEP_S = $(OBJ_S:%.o=%.d)

NAME   = sdstore
NAME_C = sdstore

NAME_S   = sdstored
NAME_S_S = sdstored

# Tudo
all: $(NAME) $(NAME_S)

# Cliente
$(NAME): $(BIN_DIR)/$(NAME)

$(BIN_DIR)/$(NAME): $(OBJ)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ $(LDFLAGS_C) -o $@
	ln -sf $@ $(NAME_C)

-include $(DEP)
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INC_DIR) -MMD -c $< -o $@

# Servidor
$(NAME_S): $(BIN_DIR)/$(NAME_S)

$(BIN_DIR)/$(NAME_S): $(OBJ_S)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ $(LDFLAGS_S) -o $@
	ln -sf $@ $(NAME_S_S)

-include $(DEP)
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INC_DIR) -MMD -c $< -o $@
	mkdir -p tmp

.PHONY: clean
clean:
	-rm -rf obj/* $(NAME_C)
	-rm sdstore
	-rm tmp/*

.PHONY: cleans
cleans:
	-rm -rf obj/* $(NAME_S)
	-rm sdstored
	-rm tmp/*

.PHONY: call
call: 
	-rm -rf obj/* $(NAME_C) $(NAME_S)
	-rm sdstore
	-rm sdstored
	-rm tmp/*