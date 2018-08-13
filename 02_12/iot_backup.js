/**********************************************************************************
*             2018_01_08 IoT.js                                                   *
*			  Posco PLTE Data Transmission										  *
*			  by CYJ	    v.0222							        			  *
**********************************************************************************/

var exec = require("child_process").exec;           // Use for running linux scripts
exec('/root/v5.8.1.0/app/builder/10_17_Gateway_Fullver/build/exe/./10_17_Gateway_Fullver');
var mosca = require('mosca');                       // MQTT Broker
var net = require('net');                           // Socket Server
var i = 0;
var a, b = 0;

var eui = new Array();          // List of device EUI
var temp = new Array();         // List for device temperature
var time = new Array();         // List for device send time
var gyro = new Array();

// Setting for MQTT Broker
var pubsubsettings = {
    type: 'mqtt',
    json: false,
    mqtt: require('mqtt'),
    host: '127.0.0.1',          // Loopback IP address ~ Local IP
    port: 1883                  // Listening port
};
var server = new mosca.Server(pubsubsettings);
server.on('ready', setup);
function setup() {
    console.log('Mosca server is up and running');
    //exec('/home/eai2/uCUBE/dep/adapter/uap/./sendtest');
    //exec('/home/eai2/uCUBE/dep/adapter/uap/./usend');
}

