/**********************************************************************************
*             2018_01_08 IoT.js  (Temperature)                                    *
*			  Posco PLTE Data Transmission										  *
*			  by CYJ	    v.0419							        			  *
**********************************************************************************/

var exec = require("child_process").exec;           // Use for running linux scripts
exec('/root/v5.8.1.0/app/builder/10_17_Gateway_Fullver/build/exe/./10_17_Gateway_Fullver');
var mosca = require('mosca');                       // MQTT Broker
var net = require('net');                           // Socket Server
var i = 0;
var initialize = 0;
var tmpValue = 0;
var makeFlag = 0;

var eui = new Array();          // List of device EUI
var temp = new Array();         // List for device temperature
var time = new Array();         // List for device send time
var gyro = new Array();

var value = new Array();

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

function dataInitialize() {

    var d = new Date();
    initialize = 1;
    var date = d.getFromFormat('yyyymmddhhiiss');
    date = date.toString();

    for (i = 0; i < 8; i++) {

        value[i] = '00000'; // Temp (5)
        time[i] = date;

        console.log('Data [' + i + '] : ' + value[i] + ' Time [' + i + '] : ' + time[i]);
    }
};

function createData() {

    var realData = 0;
    makeFlag = 0;

    tmpValue = Number(tmpValue);
    if (eui.length != 0 && tmpValue != 0)
    {
        realData = tmpValue;

        realData = Number(realData);
        console.log('real Data (' + realData + ')');
        makeFlag = 1;
    }
    else if (eui.length == 0 || tmpValue == 0)
    {
        if (eui.length != 0)
        {
            for(i = 0; i < 8; i++)
            {
                if(value[i] != 0)
                {
                    realData = value[i];
                    realData = Number(realData);
                    console.log('(Basic) real Data ' + realData + ')');
                    makeFlag = 1;
                    break;
                }
                else if(value[i] == 0)
                {
                    if(i == 7)
                    {
                        realData = 2000;
                        console.log('(Basic) fake Data ' + realData + ')');
                        makeFlag = 1;
                        break;
                    }
                }
            }
        }
        else if (eui.length == 0)
        {
            realData = 2000;
            console.log('fake Data (' + realData + ')');
            makeFlag = 1;
        }
        else
        {
            console.log('Error');
        }
    }
    else{
        console.log('Error');
    }
    makeingRandomValue(realData);
};

