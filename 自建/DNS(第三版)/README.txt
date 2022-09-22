简单介绍一下exe程序的调试和使用方法

1.首先在shell窗口使用 ipconfig/all 命令查看当前PC使用的因特网DNS服务器的IP地址（假设为192.168.3.1）

2.进入“网络和Internet设置” 修改当前PC的默认因特网DNS服务器的IP地址，改为自己编写的DNS服务器IP地址 127.0.0.1（本地回环地址）（教程见 https://www.fayagu.com/139354.html）

3.编译链接源码得到.exe文件，在该可执行文件的目录下打开shell终端，输入 XXX.exe [-d | -dd] [dns-server-ipaddr] [filename] 运行该可执行文件（也就相当于打开DNS服务器）
[dns-server-ipaddr]参数可填入上述IP地址192.168.3.1或网上找可以使用的因特网DNS服务器的IP地址，这一步是为了能够让自己编写的服务器能在查询不到域名时求助因特网DNS服务器
[-d | -dd]参数表示输出详细或简略运行日志
[filename]参数就是自定义的URL-IP映射表(包括后缀名)
例如：DNS中继服务器.exe -d 192.168.3.1 dnsrelay.txt

4.接着我们就可以进行调试，这里有两种方法
一种是新建一个shell窗口使用 nslookup 命令进行DNS解析调试
一种是使用Wireshark工具进行DNS解析调试（前提是你能玩得来这个软件）

注意：
1.进行调试的时候最好不要运行其他联网软件，否则服务器会一直printf调试日志
2.调试结束后记得修改DNS为原来的因特网DNS服务器，否则PC没法上网了
3.就算关闭了其他联网软件Windows会自带一些联网行为也会触发请求查询行为

其他命令：
ipconfig/displaydns: 查看当前 dns cache 的内容以确认程序执行结果的正确性（这个我还没玩明白，不知道是从服务器的shell看还是客户端的shell看，有没有可能是必须要在服务器运行的状态下才能看到）
ipconfig/flushdns: 清除 dns cache 中缓存的所有 DNS 记录
