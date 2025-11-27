# system-call-http

gcc ./calc_server.c -o ./calc_server && ./calc_server

curl "http://localhost:8000/calc?query=2+10"