function makeingRandomValue(realData) {

    var d = new Date();
    var date = d.getFromFormat('yyyymmddhhiiss');

    if (makeFlag) {
        for (i = 0; i < 8; i++) {
            // Random Value ( 0 ~ 199 )
            var rData = Math.random() * 100;
            rData = parseInt(rData);

            //var dumpData = 2000 + rData;
            var dumpData = realData + rData;
            dumpData = '0' + dumpData;


            var rSec = Math.random() * 50 + 10;
            var rMin = Math.random() * 50 + 10;
            rSec = parseInt(rSec);
            rMin = parseInt(rMin);
            var dumpTime = date.toString();
            dumpTime = dumpTime.substring(0, 10);
            dumpTime = dumpTime + rMin + rSec;

            value[i] = dumpData;
            time[i] = dumpTime

            console.log('Data [' + i + '] : ' + value[i] + ' Time [' + i + '] : ' + time[i]);
            console.log('Size : ' + value.length + ' ' + time.length);
        }
        console.log('Random value create Succ');
    }
    else if(makeFlag == 0)
    {
        console.log('Random value create Fail');
    }
    else
    {
        console.log('Error');
    }
};
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
	        // Date
	        tmpValue = 0;
	        var d = new Date();
	        var date = d.getFromFormat('yyyymmddhhiiss');
	        var time_tmp;
	        time_tmp = date.toString();

            // String
	        var readingData = packet.payload.toString();
	        var jsonText = JSON.parse(readingData);
	        var tmp = cmdString[3];
	        console.log('<Basic> EUI : ' + tmp);

	        var value_tmp = "00000";

	        if (eui.length == 0) {
	            console.log('< Empty the eui[0] >');
	            eui[0] = tmp;
	            time[0] = time_tmp;
	            //eui.push(tmp);
	            //time.push(time_tmp);
	            console.log('Insert the New eui[0] : ' + eui[0] + ' Time : ' + time[0]);
	        }
	        else if (eui.length != 0) {
	            console.log('< eui length : ' + eui.length + '>');
	            var flag = 0;
	            for (i = 1; i <= eui.length; i++) {
	                // console.log('<' + i + 'st Serch> : ' + eui.length);
	                if (eui[i - 1] == tmp) {
	                    console.log('Already eui[' + (i - 1) + '] is same !');
	                    console.log('Changing to the value[' + (value[i - 1]) + '] -> New value[' + value_tmp + ']');
	                    eui[i - 1] = tmp;
	                    value[i - 1] = value_tmp;
	                    time[i - 1] = time_tmp;
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
	                eui[eui.length] = tmp;
	                value[eui.length] = value_tmp;
	                time[eui.length] = time_tmp;
	                // eui.push(tmp);
                    // value.push(value_tmp)
	                //time.push(time_tmp);
	                console.log('Insert the New eui : ' + eui.length + ' Data : ' + value_tmp + ' Time : ' + time_tmp);
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
	        var readingData = packet.payload.toString();
	        var jsonText = JSON.parse(readingData);

	        var tmp;
	        tmp = cmdString[3];
	        console.log('<Temp> EUI : ' + tmp);

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

	        var value_tmp;
	        value_tmp = JSON.stringify(jsonText.Temperature);

	        if (value_tmp.length == 5) {
	            console.log('< Temp is Length == 5 >');
	            value_tmp = value_tmp.substring(0, 4);
	            value_tmp = '0' + value_tmp;
	        }
	        else if (value_tmp.length == 4) {
	            console.log('< Temp is Length == 4 >');
	            value_tmp = value_tmp.substring(0, 3);
	            value_tmp = "00" + value_tmp;
	        }
	        tmpValue = value_tmp;

	        //-------------------------------------------------------------------------------
	        // Date

	        var d = new Date();
	        var date = d.getFromFormat('yyyymmddhhiiss');
	        var time_tmp;
	        time_tmp = date.toString();

	        //-------------------------------------------------------------------------------
	        // Substitution

	        if (eui.length == 0) {
	            console.log('< Empty the eui[0] >');
	            eui[0] = tmp;
	            value[0] = value_tmp;
	            time[0] = time_tmp;
	            console.log('Insert the New eui[0] : ' + eui[0] + ' Data : ' + value[0] + ' Time : ' + time[0]);
	        }
	        else if (eui.length != 0) {
	            console.log('< eui length : ' + eui.length + '>');
	            var flag = 0;
	            for (i = 1; i <= eui.length; i++) {
	                // console.log('<' + i + 'st Serch> : ' + eui.length);
	                if (eui[i - 1] == tmp) {
	                    console.log('Already eui[' + (i - 1) + '] is same !');
	                    console.log('Changing to the value[' + (value[i - 1]) + '] -> New value[' + value_tmp + ']');
	                    eui[i - 1] = tmp;
	                    value[i - 1] = value_tmp;
	                    time[i - 1] = time_tmp;
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
	                eui[eui.length] = tmp;
	                value[eui.length] = value_tmp;
	                time[eui.length] = time_tmp;
	                console.log('Insert the New eui : ' + eui.length + ' Data : ' + value_tmp + ' Time : ' + time_tmp);
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
	        var readingData = packet.payload.toString();
	        var jsonText = JSON.parse(readingData);

	        var tmp;
	        tmp = cmdString[3];
	        console.log('<Gyro> EUI : ' + tmp);

	        // Gyro 

	        var value_tmp;
	        value_tmp = JSON.stringify(jsonText.Gyro_X);
	        tmpValue = value_tmp;

	        value_tmp = Math.abs(value_tmp);
	        value_tmp = JSON.stringify(value_tmp);

	        if (value_tmp.length == 5) {
	            console.log('< Gyro length is 5 >');
	            value_tmp = value_tmp.substring(0, 5);
	            value_tmp = '0' + value_tmp;
	        }
	        else if (value_tmp.length == 4) {
	            console.log('< Gyro length is 4 >');
	            value_tmp = value_tmp.substring(0, 4);
	            value_tmp = "00" + value_tmp;
	        }
	        else if (value_tmp.length == 3) {
	            console.log('< Gyro length is 3 >');
	            value_tmp = value_tmp.substring(0, 3);
	            value_tmp = "000" + value_tmp;
	        }
	        else if (value_tmp.length == 2) {
	            console.log('< Gyro length is 2 >');
	            value_tmp = value_tmp.substring(0, 2);
	            value_tmp = "0000" + value_tmp;
	        }
	        else if (value_tmp.length == 1) {
	            console.log('< Gyro length is 1 >');
	            value_tmp = value_tmp.substring(0, 1);
	            value_tmp = "00000" + value_tmp;
	        }

	        // Date

	        var d = new Date();
	        var date = d.getFromFormat('yyyymmddhhiiss');
	        var time_tmp;
	        time_tmp = date.toString();

	        // Substitution

	        if (eui.length == 0) {
	            console.log('< Empty the eui[0] >');
	            eui[0] = tmp;
	            value[0] = value_tmp;
	            time[0] = time_tmp;
	            console.log('Insert the New eui[0] : ' + eui[0] + ' Data : ' + value[0] + ' Time : ' + time[0]);
	        }
	        else if (eui.length != 0) {
	            console.log('< eui length : ' + eui.length + '>');
	            var flag = 0;
	            for (i = 1; i <= eui.length; i++) {
	                // console.log('<' + i + 'st Serch> : ' + eui.length);
	                if (eui[i - 1] == tmp) {
	                    console.log('Already eui[' + (i - 1) + '] is same !');
	                    console.log('Changing to the value[' + (value[i - 1]) + '] -> New value[' + value_tmp + ']');
	                    eui[i - 1] = tmp;
	                    value[i - 1] = value_tmp;
	                    time[i - 1] = time_tmp;
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
	                eui[eui.length] = tmp;
	                value[eui.length] = value_tmp;
	                time[eui.length] = time_tmp;
	                console.log('Insert the New eui : ' + eui.length + ' Data : ' + value_tmp + ' Time : ' + time_tmp);
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
	        var readingData = packet.payload.toString();
	        var jsonText = JSON.parse(readingData);
	        break;
	    }
	}

    //----------------------------------------------------------------------------------------
    /* String Cut function : substring (start, end point)  substr(start, length) */
    /* String Setch functon : indexOf("word")  lastIndexOf("word") */

});

var socket = net.createServer(function (data) {
    setInterval(function () {
       
        var d = new Date();
        var date = d.getFromFormat('yyyymmddhhiiss');


        /*----------------------------------- Send ---------------------------------------------*/
        
        if (initialize == 0)
        {
            dataInitialize();
        }

        if (initialize == 1)
        {
            data.write(value[0] + ',' + time[0] + ',' + value[1] + ',' + time[1] + ',' + value[2] + ',' + time[2] + ','
                     + value[3] + ',' + time[3] + ',' + value[4] + ',' + time[4] + ',' + value[5] + ',' + time[5] + ','
                     + value[6] + ',' + time[6] + ',' + value[7] + ',' + time[7]); // '\r\n'
            //value.write(value[0] +','+ time[0]);
            console.log('Sent to the data : ' + date);
        }

        createData();

        // Sort
        // . . . . . . 

    }, 30000); //3600000 : 1h
});

setInterval(function () {
    var currentTime = new Date();
    var date = currentTime.getFromFormat('yyyymmddhhiiss');
    var printTime = date.toString();
    console.log('Now date : ' + printTime);

   /* for (i = 0; i < eui.length; i++) {
        if (eui[i] != undefined) {
            console.log('Device : ' + eui[i] + ' Data : ' + value[i] + ' Time : ' + time[i]);
        }
    }*/
    console.log('Size : ' + value.length + ' ' + time.length);
    for (i = 0; i < value.length; i++) {
        if (value[i] != undefined) {
            console.log('['+i+'] Device : ' + eui[i] + ' Data : ' + value[i] + ' Time : ' + time[i]);
        }
    }
}, 10000);600000

setInterval(function () {    //garbage collection
    if (global.gc) {
        global.gc();
    } else {
        // console.log('Garbage collection unavailable.  Pass --expose-gc ' + 'when launching node to enable forced garbage collection.');
    }
}, 5000);

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

socket.on('connection', function (client) {
    console.log('Socket client connected: ', client.address());
});
socket.on('end', function (client) {
    consol.log('Socket client disconnected: ', client.address());
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
