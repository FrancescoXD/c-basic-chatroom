#mkdir build
gcc -c -O2 src/server_utils.c -I./include/ -Wall -Wextra -Werror -pedantic -o build/server_utils
gcc -O2 src/client.c -I./include/ -Wall -Wextra -Werror -pedantic -lpthread -o build/client
gcc -O2 src/server.c build/server_utils -I./include/ -Wall -Wextra -Werror -pedantic -o build/server
