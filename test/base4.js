var gIdentify = 2147483647;
function getIdentify() {
    if (0 == getIdentify) {
        gIdentify = 2147483647;
    } else {
        gIdentify--;
    }
    return gIdentify;
}

var data = {
  control:
    {
        mac: '84F3EB0C9344',
        action: 2,
        identify: getIdentify()
    }
};
var content = JSON.stringify(data);
console.log(content)

var mqtt = require('mqtt')
//var client  = mqtt.connect('mqtt://test.mosquitto.org')
var client  = mqtt.connect('mqtt://192.168.200.202')
//var client  = mqtt.connect('mqtt://faye.weixinxk.net')
console.log(client.options.clientId);

client.on('connect', function () {
  client.publish('WM_84F3EB83AD7D', content, function (err) {
    if (!err) {
      console.log("published");
      client.end()
    }
  })
})
