var string = 'Nice to Meet you';

var buffer1 = new Buffer(15);
var buffer2 = new Buffer('이것은 버퍼 테스트입니다','utf8');
var buffer3 = new Buffer(string,'utf8');

console.log('1st Buffer : %s (%d)', buffer1.toString(), Buffer.byteLength(buffer1));
console.log('2st Buffer : %s (%d)', buffer2.toString(), Buffer.byteLength(buffer2));
console.log('3st Buffer : %s (%d)', buffer3.toString(), Buffer.byteLength(buffer3));

console.log('1st 버퍼 객체의 타입 [%s] [%s]', Buffer.isBuffer(buffer1), typeof buffer1);
console.log('2st 버퍼 객체의 타입 [%s] [%s]', Buffer.isBuffer(buffer2), typeof buffer2);
console.log('3st 버퍼 객체의 타입 [%s] [%s]', Buffer.isBuffer(buffer3), typeof buffer3);

var str1 = buffer1.toString('utf8', 0, Buffer.byteLength(buffer1));
var str2 = buffer2.toString('utf8');

console.log('1st 문자열 변수 : %s [%s]', str1, typeof str1);
console.log('2st 문자열 변수 : %s [%s]', str2, typeof str2);

var mergeBuffer = Buffer.concat([buffer2, buffer2]);
console.log('합친 버퍼 : %s (%d) (%s)', mergeBuffer.toString(), Buffer.byteLength(mergeBuffer), typeof mergeBuffer);

mergeBuffer = mergeBuffer.toString();
console.log('toString() 후 버퍼 : %s (%d) (%s)', mergeBuffer.toString(), Buffer.byteLength(mergeBuffer), typeof mergeBuffer);
