# LeptHttp

A lept HTTP web server, coding in Linux.



**Usage：**

- git clone the code.

```cmd
git clone git@github.com:M3stark/LeptHttp.git
```

- cd to the dir.

```cmd
cd ~./LeptHttp
```

- make the project.

```cmd
make
```

- run target with port name.

```cmd
./webserver 10000
```

<img src="/home/mike/Projects/LeptHttp/resources/screenshoot/usage_sample.png" alt="usage_sample" style="zoom:67%;" />

- Open the browser, input the the resources.

```cmd
http:// your link : port name / index.html
```



**Highlights：**

- Using Thread Pool, Non-blocking Socket, Epoll and Proactor in this work.
- Using State Machine to resolve the HTTP request.
- Using Webbench to test presure.




