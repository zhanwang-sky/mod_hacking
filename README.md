# mod_hacking

## Build

**1. 设置`PKG_CONFIG_PATH`环境变量**
```
export PKG_CONFIG_PATH=<freeswitch_install_prefix>/lib/pkgconfig
```

**2. 编译并安装`mod_hacking`**
```
./bootstrap.sh
./configure
make
(sudo) make install
```
