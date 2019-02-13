PORT=90000
IP_SERVER="127.0.0.1"
USERS_DATA_FILE="users_data_file"

build: server client

server: function-server.o server.o database.o
	gcc -Wall -Werror -Wextra -g -o server function-server.o server.o database.o
	rm -fr $^

function-server.o: src/function-server.c
	gcc -Wall -Werror -Wextra -g -lnsl -c src/function-server.c

database.o: src/utils/database.c
	gcc -Wall -Werror -Wextra -g -c src/utils/database.c

server.o: src/server.c
	gcc -Wall -Werror -Wextra -g -c src/server.c

client: client.o function-client.o
	gcc -Wall -Werror -Wextra -g -o client client.o function-client.o
	rm -fr $^

client.o: src/client.c
	gcc -Wall -Werror -Wextra -g -c src/client.c

function-client.o: src/function-client.c 
	gcc -Wall -Werror -Wextra -g -lnsl -c src/function-client.c

clean:
	rm -fr *.o utils/*.o server client
clean_log:
	rm -fr *.log

run_server: server
	./server ${PORT} ${USERS_DATA_FILE}

run_client: client
	./client ${IP_SERVER} ${PORT}
