## 硬件平台
  本例子是为了演示在 LinkltONE 平台上使用 YUNBA 服务的。LinkltONE 是针对智能穿戴以及物联网方面的开源开发板，微处理是联发科 MT2502A 。
关于开发板的详情 [LintltONE](http://www.seeedstudio.com/wiki/LinkIt_ONE%E5%BC%80%E5%8F%91%E6%9D%BF)。
  
## 云巴 SDK 库的安装
  云巴提供了开源的 yunba-ardunio-SDK 。该 sdk 是基于 arduino-mqtt 做了调整。首先，从这里下载 [yunba-ardunio-sdk](https://github.com/alexbank/yunba-arduino-sdk)，复制放到 arduino 库目录下。

## 例子
　　我们分别做了 WIFI 以及 GPRS 网络的例子。

### WIFI 例子
  我们使用 ardunio IDE 创建 arduino sketch ，文件名为 YunbaWIFI.ino 。

1）添加所需要的库

```
#include "SPI.h"

#include <LWiFi.h>
#include <MQTTClient.h>
#include <LWiFiClient.h>
#include <ArduinoJson.h>

```

注意的是: 用户通过 ardunio IDE 上下载 ArduinoJson 库。

2）定义需要接入 WIFI 的 ssid 以及密码。

```
#define WIFI_AP "<your ssid>"
#define WIFI_PASSWORD "<your WIFI AP password>"
#define WIFI_AUTH LWIFI_WPA
``` 

3) 定义你的 Appkey , topic , device id 等。

```
const char yunba_appkey[] = "<your-appkey>";
const char yunba_topic[] = "<your-topic>";
const char yunba_devid[] = "<your-device-id>";

char client_id[56];
char username[56];
char password[56];
char device_id[56];

char broker_addr[56];
uint16_t port
```

client_id ，username ，password 为从云巴注册服务器获取到的 mqtt 的注册信息。

broker_addr ，port 为从云巴 ticket 服务器获取到 broker 地址以及端口。

4)　获取注册信息以及 broker 地址

```
bool get_host_v2(const char *appkey, char *url) {
 ....
}

bool setup_with_appkey_and_devid(const char* appkey, const char *deviceid) {
 ....
}
```

函数 get_host_v2　把获取的 broker 信息保存到 broker_addr ，port 变量中。

函数 setup_with_appkey_and_devid 获取到 mqtt 注册信息 保存到 client_id，username，password 变量中。

5)　setup 函数。

```
void setup() {
  Serial.begin(9600);
  Serial.println("Serial set up");
  Serial.println("Connecting to AP");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD))) {
    Serial.println(" . ");
    delay(1000);
  }
  Serial.println("Connected to AP");

  get_host_v2(yunba_appkey, url);
  setup_with_appkey_and_devid(yunba_appkey, yunba_devid);

  get_ip_pair(url, broker_addr, &port);
 
  client.begin(broker_addr, port, net);
  connect();
}
```

在上面函数中做了初始化串口，连接 WIFI ，获取 broker 和注册信息，连接 broker 。

6）loop 函数

```
void loop() {
  client.loop();

  check_connect(6000);
  if (millis() - lastMillis > 20000) {
    lastMillis = millis();
    client.publish(yunba_topic, "publish test");
  }
}
```

该函数中 client.loop() 为 SDK 检查 mqtt 进来的消息等。

使用函数 client.publish 在频道 yunba_topic ，推送一条消息。

7）接收消息回调

```
void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();
}
```

8) 扩展命令消息回调

```
void extMessageReceived(EXTED_CMD cmd, int status, String payload, unsigned int length) {
  Serial.print("incoming ext message: ");
  Serial.print(cmd);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();
}
```

### GPRS 例子
  
  我们使用 yunba-ardunio-SDK 做了 GPS 追踪器。用户可以订阅相应的频道，来接收追踪器发来的地理位置等信息。

  方法与步骤大体与 WIFI 例子相同。这里我们仅介绍不同的地方。

1）包含 GPRS 库

```
#include <LGPRS.h>
#include <LGPRSClient.h>
```

2) setup 函数

```
void setup() {
  Serial.begin(9600);
  Serial.println("Serial set up");
  Serial.println("Connecting to GPRS");
  while (!LGPRS.attachGPRS("3gnet", "", "")) {
    Serial.println(" . ");
    delay(1000);
  }
  Serial.println("Connected to GPRS");
  LGPS.powerOn();

  get_host_v2(yunba_appkey, url);
  setup_with_appkey_and_devid(yunba_appkey, yunba_devid);

  get_ip_pair(url, broker_addr, &port);
  client.begin(broker_addr, port, net);

  connect();
}
```

与 WIFI 例子结构大体相同。这里需要打开 GPRS 连接以及 GPS 。

3) loop 函数

```
void loop() {
  client.loop();
  check_connect(6000);

  if (millis() - lastMillis > 20000) {
    lastMillis = millis();
    char GPS_formatted[130];
    getGPSData(g_info, GPS_formatted);
    client.publish(yunba_topic, gmaps_buffer);
  }
}
```

这里使用函数 getGPSData 对获取到的 GPS 进行处理，并打包到 JSON 。

JSON 的格式为

```json
{"sensor":"GPS", "status":"<unfixed or fixed>", "lat":"latitude", "log": "longitude"}
```

使用 client.publish 发送当前的地理位置信息，速度等信息。

用户可以把接收到的信息显示在地图上，这样就完成了自己 GPS 追踪器。
