const net = require('net');
let count = 0;

const socket = net.createServer( (data) => {

let flag = 1;
  while(flag)
  {
    data.write(count).catch(err => {console.log(`err : ${err.stack}`)});
    count++;
  }

});

socket.listen(9999, '127.0.0.1');

socket.on('connection', function (client) {
    console.log('Socket client connected: ', client.address());
});
socket.on('end', function (client) {
    consol.log('Socket client disconnected: ', client.address());
});

setInterval ( () => {
  const socketClient = net.connect({port:9999, host:'192.168.0.10'}, () => {
    socketClient.on('connect', () => {
      console.log('[Socket] Connected to server');
    });
    socketClient.on('end', () => {
        console.log('[Socket] Disconnected to server');
    });
    socketClient.on('data', (data) => {
      console.log("Read from Server : " + data.toString());
      console.log('[Socket] sent to data : ' + count);
      socketClient.write(count);
      socketClient.end();
    });

  });

});
