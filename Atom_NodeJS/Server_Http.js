var http = require('http');
var fs = require('fs');

var server = http.createServer();

var port = 3000;
server.listen(port, function(){
  console.log('Web Server 시작되었습니다. [%d]', port);
});

server.on('connection', function(socket){
  var addr = socket.address();
  console.log('클라이언트 접속 : %s [%d]', addr.address, addr.port);
});

server.on('request', function(req,res){
  console.log('클라이언트으로 부터 요청');
  //console.dir(req);

var filename = './Atom_NodeJS/흑우.png';
var infile = fs.createReadStream(filename,{flags:'r'});
var filelength = 0;
var curlength = 0;

fs.stat(filename,function(err,stats){
  filelength = stats.size;
});

res.writeHead(200,{"Content-Type": "image/png"});

infile.on('readable', function(){
  var chunk;
  while(null != (chunk = infile.read())){
    console.log('읽은 데이터 크기 : %d byte', chunk.length);
    curlength += chunk.length;
    res.write(chunk, 'utf8', function(err){
      console.log('파일 부분 쓰기 완료 : %d, 파일 크기 : %d', curlength, filelength);
      if(curlength >= filelength){
        res.end();
      }
    });
  }
});

/* Pipe 이용 간단하게 출력
var filename = './Atom_NodeJS/흑우.png';
var infile = fs.createReadStream(filename,{flags:'r'});
infile.pipe(res);
*/
/* PNG 보여주기
  var filename = './Atom_NodeJS/흑우.png';
  fs.readFile(filename, function(err,data){
    res.writeHead(200, {"Content-Type": "image/png"});
    res.write(data);
    res.end();
  });
*/
/* // HTML 보여주기
  res.writeHead(200, {"Content-Type": "text/html; charset=utf-8"});
  res.write("<!DOCTYPE html>");
  res.write("<html>");
  res.write(" <head>");
  res.write("   <title>응답 페이지</title>");
  res.write(" </head>");
  res.write(" <body>");
  res.write("   <h1>Node.JS Html Server으로부터 응답</h1>");
  res.write(" </body>");
  res.write("</html>");
  res.end();*/
});

server.on('close', function(){
  console.log('서버 다운');
});

server.on('custom',function(){
  console.log('커스텀 이벤트 입니다.');
});
