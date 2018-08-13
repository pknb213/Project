//-------------------- Code Patten --------------------
/* Function Assosication pattern
exports.printUser = function(){
  console.log('Ari User');
};
*/
// Instance Object Assosication pattern
function User(id,name){
  this.id = id;
  this.name = name;
}
User.prototype.getUser = function(){
  return {id:this.id, name:this.name};
};
User.prototype.group = {id:'Group', name:'모스트'};
User.prototype.printUser = function(){
  console.log('User 이름 : %s, Group 이름 : %s', this.name, this.group.name);
};
//module.exports = new User('Ari', '아리');
//exports.user = new User('Ari', '아리');
//module.exports = User;
exports.User = User;
//-----------------------------------------------------
/* Same the bottom code
exports.getUser = function(){
  return {id : 'Ari', name : '아리'};
};

exports.group = {id : 'group01', name : '친구'};
*/
/*
exports = {
  getUser : function(){
    return {id:'Ari', name:'아리'};
  },
  group : {id:'group1', name:'모스트'}
};
*/
/*
var user = {
  getUser : function(){
    return {id:'Ari', name:'아리'};
  },
  group:{id:'group', name:'모스트'}
};

module.exports = user;
module.exports = function(){
  gorup = {id:'gorup', name:'모스트2'};
  return {id:'Zed', name:'제드'};
};
*/
/*
module.exports = {
  getUser : function(){
    return {id:'Ari', name:'아리'};
  },
  group : {id:'Group', name:'모스트'}
};
exports.group = {id:'Group2', name:'워스트'};
*/
