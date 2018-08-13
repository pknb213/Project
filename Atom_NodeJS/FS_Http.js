var fs = require('fs');
var http = require('http');
var inname = './Atom_NodeJS/output.txt';

var server = http.createServer(function(req,res){
  var instream = fs.createReadStream(inname, {flags: 'r'});
  instream.pipe(res);
});
server.listen(7001,'127.0.0.1');
