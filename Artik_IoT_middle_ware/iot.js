/**********************************************************************************
*             2018_09_11 IoT.js                                                   *
*			  Robot IoT Data Transmission (SI7021, BMA220)             								  *
*			  by CYJ	    v.1025							        		                          	  *
*                                                                                 *
*       1. Mosca Mqtt Transmission method                                         *
*       2. TCP Socket Transmission method                                         *
*       3. Influx Databsae synchronization                                        *
*       4. Google cloud synchronization                                           *
*       5. Artik cloud synchronization                                            *
*       6. Chronograh synchronization                                             *
*                                                                                 *
*                                                                                 *
**********************************************************************************/

var exec = require("child_process").exec;           // Use for running linux scripts
//exec('/root/v5.8.1.0/app/builder/10_17_Gateway_Fullver/build/exe/./10_17_Gateway_Fullver');
var mosca = require('mosca');                       // MQTT Broker, Client
var net = require('net');                           // Socket
var mqtt = require('mqtt');

var volt_flag = 0;
var valArr = new Array();

// InfluxDB ----------------------------------------------------------------------
const Influx = require('influx');
// Connect to a single host with a DSN:
// const influx = new Influx.InfluxDB('http://user:password@host:8086/database')
const influx = new Influx.InfluxDB({
  host: '192.168.0.193',
  database: 'testdb',
  port: 8086,
  // username : '',
  // password : '',
  schema: [
    {
      measurement: 'testKey',
      fields: {
        temper: Influx.FieldType.INTEGER,
        path: Influx.FieldType.STRING,
        duration: Influx.FieldType.INTEGER
      },
      tags: [
        'host'
      ]
    }
  ]
})

influx.getDatabaseNames().then(names =>
  console.log("My Database names are : " + names.join(', '))
);

influx.getMeasurements().then(names =>
  console.log("My measurement names are : " + names.join(', '))
);

  var x = 0;
setInterval (() => {

  x = x+1;
  //influx.createContinuousQuery('test_10s', 'insert test id=1');
  //influx.query('select * from testKey').then(results => {
  //  console.log(results)}).catch(err => {console.log("err")});
  influx.writeMeasurement('testdb', [
    {
    tags: { host: '192.168.0.193'},
    fields: { temper: 77, duration: x},
    }
]).catch(err => {console.log("err : ${err.stack}")});
  console.log("[InfluxDB] Inserting : " + x)
}, 10000);

/*
influx.createContinuousQuery('sample','
  SELECT MEAN(cpu) INTO "7d"."perf"
  FROM "id"."perf" GROUP BY time(1m)
')

influx.writePoints([
  {
    measurement: 'response_times',
    tags: { host: os.hostname() },
    fields: { duration, path: req.path },
  }
]).then(() => {
  return influx.query(`
    select * from response_times
    where host = ${Influx.escape.stringLit(os.hostname())}
    order by time desc
    limit 10
  `)
}).then(rows => {
  rows.forEach(row => console.log(`A request to ${row.path} took ${row.duration}ms`))
})

*/
// Mosca Server ------------------------------------------------------------------
// Setting for MQTT Broker
var pubsubsettings = {
    type: 'mqtt',
    json: false,
    mqtt: require('mqtt'),
    //host: '127.0.0.1',          // Loopback IP address ~ Local IP
    port: 1883                  // Listening port
};

