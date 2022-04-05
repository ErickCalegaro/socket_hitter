clear
echo -e '\e[1;40;93mCompilando Server...\e[m'
gcc "./src/server.c" -o "./server" -lpthread -lcurl
echo -e '\e[1;40;36mCompilação Concluida!\e[m'