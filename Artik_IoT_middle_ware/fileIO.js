var fs = require('fs');

var n = 0;
var newData = "Hi new Data ";

// 비동기방식의 파일 읽기, 읽은 후 callback 함수 호출
fs.readFile('readTest.js', 'utf-8', (error, data) => {
  console.log('01 readAsync : %s', data);
});

// 비동기방식 파일 확인
fs.exists("readTest.js", (exists) => {
  console.log("Async 파일 존재 ? : ", exists);
});

// 비동기방식의 파일 생성
fs.writeFile('file01_async.txt', newData, 'utf-8', (e) => {
  if(e){
    console.log(e);
  }else{
    console.log('01 write done ' + n);
  }
});

//----------------------------------------------------------------------

// 동기방식의 파일 읽기, 읽은 후 data 변수에 저장
var data = fs.readFileSync('readTest.js', 'utf-8');
console.log('02 readSync : %s', data);

// 동기방식 파일 확인
var exists = fs.existsSync("readTest.js");
console.log("Sync 파일 존재? : " , exists);

// 동기방식은 callback을 통한 에러처리를 할 수 없기에 try문으로 대체
try{
  fs.writeFileSync('file02_sync.txt', newData, 'utf-8');
  console.log('02 write done ' + n);
}catch(e){
  console.log(e);
}

var exit = 1;
while(exit){
  n++;
  // 내용 추가
  fs.appendFile('./file01_async.txt', '\n' + n + ' : ' + newData, 'utf-8');

  if(n > 5000)
  {
    exit = 0;
  }
}
/* 비동기 파일 복사
fs.copyFile('readTest.js', 'readTestAsyncCopy.txt', (error) => {
  if(error) { throw err};
  console.log('Async readTest 복사됨');
});

// 동기 파일 복사
fs.copyFileSync('readTest.js', 'readTestSyncCopy.txt');
console.log('Sync readTest 복사됨');
*/

// Stream 단위
var infile = fs.createReadStream('./file02_sync.txt',{flags:'r'});
var outfile = fs.createWriteStream('./testStreamFile.txt', {flags:'w'});

// Stream 이벤트 등록
infile.on('data',(data) =>{
  console.log('읽어 들인 데이터', data);
  outfile.write(data);
});

// Stream 이벤트 등록
 infile.on('end', () =>{
   console.log('파일 읽기 종료');
   outfile.end( () => {
     console.log("파일 쓰기 종료");
   })
 })
