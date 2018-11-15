/* 2018. 11. 07
    자체 평가 보고서 Test용

*/

var mqtt = require('mqtt');
var mqttHost = "mqtt://35.221.120.219:5000";

var jsonformat = {
  src : "192.168.0.86",
  des : "35.221.120.219",
  dport : "5000",
  date : "2018.11.07",
  temp : 44,
  gyro : {
    x : 1,
    y : 2,
    z : 3
  },
  pwrcons : 777,
  gwId : "TMP1234567890"
}

var mqttClient = mqtt.connect(mqttHost);

mqttClient.on('message', (topic, message, packet) => {
    console.log("Topic : " + topic + " Message : " + message.toString());
});

var options = {
    retain : true,
    qos : 1
};

mqttClient.on('connect', () => {
    console.log("connect : " + mqttClient.connected);
});

mqttClient.on("error",function(error){
    console.log("Can't connect" + error);
    process.exit(1);
});

setInterval(() => {
    if (mqttClient.connected == true) {
      var currentTime = new Date();
      var date = currentTime.getFromFormat('yyyymmddhhiiss');
      var printTime = date.toString();

      console.log("[Publish] Mqtt time : " + printTime);
      console.log(jsonformat);
      mqttClient.publish ( "report", "Mqtt : " + jsonformat, options);
    }
},20000);

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
