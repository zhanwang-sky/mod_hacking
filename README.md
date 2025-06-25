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

## Usage

### 使用`Speech`接口

**1. 拨号计划中配置TTS相关参数**
```xml
<action application="set" data="tts_engine=hacking:newbee-tts"/>
<action application="set" data="tts_voice=xiaoming"/>
<action application="set" data="timer_name=soft"/>
```

**2. ESL消息内容**
```
auth ClueCon

sendmsg <uuid>
call-command: execute
execute-app-name: playback
execute-app-arg: say:{Tone-HZ=450,Duration=1}hello world
```
