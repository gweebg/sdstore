# Compilador
CC = gcc

# Flags de compilação
CFLAGS   = -std=gnu99 -Wall -Wextra -O2 -Wundef -Wshadow -Wstrict-prototypes -Wwrite-strings -Wunreachable-code -Werror

# Flags de linking
LDFLAGS  = -lm

# Variáveis
SRC_DIR = src 
INC_DIR = includes
BIN_DIR = obj

SRC = $(shell find src -type f \( -iname "*.c" ! -iname "testing.c" \))
OBJ = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRC))
DEP = $(OBJ:%.o=%.d)

NAME_C   = client
NAME_CLIENT = client

# Programa principal
$(NAME_C): cleant
$(NAME_C): $(BIN_DIR)/$(NAME_C)

$(BIN_DIR)/$(NAME_C): $(OBJ)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	ln -sf $@ $(NAME_CLIENT)
	mkdir -p entrada saida

-include $(DEP)
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INC_DIR) -MMD -c $< -o $@

.PHONY: clean
clean:
	-rm -rf obj/* $(NAME_CLIENT)
	-rm client

