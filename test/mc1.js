var mqtt = require('mqtt')
//var client  = mqtt.connect('mqtt://test.mosquitto.org')
//var client  = mqtt.connect('mqtt://192.168.200.202')
var client  = mqtt.connect('mqtt://faye.weixinxk.net')
console.log(new Date().toLocaleString() + " : " + client.options.clientId);

client.on('connect', function () {
  client.subscribe("/child_online", function (err) {
    if (!err) {
    }
  })
  client.subscribe("/child_offline", function (err) {
    if (!err) {
    }
  }) 
  client.subscribe("/root_offline", function (err) {
    if (!err) {
    }
  })
  client.subscribe("/root_online", function (err) {
    if (!err) {
    }
  })
  client.subscribe("/action_resp", function (err) {
    if (!err) {
    }
  })
  client.subscribe("/interrupt_resp", function (err) {
    if (!err) {
    }
  })
  client.subscribe("WM_84F3EB0C9344", function (err) {
    if (!err) {
    }
  })
})

client.on('message', function (topic, message) {
  // message is Buffer
  console.log(new Date().toLocaleString() + " : " + topic + " : " + message.toString())
  //client.end()
})

/*
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
        mac: '',
        action: 0,
        identify: 0
    }   
};



const readline = require('readline');

rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

function publish(topic, data) {
  console.log('fun(publish) - topic : ' + topic + ", data : " + data)
  client.publish(topic, data, function (err) {
    if (!err) {
      console.log("published");
      client.end()
    }   
  })  
}

var content;
function mainTask() {
  rl.on('close',function(){
    rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout
    });
    mainTask();
  });

  rl.question('topic:', (topic) => {
    // TODO: Log the answer in a database
    rl.question('mac:', (mac) => {
      rl.question('action:', (action) => {
        data.control.mac = mac;
        data.control.action = Number(action);
        data.control.identify = Number(getIdentify());
        content = JSON.stringify(data);
        //console.log('topic : ' + topic + ", content : " + content);
        publish(topic, content);
        rl.close();
      });
    });
  });
}


mainTask();
*/
