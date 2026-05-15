NAME = codexion
CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread
SRC = main.c init.c parsing.c utils.c \
	monitor.c routine.c cleanup.c queue.c \
	state.c dongles.c
OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all
