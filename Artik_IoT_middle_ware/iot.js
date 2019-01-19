/**********************************************************************************
*             2018_09_11 IoT.js                                                   *
*			  Robot IoT Data Transmission (SI7021, BMA220, SDM120)             								  *
*			  by CYJ	    v.1025							        		                          	  *
*                                                                                 *
*       1. Mosca Mqtt Transmission method                                         *
*       2. TCP Socket Transmission method                                         *
*       3. Influx Databsae synchronization                                        *
*       4. Google cloud synchronization                                           *
*       5. Artik cloud synchronization                                            *
*       6. Chronograh synchronization                                             *
*       7. Si7021, BMA220, SDM120 communication                                   *
*                                                                                 *
*                                                                                 *
*                                                                                 *
**********************************************************************************/

var exec = require("child_process").exec;           // Use for running linux scripts
//exec('/root/v5.8.1.0/app/builder/10_17_Gateway_Fullver/build/exe/./10_17_Gateway_Fullver');
var mosca = require('mosca');                       // MQTT Broker, Client
var net = require('net');                           // Socket
var mqtt = require('mqtt');

var mqttHost = "mqtt://35.221.120.219:5000"; // Google cloud MQTT Server ip
//var mqttHost = "mqtt://192.168.0.3:1883"; // public :211.106.106.186:1883 - web socket: 9001

// 데이터 저장 용도 배열
var valArr = new Array();

// Xlsx Module (Excel) -----------------------------------------------------------
// Added workbook, lows, columns
const workbook = xlsx.utils.book_new();
let _rows1, _rows2, _rows3, _rows4 = 0;
// Added worksheet
const wsName = "SheetJS";
const ws_data = Array.from(Array(8), () => Array());
const ws = xlsx.utils.aoa_to_sheet(["Mini factoring] Excel file".split(" ")], ws_data);
// 시트 추가
xlsx.utils.book_append_sheet(workbook,ws,wsName);
// 워크북 쓰기
xlsx.writeFile(workbook, 'Log.xlsx');

// InfluxDB ----------------------------------------------------------------------
const Influx = require('influx');
// Connect to a single host with a DSN:
// const influx = new Influx.InfluxDB('http://user:password@host:8086/database')
const influx = new Influx.InfluxDB({
  host: '192.168.0.193', // Localhost
  database: 'testdb',
  port: 8086,
  // username : '',
  // password : '',
  schema: [
    {
      measurement: 'testKey',
      fields: {
        seq: Influx.FieldType.DOUBLE,
        gyroX: Influx.FieldType.INTEGER,
        gyroY: Influx.FieldType.INTEGER,
        gyroZ: Influx.FieldType.INTEGER,
        temper: Influx.FieldType.INTEGER,
        path: Influx.FieldType.STRING,
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

/* ------- 주기마다 Influx DB에 저장 --------
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
*/
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
          var _temp = parseFloat(JSON.stringify(obj.Temperature));
          var _seq = parseInt(JSON.stringify(obj.Seq));
          console.log("Val(temp) : " + _seq + " " + _temp);

          // Xlsx
          ws_data[0][_rows1] = _temperature;
          ws_data[1][_rows1] = _seq;
          _rows1++;

          influx.writeMeasurement('testdb', [
            {
            tags: { host: '192.168.0.193'},
            fields: { seq: _seq, temper: _temp},
            }
          ]).catch(err => {console.log(`err : ${err.stack}`)});
          console.log("[InfluxDB] Inserting : " + _seq + " " + _temp);
          break;
        }
        case "gyro":
        {
          var data = packet.payload.toString();
          var obj = JSON.parse(data);
          var _gyroX = parseFloat(JSON.stringify(obj.Gyro_X));
          var _gyroY = parseFloat(JSON.stringify(obj.Gyro_Y));
          var _gyroZ = parseFloat(JSON.stringify(obj.Gyro_Z));
          var _seq = parseInt(JSON.stringify(obj.Seq));
          console.log("Val(gyro) : " + _seq + " " + _gyroX + "  " + _gyroY + "  " + _gyroZ);

          influx.writeMeasurement('testdb', [
            {
            tags: { host: '192.168.0.193'},
            fields: { seq: _seq, gyroX: _gyroX, gyroY: _gyroY, gyroZ: _gyroZ},
            }
          ]).catch(err => {console.log(`err : ${err.stack}`)});
          console.log("[InfluxDB] Inserting : " + _seq + " " + _gyroX + ' ' + _gyroY + ' ' + _gyroZ);
          break;
        }
    }
    if(cmdString[1] == "power")
    {
      var data = packet.payload.toString();
      var _power = parseFloat(data);
      if(_power != 0)
      {
          _power = _power / 1000;;
      }
      else
      {
          _power = 0;
      }
      console.log("Val(power) : " + _power);

      influx.writeMeasurement('test02', [
      {
          tags: { host: '211.106.106.186'},
          fields: { power: _power },
      }
      ]).catch(err => {console.log(`err : ${err.stack}`)});
      console.log("[InfluxDB] Inserting : " + _power);
    }

    /* String Cut function : substring (start, end point)  substr(start, length) */
    /* String Setch functon : indexOf("word")  lastIndexOf("word") */
});

// Auto excel writing function
setInterval( () => {

    if(!zigbeeConnection)
    {
        xlsx.utils.sheet_add_aoa(ws, ws_data, {origin:"A3"});
        xlsx.writeFile(workbook, 'Log.xlsx');
    }
}, 60000);

//  Mosca Client ----------------------------------------------------------------
var client = mqtt.connect(mqttHost);

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
