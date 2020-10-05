curl localhost:8080/hello &
curl --head localhost:8080/hello2 &
curl --upload-file hello4 localhost:8080/hello3 &
curl localhost:8080/hello &
curl --head localhost:8080/hello2 &
curl --upload-file hello4 localhost:8080/hello5 &
curl localhost:8080/hello &
curl --head localhost:8080/hello2 &
curl --upload-file hello4 localhost:8080/hello6 &
curl --upload-file 11oNUC7-y64ZKfw4LzK89-SPPZM localhost:8080/hello7 &

wait
