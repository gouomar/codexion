NAME = coders/codexion
CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread
SRC = coders/main.c coders/init.c coders/parsing.c coders/utils.c \
	coders/monitor.c coders/routine.c coders/cleanup.c coders/queue.c
OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all
