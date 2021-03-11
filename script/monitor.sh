#!/bin/bash 

#检查监控脚本进程探测
function check_monitor()
{
	normal_num=0
	monitor_path=`pwd`
	
	monitor_id=`ps -ef|grep monitor.sh|grep -v grep|awk '{print $2}'`
	for pid in ${monitor_id[*]}
	do
		tmp=`ls -al /proc/$pid|grep $monitor_path|wc -l`
		if [ $tmp -ge 1 ]
		then
			let normal_num++
		fi
	done

	if [ $normal_num -ge 2 ]
	then
		echo `/bin/date +"%Y-%m-%d %H:%M:%S"`
		echo "monitor.sh have been started on other window"
		exit 0
	fi
}

#进程状态探测
function status()
{
	normal_num=0
	exe_path=`pwd`/$1

	exe_id=`ps -ef|grep ${1}|grep -v grep|grep -v sh|grep -v vi|grep -v gdb|grep -v monitor.sh|grep -v jvm|awk '{print $2}'|uniq` 
	for pid in ${exe_id[*]}
	do
		tmp=`ls -al /proc/$pid|grep $exe_path|wc -l`
		if [ $tmp -ge 1 ]
		then
			normal_num=1
		fi
	done
	
	if [ $normal_num -eq 0 ];then
		return 0
	else
		return 1
	fi
}

#进程检测和启动
function monitor()
{
	status $1
	if [ $? -eq 0 ]
	then
		rm -rf ./core*
		touch ./operation_record.txt
		
		echo "" >>operation_record.txt
		echo `/bin/date +"%Y-%m-%d %H:%M:%S"` >>operation_record.txt
		echo "monitor.sh restarting $1....." >>operation_record.txt
		
		#启动程序
		./run.sh run $2

		echo "">>operation_record.txt
		echo `/bin/date +"%Y-%m-%d %H:%M:%S"` >> operation_record.txt
			
		#打印执行结果
		if [ $? -eq 255 ];then
			echo "$1 is no exit and monitor.sh restart $1 fail!" >>operation_record.txt
			exit -1
		fi
		
		#再进程状态探测
		status $1
		if [ $? -eq 1 ];then
			echo "monitor.sh restart $1 successful!" >>operation_record.txt
		else
			echo "monitor.sh restart $1 fail!" >>operation_record.txt
		fi
	fi
}

 # monitor main
if [ -z $1 ];
then
	echo "program name is empty"
	exit 0
fi

cd `dirname $0`

check_monitor 

while :
do
	monitor $1 $2
	sleep 2
done
 
