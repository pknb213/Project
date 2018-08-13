var fs = require('fs');

var infile = fs.createReadStream('./Atom_NodeJS/output.txt', {flags:'r'} );
var outfile = fs.createWriteStream('./Atom_NodeJS/output2.txt', {flags:'w'} );

infile.on('data',function(data){
  console.log('읽은 Data : ', data);
  outfile.write(data);
});

infile.on('end',function(){
  console.log('파일 읽기 종료');
  outfile.end(function(){
    console.log('파일 쓰기 종료');
  });
});
