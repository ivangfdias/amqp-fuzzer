sudo ./router.sh number
sleep 1

./monitor.sh ./main.out 10.0.0.1 -n 200 -R 0 -P 0 -v 
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 200 -R 1 -P 0 -v
sleep 10

exit 1
./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 2 -P 0 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 3 -P 0 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 4 -P 0 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 0 -P 1 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 1 -P 1 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 2 -P 1 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 3 -P 1 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 4 -P 1 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 0 -P 2 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 1 -P 2 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 2 -P 2 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 3 -P 2 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 4 -P 2 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 0 -P 3 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 1 -P 3 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 2 -P 3 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 3 -P 3 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 4 -P 3 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 0 -P 4 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 1 -P 4 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 2 -P 4 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 3 -P 4 -v
sleep 10

./monitor.sh ./main.out 10.0.0.1 -n 8000 -R 4 -P 4 -v
sleep 10

amqp-publish -r finish -b placeholder -s 10.0.0.1:9002
