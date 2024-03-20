# Nginx HTTP Rest Module

## Setup

You get to have an Nginx distribution. In this case, this was developed with the 1.24.0 release (https://nginx.org/en/download.html)

After you have a distribution of Nginx, you have to clone this repository into the distribution project structure

```bash
cd nginx-x.x.x/
git clone https://github.com/mayoras/nginx_http_rest_module.git
```

The folder where the custom modules are stored can get any name, for simplicity use `modules`

```bash
mv nginx_http_rest_module modules
```

The rest is just following the steps made [on this post](https://codedamn.com/news/backend/custom-module-development). Although the module can be added to Nginx as an static module (meaning it is compiled along Nginx), it also can be added dynamically, we are going to do this last one (for doing it statically follow the steps on online post).

First configure the Nginx build.
```bash
./configure --prefix=/usr/local/nginx --add-dynamic-module=modules/ngx_http_rest_module
```

Second, build and link and then install to system (it is going to be installed differently depending on the `prefix` we configured, this prefix will determine the root Nginx is going to look for).

```bash
make
sudo make install
```

Third and last, run Nginx!
```bash
sudo /usr/local/nginx/sbin/nginx
```

### Dynamic loading
For those who built the addon modules dynamically, you have to tell explicitly Nginx to load that module in the config file `<prefix>/conf/nginx.conf`
```nginx
load_module modules/ngx_http_rest_module.so;
...

http {
    ...
}
```
