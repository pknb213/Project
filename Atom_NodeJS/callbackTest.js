function add(a,b,callback){
  console.log('Add Start ');
  var result = a + b;
  callback(result);
  console.log('After callback');

  var history = function(){
    console.log('History Start');
    return a + ' + ' + b + ' = ' + result;
  }
  return history;
}

var add_History = add(10,20,function(result){
  console.log('Called');
  console.log('Result : %d', result);
});

console.log('Function Operation : ' + add_History() );
