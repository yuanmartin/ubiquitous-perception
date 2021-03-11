#!/bin/bash 

#进程名称
exe_name=UbiquitousPerception

#相关路径
cur_path=`pwd`
key_word=${cur_path//\//\\/}
monitor_path=$cur_path/monitor.sh
crontab_local=/var/spool/cron/crontabs/root

#删除本目录下监控脚本的定时执行
function del_crontab()
{
	sed -i "/$key_word/d" $crontab_local
}

#配置本目录下监控脚本的定时执行
function cfg_crontab()
{
	touch $crontab_local
	sed -i "/$key_word/d" $crontab_local
	echo "*/1 * * * * $monitor_path $exe_name background" >> $crontab_local
}

#获取进程运行状态
function get_status()
{
	normal_num=0
	exe_path=`pwd`/$exe_name

	exe_id=`ps -ef|grep ${exe_name}|grep -v grep|grep -v sh|grep -v vi|grep -v gdb|grep -v monitor.sh|grep -v jvm|awk '{print $2}'|uniq` 
	for pid in ${exe_id[*]}
	do
		tmp=`ls -al /proc/$pid|grep $exe_path|wc -l`
		if [ $tmp -ge 1 ]
		then
			normal_num=1
		fi
	done

	if [ $normal_num -eq 0 ];then
		echo $exe_name is not running
		return 0
	else
		echo $exe_name is running
		return 1
	fi
}

#关闭监控脚本进程
function close_monitor()
{
	monitor_path=`pwd`	
	monitor_id=`ps -ef|grep monitor.sh|grep -v grep|awk '{print $2}'`
	for pid in ${monitor_id[*]}
	do
		tmp=`ls -al /proc/$pid|grep $monitor_path|wc -l`
		if [ $tmp -ge 1 ]
		then
			kill -9 $pid
			echo "monitor proc stoping ..." && sleep 1
		fi
	done
}

#进程状态探测
function check_status()
{
	echo "">>operation_record.txt
	echo `/bin/date +"%Y-%m-%d %H:%M:%S"` >> operation_record.txt
	
	get_status
	if [ $? -eq 1 ];then
		echo "$1 $exe_name successful!"
		echo "$1 $exe_name successful!" >>operation_record.txt
		return 1
	else
		echo "$1 $exe_name fail!"
		echo "$1 $exe_name fail!" >>operation_record.txt
		return 0
	fi
}

#检查可执行文件是否存在
function check_exefile()
{	
	exe_path=`pwd`/$1
	if [ ! -x "$exe_path" ]
	then
		echo "exe file no exist"
		exit -1
	fi
}

#开放端口
function check_port()
{
	firewall-cmd --zone=public --add-port=7788/tcp --permanent >/dev/null
	firewall-cmd --zone=public --add-port=12316/tcp --permanent >/dev/null
	firewall-cmd --reload >/dev/null
}

function sys_config()
{
	ulimit -n 65535
	ulimit -c unlimited
	ulimit -v unlimited
	ulimit -m unlimited
	ulimit -s 10240
	export LD_LIBRARY_PATH=`pwd`/3rdparty:$LD_LIBRARY_PATH
	export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
	ldconfig >/dev/null 2>&1
}

#前端运行
function RunInForthground()
{
	touch ./operation_record.txt
	echo "$1 is started in forthground" >>operation_record.txt
	./$1
}

#后台运行
function RunInBackground()
{
	touch ./operation_record.txt
	echo "$1 is started in background" >>operation_record.txt
	./$1 >/dev/null 2>&1 &
}

function status()
{
	get_status
}

function stop()
{
	#关闭监控进程
	close_monitor
	
	#删除crontab配置
	del_crontab

	#进程状态探测
	get_status
	if [ $? -eq 0 ];then
		return
	fi
	
	#关闭进程
	exe_path=`pwd`/$exe_name
	exe_id=`ps -ef|grep $exe_name|grep -v grep|grep -v sh|grep -v vi|grep -v gdb|grep -v monitor.sh|grep -v jvm|awk '{print $2}'|uniq` 
	for pid in ${exe_id[*]}
	do
		tmp=`ls -al /proc/$pid|grep $exe_path|wc -l`
		if [ $tmp -ge 1 ]
		then
			kill -9 $pid
			echo "$exe_name stoping ..." && sleep 3
		fi
	done
	
	#记录直接停止操作
	if [ $# -eq 0 ];then
		echo "">>operation_record.txt
		echo `/bin/date +"%Y-%m-%d %H:%M:%S"` >> operation_record.txt
		
		get_status
		if [ $? -eq 0 ];then
			echo "stop $exe_name successful!" >>operation_record.txt
			echo "stop $exe_name successful!" 
		else
			echo "stop $exe_name fail!" >>operation_record.txt
			echo "stop $exe_name fail!" 
		fi
	fi
}

function run()
{
	check_exefile $1

	check_port

	sys_config

	if [ -z $2 ];
	then
		RunInForthground $1
	else
		RunInBackground $1
	fi
}

function exe()
{
	#程序启动
	run $exe_name $2

	#打印和记录操作及结果
	touch ./operation_record.txt
	echo "">>operation_record.txt
	echo `/bin/date +"%Y-%m-%d %H:%M:%S"` >> operation_record.txt
	
	if [ $? -eq 255 ];then
		echo "$1 $exe_name fail!"
		echo "$1 $exe_name fail!" >> operation_record.txt
		exit -1
	else
		echo "$1..." && sleep 3
	fi
	
	#再检测进程状态
	check_status $1

	#成功启动情况下配置定时器
	if [ $? -eq 1 ];then
		cfg_crontab
	fi
}

function start()
{
	#进程状态探测
	get_status
	if [ $? -eq 1 ];then
		exit 0
	fi
	
	#进程执行
	exe start $1
}

function restart()
{
	#关闭进程
	stop restart
	
	#进程执行
	exe restart $1
}

#main entrance
cd `dirname $0`

case "$1" in
		'status')
		status
		;;
		'start')
		start $2
		;;
		'stop')
		stop
		;;
		'restart')
		restart $2
		;;
		'run')
		run $exe_name $2
		;;
		*)
		echo "usage: $0 {start -[f]|restart -[f]|stop|status}"
		exit 0
		;;
esac