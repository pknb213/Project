const http = require('http');
const net = require('net');

// Mosca MQTT Broker (Server)
var mosca = require('mosca'); var pubsubsettings = {
	type: 'mqtt',
	json: false,
	mqtt: require('mqtt'),

	port: 5000
};
var server = new mosca.Server(pubsubsettings);
server.on('ready', () => {
	console.log('Mosca Server running: 5000');
});

server.on('published', (packet,client, cb) => {
	/*if(packet.topic.indexOf('echo') === 0) {
		console.log('ON PUBLISHED', packet.payload.toString(), 'on topic', packet.topic);
		return cb();
	}

	var newPacket = {
		topic: 'echo/' + packet.topic,
		payload: packet.payload,
		retain: packet.retain,
		qos: packet.qos
	};
	console.log('newPacket', newPacket);
*/
	console.log(packet.payload.toString());
//	server.publish(newPacket, cb);
});

server.on('clientConnected', (client) => {
	console.log('Client Connected : ' + client.id);
});

server.on('clientDisconnected', (client) => {
	console.log('Client Disconnected : ' + client.id);
});

//Socket Server
var server = net.createServer((socket) => {
	console.log(socket.address().address + " connected ");

	socket.on('data', (data) => {
		console.log('Socket : ' + data);
	});

	socket.on('close', () => {
		console.log('client disconnected');
	});
		socket.write('welcome to server');
});
server.listen(4000, () => {
	console.log('Socket Server Listen on 4000');
});

// Http Server
const port = 3000;
const host = '10.146.0.2';

http.createServer((req,res) => {
	res.writeHead(200, {'Content-Type':'text/plain'});
	res.end('Hell world\n');
}).listen(port,host,() => {
	console.log("Server running at http://" + host + ":" + port + '/');
});

setInterval(() => {
	var date = new Date();
	console.log("running : " + date.getHours() + '시' + date.getMinutes() + "분\n");
},600000);
