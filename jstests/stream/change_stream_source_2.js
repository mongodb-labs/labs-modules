// Listening to change streams with non existent collections

function sleep(milliseconds) {
  var start = new Date().getTime();
  for (var i = 0; i < 1e7; i++) {
    if ((new Date().getTime() - start) > milliseconds){
      break;
    }
  }
}

const agg = [
  {
    $stream: [
      {
        $in: {
          db: "test",
          coll: "input"
        }
      }
    ]
  }
];

assert.commandWorked(db.createStream("changeStreamSource2", agg))

sleep(3000)

assert.eq(db.changeStreamSource2.drop(), true)