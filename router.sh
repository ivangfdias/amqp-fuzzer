
sudo ip addr add 10.0.0.2 dev enp1s0
sudo ip route add 10.0.0.1/32 dev enp1s0
pgrep -f tests-$1.sh
while [ $? == 0 ]
do
	if [ ip route | grep "10.0.0.1" == "" ]; then

		sudo ip route add 10.0.0.1/32 dev enp1s0
	fi

	pgrep -f tests.sh
done
