//-------------------- Code Pattern --------------------
/* Function Assosication pattern
var printUser = require('./user').printUser;

printUser();
*/
// Instance Object Assosication pattern
var User = require('./user').User;
var user = new User('Ari', '아리');
user.printUser();

//
//-----------------------------------------------------

/*
var user = require('./user');

function showUser(){
//  return '이름 : ' + user.getUser().name + ', 그룹 : ' + user.group.name;
  return '이름 : ' + user.getUser().name + ', 그룹 : ' + user.group.name;
}
console.log('사용자 정보 : %s', showUser());
*/
/*
var require = function(path){
  var exports = {
    getUser : function(){
      return {id: 'Ari', name:'아리'};
    },
    group:{id:'Group', name:'모스트'}
  };
  return exports;
};

var user = require('./user');

function showUser(){
  return user.getUser().name + ', ' + user.group.name;
}
console.log('사용자 정보 : %s', showUser());
*/
