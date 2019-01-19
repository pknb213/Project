// 18.11.15 xlsx module로 엑셀 작성하기 !!

const xlsx = require('xlsx');

// Stack structure
function Stack() {
  this.dataStore = [];
  this.top = 0;
  this.push = push;
  this.pop = pop;
  this.peek = peek;
  this.clear = clear;
  this.length = length;
}
function push(element){
  this.dataStore[this.top++] = element;
}
function peek(){
  return this.dataStore[this.top-1];
}
function pop(){
  return this.dataStore[--this.top]
}
function clear(){
  this.top = 0;
}
function length(){
  return this.top;
}

const s = new Stack();

// create a new blank workbook
const wb = xlsx.utils.book_new();
// 새로운 워크시트 추가
const wsName = "SheetJS";
const ws_data = [
  [" S ", " h", " e ", " e ", " t "],
  [1,2,3,4,5,6,7,8,9],
  [],
  [],
];
//var ws_data2 = new Array();
const ws_data2 = Array.from(Array(4),() => Array()); // a[max = 4][0 ~]

/* 시트 변환
 aoa_to_sheet : Data array -> Worksheet
 json_to_sheet : Json array -> Worksheet
 table_to_sheet : DOM TABLE -> Worksheet
 sheet_add_aoa : Data array -> existing worksheet
 sheet_add_json : Json array -> existing worksheet

sheet_to_json
sheet_to_csv
sheet_to_txt
sheet_to_html
sheet_to_formulae

NPM 참조

*/
const ws = xlsx.utils.aoa_to_sheet(["HelloHappyWorld".split("")], ws_data2);
// 시트 추가
xlsx.utils.book_append_sheet(wb,ws,wsName);
// 워크북 쓰기
xlsx.writeFile(wb, 'create.xlsx');

/*for(var i =0 ; i<5; i++)
{
  ws_data2[0][i] = i;
  ws_data2[1][i] = i+10;
  ws_data2[2][i] = i+100;
  ws_data2[3][i] = i +1000;
}
console.log(ws_data);
console.log(ws_data2);
*/
let cout = 0;
console.log("Start : " + s.length());
setInterval( () => {

  let aSensor = cout;
  cout++;
  s.push(aSensor);
  console.log("push~ push~ : " + s.length());

}, 10)

setInterval( () => {

  console.log("Writing....");

  let a = 0;
  while(s.length())
  {
      ws_data2[0][a] = s.pop();
      ws_data2[1][a] = s.length();
      ws_data2[2][a] = s.length();
      ws_data2[3][a] = s.length();
      console.log("pop~ pop~ : " + s.length());
      a++;

      if(!s.length())
      {
        console.log("Clear~");
        s.clear();
        cout = 0;

        xlsx.utils.sheet_add_aoa(ws, ws_data2, {origin:"A3"});
        xlsx.writeFile(wb, 'create.xlsx');

      }
  }

},10000)


/*
// 읽어 오기
var workbook = xlsx.readFile('create.xlsx');

// 워크북의 시트를 가져오기
var firstSheetName = workbook.SheetNames[0];
var fistSheet = workbook.Sheets[firstSheetName];

// 해당 시트의 A1에 내용을 출력
console.log(firstSheetName['A1']);

// 워크북 저장
xlsx.writeFile(workbook, 'out.xlsx');
*/