// Process data when client publish data to broker 
server.on('published', function (packet, client) {
    /* cmdString[] is ex) gw / Gate way EUI / topic / Device EUI / sensor ...  */
    var cmdString = packet.topic.toString();        // Get message by topic
    cmdString = cmdString.split('/');               // Split message by '/'
    //console.log(packet.topic);

    /* Test code : array show
	var i = 0;
	for (i = 0; i < cmdString.length ; i++)
	{
	    console.log(cmdString);
	}
    */

    
	switch(cmdString[4])
	{
	    case "basic":
	    {
	        console.log('<Basic>');
            // Date
	        var d = new Date();
	        var date = d.getFromFormat('yyyymmddhhiiss');
	        var time_tmp;
	        time_tmp = date.toString();

            // String
	        var readingData = packet.payload.toString();
	        var jsonText = JSON.parse(readingData);

	        var tmp;
	        tmp = cmdString[3];
	        console.log('EUI : ' + tmp);

	        if (eui.length == 0) {
	            console.log('Empty the eui[0] : ' + eui[0] + ' Time : ' + time[0]);
	            eui.push(tmp);
	            time.push(time_tmp);
	            console.log('Insert the New eui[0] : ' + eui[0] + ' Time : ' + time[0]);
	        }
	        else if (eui.length != 0) {
	            console.log('< eui length : ' + eui.length + '>');
	            var flag = 0;
	            for (i = 1; i <= eui.length; i++) {
	                // console.log('<' + i + 'st Serch> : ' + eui.length);
	                if (eui[i - 1] == tmp) {
	                    console.log('eui[' + (i - 1) + '] and tmp eui is same !');
	                    eui[i - 1] = tmp;
	                    time[i - 1] = time_tmp;
	                    console.log('eui[' + (i - 1) + '] data changed to tmp: ' + tmp);
	                    flag = 1;
	                }
	                else if (eui[i - 1] != tmp) {
	                    // continue . . .
	                }
	                else {
	                    console.log('error Desu');
	                }
	            }
	            if (flag != 1) {
	                eui.push(tmp);
	                time.push(time_tmp);
	                console.log('Insert the New eui : ' + tmp);
	            }
	            else if (flag == 1) {
	                // continue . . .
	            }
	            else {
	                console.log('error Desu');
	            }
	        }
	        else {
	            console.log('error Desu');
	        }

	        break;
	    }
	    case "temp":
	     {
	        console.log('<Temp>');
	        var readingData = packet.payload.toString();
	        var jsonText = JSON.parse(readingData);

	        var tmp;
	        tmp = cmdString[3];
	        console.log('EUI : ' + tmp);

	        //-------------------------------------------------------------------------------
	        /* EUI Check

	        if (tmp == '0x000B57FFFE366427') {
	            console.log('It is 366427');
	        }
	        else if (tmp == '0x000B57FFFE366C04') {
	            console.log('It is 366C04');
	        }
	        else if (tmp == '0x000B57FFFE36685A') {
	            console.log('It is 36685A');
	        }
	        else {
	            console.log('It isnt');
	        }
            */
	        //-------------------------------------------------------------------------------
	        // Temperature 

	        var temp_tmp;
	        temp_tmp = JSON.stringify(jsonText.Temperature);

	        if (temp_tmp.length == 5) {
	            console.log('< Temp is Length == 5 >');
	            temp_tmp = temp_tmp.substring(0, 4);
	            temp_tmp = '0' + temp_tmp;
	        }
	        else if (temp_tmp.length == 4) {
	            console.log('< Temp is Length == 4 >');
	            temp_tmp = temp_tmp.substring(0, 3);
	            temp_tmp = "00" + temp_tmp;
	        }

	        //-------------------------------------------------------------------------------
	        // Date

	        var d = new Date();
	        var date = d.getFromFormat('yyyymmddhhiiss');
	        var time_tmp;
	        time_tmp = date.toString();

	        //-------------------------------------------------------------------------------
	        // Substitution

	        if (eui.length == 0) {
	            console.log('Empty the eui[0] : ' + eui[0] + ' Temp : ' + temp[0] + ' Time : ' + time[0]);
	            eui.push(tmp);
	            temp.push(temp_tmp);
	            time.push(time_tmp);
	            console.log('Insert the New eui[0] : ' + eui[0] + ' Temp : ' + temp[0] + ' Time : ' + time[0]);
	        }
	        else if (eui.length != 0) {
	            console.log('< eui length : ' + eui.length + '>');
	            var flag = 0;
	            for (i = 1; i <= eui.length; i++) {
	                // console.log('<' + i + 'st Serch> : ' + eui.length);
	                if (eui[i - 1] == tmp) {
	                    console.log('eui[' + (i - 1) + '] and tmp eui is same !');
	                    eui[i - 1] = tmp;
	                    temp[i - 1] = temp_tmp;
	                    time[i - 1] = time_tmp;
	                    console.log('eui[' + (i - 1) + '] data changed to tmp: ' + tmp);
	                    flag = 1;
	                }
	                else if (eui[i - 1] != tmp) {
	                    // continue . . .
	                }
	                else {
	                    console.log('error Desu');
	                }
	            }
	            if (flag != 1) {
	                eui.push(tmp);
	                temp.push(temp_tmp);
	                time.push(time_tmp);
	                console.log('Insert the New eui : ' + tmp);
	            }
	            else if (flag == 1) {
	                // continue . . .
	            }
	            else {
	                console.log('error Desu');
	            }
	        }
	        else {
	            console.log('error Desu');
	        }

	        break;
	    }

	    case "gyro":
	    {
	        console.log('<Gyro>');
	        var readingData = packet.payload.toString();
	        var jsonText = JSON.parse(readingData);

	        var tmp;
	        tmp = cmdString[3];
	        console.log('EUI : ' + tmp);

	        // Gyro 

	        var gyro_tmp;
	        gyro_tmp = JSON.stringify(jsonText.Gyro_X);

	        if (gyro_tmp.length == 5) {
	            console.log('< Gyrop length is 5 >');
	            gyro_tmp = gyro_tmp.substring(0, 5);
	            gyro_tmp = '0' + gyro_tmp;
	        }
	        else if (gyro_tmp.length == 4) {
	            console.log('< Gyrop length is 4 >');
	            gyro_tmp = gyro_tmp.substring(0, 4);
	            gyro_tmp = "00" + gyro_tmp;
	        }
	        else if (gyro_tmp.length == 3) {
	            console.log('< Gyrop length is 3 >');
	            gyro_tmp = gyro_tmp.substring(0, 3);
	            gyro_tmp = "000" + gyro_tmp;
	        }
	        else if (gyro_tmp.length == 2) {
	            console.log('< Gyrop length is 2 >');
	            gyro_tmp = gyro_tmp.substring(0, 2);
	            gyro_tmp = "0000" + gyro_tmp;
	        }
	        else if (gyro_tmp.length == 1) {
	            console.log('< Gyrop length is 1 >');
	            gyro_tmp = gyro_tmp.substring(0, 1);
	            gyro_tmp = "00000" + gyro_tmp;
	        }

	        // Date

	        var d = new Date();
	        var date = d.getFromFormat('yyyymmddhhiiss');
	        var time_tmp;
	        time_tmp = date.toString();

	        // Substitution

	        if (eui.length == 0) {
	            console.log('Empty the eui[0] : ' + eui[0] + ' Gyro : ' + gyro[0] + ' Time : ' + time[0]);
	            eui.push(tmp);
	            gyro.push(gyro_tmp);
	            time.push(time_tmp);
	            console.log('Insert the New eui[0] : ' + eui[0] + ' Gyro : ' + gyro[0] + ' Time : ' + time[0]);
	        }
	        else if (eui.length != 0) {
	            console.log('< eui length : ' + eui.length + '>');
	            var flag = 0;
	            for (i = 1; i <= eui.length; i++) {
	                // console.log('<' + i + 'st Serch> : ' + eui.length);
	                if (eui[i - 1] == tmp) {
	                    console.log('eui[' + (i - 1) + '] and tmp eui is same !');
	                    eui[i - 1] = tmp;
	                    gyro[i - 1] = gyro_tmp;
	                    time[i - 1] = time_tmp;
	                    console.log('eui[' + (i - 1) + '] data changed to tmp: ' + tmp);
	                    flag = 1;
	                }
	                else if (eui[i - 1] != tmp) {
	                    // continue . . .
	                }
	                else {
	                    console.log('error Desu');
	                }
	            }
	            if (flag != 1) {
	                eui.push(tmp);
	                gyro.push(gyro_tmp);
	                time.push(time_tmp);
	                console.log('Insert the New eui : ' + tmp);
	            }
	            else if (flag == 1) {
	                // continue . . .
	            }
	            else {
	                console.log('error Desu');
	            }
	        }
	        else {
	            console.log('error Desu');
	        }

	        break;
	    }
	    case "noise":
	    {
	        console.log('<Noise>');
	        var data = packet.payload.toString();
	        var obj = JSON.parse(data);
	        break;
	    }
	}

    //----------------------------------------------------------------------------------------
    /* String Cut function : substring (start, end point)  substr(start, length) */
    /* String Setch functon : indexOf("word")  lastIndexOf("word") */

});

