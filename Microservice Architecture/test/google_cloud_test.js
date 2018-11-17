const http = require('http');

const port = 3000;
const host = 'localhost';

http.createServer((req,res) => {
  res.writeHead(200, { 'Content-Type' : 'text/plain'});
  res.end('Hello World\n');
}).listen(port, host, () => {
  console.log('Server running at http://' + host + ':' + port + '/');
});

setInterval(() =>{
  var date = new Date();
  console.log('running : ' + date.getHours() + '시 ' + date.getMinutes() + '분 ' + date.getSeconds() + '초');
},10000);
