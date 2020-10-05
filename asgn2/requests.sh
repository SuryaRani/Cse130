curl localhost:8080/httpserver &
curl --upload-file httpserver localhost:8080/hello2 &
curl --head localhost:8080/httpserver & 

wait
