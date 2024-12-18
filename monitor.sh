#!/bin/bash

# this function is purely for myself
notify(){
    URL=$(cat ntfy_url) 
#    pgrep rabbitmq-server >/dev/null

#    if [ $? -eq 0 ]; then
       curl -H "t: Finished a test!" -H p:3 -H "tags: heavy_check_mark" -d "It was logged at logs/${prefix}-..." ${URL}
#  else
#       curl -H "t: RabbitMQ crashed!" -H p:4 -H "tags: warning" -d "Fuzzing process terminated" ${URL}
#    fi
}


$* &
child=$!
prefix=$(date +%Y-%m-%d-%H-%M-%S)

mkdir "./logs" 2>/dev/null

while kill -0 $child 2>/dev/null
do
    pmap  ${child} | tail -n 1 >> "./logs/${prefix}-memory.log"
    top -p ${child} -b -n 1 -d "0.001" | awk 'NR>7 && NR<13 {printf "%-4s %-4s %-s\n",$11,$9,$5}' >> "./logs/${prefix}-top.log"
    sleep "0,01"
done

notify

amqp-publish -r test -b noop -s 10.0.0.1:9001
sleep 30
#top -b -n 1 --pid = ${child} | tail -2 | head -3
