sudo ./router.sh time
sleep 1

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 0 -P 0
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 1 -P 0
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 2 -P 0
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 3 -P 0
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 4 -P 0
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 0 -P 1
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 1 -P 1
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 2 -P 1
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 3 -P 1
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 4 -P 1
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 0 -P 2
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 1 -P 2
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 2 -P 2
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 3 -P 2
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 4 -P 2
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 0 -P 3
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 1 -P 3
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 2 -P 3
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 3 -P 3
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 4 -P 3
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 0 -P 4
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 1 -P 4
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 2 -P 4
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 3 -P 4
sleep 5

./monitor.sh ./main.out 10.0.0.1 -t 1800 -R 4 -P 4
sleep 5

amqp-publish -r finish -b placeholder -s 10.0.0.1:9002