var server = new mosca.Server(pubsubsettings);
server.on('ready', () => {
  console.log('Mosca server is up and running');
  //exec('/home/eai2/uCUBE/dep/adapter/uap/./sendtest');
  //exec('/home/eai2/uCUBE/dep/adapter/uap/./usend');
});
server.on('clientConnected', function (client) {
    var currentTime = new Date();
    console.log(currentTime);
    console.log('Client Connected:', client.id);
});
server.on('clientDisconnected', function (client) {
    console.log('Client Disconnected:', client.id);
    //exec('/root/v5.8.1.0/app/builder/10_17_Gateway_Fullver/build/exe/./10_17_Gateway_Fullver');
});
// Process data when client publish data to broker
server.on('published', function (packet, client) {
    /* cmdString[] is ex) gw / Gate way EUI / topic / Device EUI / sensor ...  */
    var cmdString = packet.topic.toString();        // Get message by topic
    cmdString = cmdString.split('/');               // Split message by '/'
    //console.log(packet.topic);

    switch (cmdString[4])
    {
        case "temp":
        {
            var data = packet.payload.toString();
            var obj = JSON.parse(data);
            var val = JSON.stringify(obj.Temperature);
            valArr[1] = val;
            console.log("Vale(temp) : " + val);
        }
        case "gyro":
        {
            var data = packet.payload.toString();
            var obj = JSON.parse(data);
            var val = JSON.stringify(obj.Gyro_X);
            var val2 = JSON.stringify(obj.Gyro_Y);
            var val3 = JSON.stringify(obj.Gyro_Z);
            valArr[2] = val;
            valArr[3] = val2;
            valArr[4] = val3;

            console.log("Vale(gyro) : " + val + "  " + val2 + "  " + val3);
        }
    }
    if(cmdString[1] == "voltage")
    {
        var data = packet.payload.toString();
        console.log("voltage : " + data);
        volt = data;
        valArr[0] = volt;
        volt_flag = 1;
    }

    /* String Cut function : substring (start, end point)  substr(start, length) */
    /* String Setch functon : indexOf("word")  lastIndexOf("word") */

});

//  Mosca Client ----------------------------------------------------------------
var client = mqtt.connect("mqtt://35.221.120.219:5000");
//var client = mqtt.connect("mqtt://192.168.0.3:1883"); // public :211.106.106.186:1883 - web socket: 9001

// handle incoming message
client.on('message', (topic, message, packet) => {
    console.log("Topic : " + topic + " Message : " + message.toString());
    //client.end();
});
var options = {
    retain : true,
    qos : 1
};
client.on('connect', () => {
    console.log("publishing : " + client.connected);
});
// handle errors
client.on("error",function(error){
    console.log("Can't connect" + error);
    process.exit(1);
});
// Publish
setInterval(() => {
    if (client.connected == true) {
      var currentTime = new Date();
      var date = currentTime.getFromFormat('yyyymmddhhiiss');
      var printTime = date.toString();
      var valStr = valArr.join('/');
      valStr += '/' + printTime;
      valStr = valStr.toString();
      console.log("[Publish] Mqtt : " + valStr);
        client.publish ( "KoKoRo", "Mqtt : " + valStr, options);
    }
},20000);

// Subscribe
// client.subscribe(topic/topic array/topic object , [options], [callback]);

//  Socket TCP Server --------------------------------------------------------------
var socket = net.createServer(function (data) {
    setInterval(function () {
        var d = new Date();
        var date = d.getFromFormat('yyyymmddhhiiss');

      // Send
      //  if (initialize == 1) {
      //    data.write(value[0] + ',' + time[0] + ',' + value[1] + ',' + time[1]); // '\r\n'
            console.log('Sent to the data : ' + date + '\n\n');
      //  }
    }, 3600000); //3600000 : 1h
});
//socket.listen(3000, '35.221.120.219');
socket.listen(9999, '127.0.0.1');

socket.on('connection', function (client) {
    console.log('Socket client connected: ', client.address());
});
socket.on('end', function (client) {
    consol.log('Socket client disconnected: ', client.address());
});

//  Socket TCP Client  ----------------------------------------------------------
setInterval(() => {
  var socketClient = net.connect({port:4000,host:'35.221.120.219'}, () => {
      //var flag = 0;

      socketClient.on('connect', () => {
          console.log('[Socket] Connected to server');
          //flag = 1;
      });
      socketClient.on('end', () => {
          console.log('[Socket] Disconnected to server');
          //flag = 0;
      });
      socketClient.on('data', (data) => {
          console.log("Read from Server : " + data.toString());
          var flag = 1; // Next time, Changed the ACK.
          var currentTime = new Date();
          var date = currentTime.getFromFormat('yyyymmddhhiiss');
          var printTime = date.toString();
          var valStr = valArr.join('/');
          valStr += '/' + printTime;
          valStr = valStr.toString();

          if(flag){
              if(volt_flag){
                console.log('[Socket] sent to data : ' + volt);
                socketClient.write(volt);
                socketClient.end();
              }
              else{
                console.log('[Socket] sent to data : ' + printTime);
                console.log("Sent value " + valStr);
                socketClient.write(valStr);
                socketClient.end();
              }
          }else{
              console.log('[Socket] Server is not connected');
              socketClient.end();
          }
      });
  });
}, 20000);