var socket = net.createServer(function (data) {
    setInterval(function () {

        for (i = 0; i < 8; i++) {
            if ((temp[i] == undefined) || (time[i] == undefined)) {
                temp[i] = '00000';
                var d = new Date();
                var date = d.getFromFormat('yyyymmddhhiiss');
                time[i] = date.toString();
                //time[i] == '00000000000000';
            }
        }
        // Sort
        // . . . . . . 

        data.write(temp[0] + ',' + time[0] + ',' + temp[1] + ',' + time[1] + ',' + temp[2] + ',' + time[2] + ',' + temp[3] + ',' + time[3] + ',' + temp[4] + ',' + time[4] + ',' + temp[5] + ',' + time[5] + ',' + temp[6] + ',' + time[6] + ',' + temp[7] + ',' + time[7] + '\r\n');
        console.log('Sent to the data');
    }, 60000);
});
server.on('clientConnected', function (client) {
    var currentTime = new Date();
    console.log(currentTime);
    console.log('Client Connected:', client.id);
});
server.on('clientDisconnected', function (client) {
    console.log('Client Disconnected:', client.id);
    exec('/root/v5.8.1.0/app/builder/10_17_Gateway_Fullver/build/exe/./10_17_Gateway_Fullver');
});
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
socket.listen(9999, '127.0.0.1');

setInterval(function () {
    var currentTime = new Date();
    var date = currentTime.getFromFormat('yyyymmddhhiiss');
    var printTime = date.toString();
    console.log('Now date : ' + printTime);

    for (i = 0; i < eui.length; i++) {
        if (eui[i] != undefined) {
            console.log('Device : ' + eui[i] + ' Temperature : ' + temp[i] + ' Time : ' + time[i]);
        }
    }
}, 600000);

setInterval(function () {    //garbage collection
    if (global.gc) {
        global.gc();
    } else {
        // console.log('Garbage collection unavailable.  Pass --expose-gc ' + 'when launching node to enable forced garbage collection.');
    }
}, 5000);
