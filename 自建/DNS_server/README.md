# 计算机⽹络课程设计

## 小组成员

**2020212183** 	杨再俨

**2020212181** 	王焕捷

**2020212259** 	何奕骁 

## 小组分工

杨再俨：负责项⽬整体框架的设计和搭建，实现服务器处理来⾃本地客户端和外部服务器的消息（ID转换、URL转换等），进⾏Windows平台下程序的调试和输 出⽇志的编写。 

王焕捷：负责对项⽬的程序进⾏调研和查找类似开源代码。同时负责参与调试， 部分⽂档的编写和了解DNS的报⽂结构，对整体程序的⽂件进⾏模块的分割确定 了功能实现的基本流程和数据结构与变量接⼝。 

何奕骁 ：负责“域名-IP 地址”对照表（DNS_table）、cache以及查询算法的实现；负责解决Windows/Linux跨平台问题与Linux平台上的编译与测试；并同时协助优化项⽬框架与socket设计。 

## 提交文件说明

report 为本次实验的实验报告

src文件夹中有本次实验的源代码

DNS_server文件夹中有源代码在Windows与Linux平台编译好的可执行文件：

​	main.exe文件是Windows平台下编译好的可执行文件，可以根据实验报告中所言的测试方法进行测试；

​	main.out文件是Linux平台下使用gcc编译好的可执行文件，可以根据实验报告中所言的测试方法进行测试。