// Init Function
function dataInitialize() {
    console.log('< DataInitialize Function Execution >');

    console.log('\n\n');
};
/* Date print Function
setInterval(function () {
    var currentTime = new Date();
    var date = currentTime.getFromFormat('yyyymmddhhiiss');
    var printTime = date.toString();
}, 30000); */
// Garbage collection Function
setInterval(function () {    //garbage collection
    if (global.gc) {
        global.gc();
    } else {
        // console.log('Garbage collection unavailable.  Pass --expose-gc ' + 'when launching node to enable forced garbage collection.');
    }
}, 5000);
// Date Format
Date.prototype.getFromFormat = function (format) {
    // Replace A String by B String
    // ex) str = WellcometoHell; str.replace('Hell', 'Heaven'); => str = "wellcometoHeaven"
    var yyyy = this.getFullYear().toString();
    format = format.replace(/yyyy/g, yyyy)
    var mm = (this.getMonth() + 1).toString();
    format = format.replace(/mm/g, (mm[1] ? mm : "0" + mm[0]));
    var dd = this.getDate().toString();
    format = format.replace(/dd/g, (dd[1] ? dd : "0" + dd[0]));
    var hh = this.getHours().toString();
    format = format.replace(/hh/g, (hh[1] ? hh : "0" + hh[0]));
    var ii = this.getMinutes().toString();
    format = format.replace(/ii/g, (ii[1] ? ii : "0" + ii[0]));
    var ss = this.getSeconds().toString();
    format = format.replace(/ss/g, (ss[1] ? ss : "0" + ss[0]));
    return format;
};

// Artik Cloud --------------------------------------------------------------------------
//var CONFIG = require('./config.json');
var CONFIG =  {
	"DEVICE_ID" : "6764aad8c53d449b83eae8aa9950de48",
	"DEVICE_TOKEN" : "08ee4d3d03734f51862d963f6782bc2e"
}

// The port number is specified at
// https://developer.artik.cloud/documentation/connect-the-data/mqtt.html#key-concepts
var credentials = {
  port: 8883,
  username: CONFIG.DEVICE_ID,
  password: CONFIG.DEVICE_TOKEN
}

// Per https://www.npmjs.com/package/mqtt#client, the URL can be on the following protocols:
// 'mqtt', 'mqtts', 'tcp', 'tls', 'ws', 'wss'
//
// For ARTIK Cloud, mqtt client must be SSL-capable.
// use ‘mqtts’, which has security layer on top of mqtt
var client  = mqtt.connect('mqtts://api.artik.cloud', credentials);

// ARTIK Cloud only allows the following 2 paths on MQTT
var PUBLISH_MESSAGE_PATH = "/v1.1/messages/" + CONFIG.DEVICE_ID;
var SUBSCRIBE_ACTION_PATH = "/v1.1/actions/" + CONFIG.DEVICE_ID;


client.on('connect', function () {
  console.log("Start MQTT session ...");
  var sampleData = getSampleData();
  console.log("publishing data:", sampleData);
  console.log("publish path:", PUBLISH_MESSAGE_PATH);

  client.publish(PUBLISH_MESSAGE_PATH, sampleData);
  console.log("Use browser to see your data in realtime https://artik.cloud/my/data")

  // Example for subscribing to receive Action
  // client.subscribe(SUBSCRIBE_ACTION_PATH);
  //client.end()
  //console.log("End MQTT session ...");
})

setInterval(() => {
	var sampleData = getSampleData();
	console.log("publishing : " + client.connected);
	if(client.connected == true){
    console.log("[Artik Cloue] : " + sampleData);
		client.publish(PUBLISH_MESSAGE_PATH, sampleData);
	}
},30000);

function getSampleData() {
  var tempVal = Math.floor((Math.random() * 200 + 100));

  //fields key/value for you ARTIK Cloud device
  return JSON.stringify({
    "data": tempVal
  })
}
