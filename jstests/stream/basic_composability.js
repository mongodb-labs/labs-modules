const aggA = [
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

assert.commandWorked(db.createStream("streamA", aggA))

const aggB = [
 {
   $stream: [
     {
       $in: "streamA"
     },
     {
      $merge: {
        into: {
          db: "test",
          coll: "output"
        }
      }
    }
   ]
 }
];

assert.commandWorked(db.createStream("streamB", aggB))

const ITERATIONS = 10

for (let j = 0; j < ITERATIONS; j++) {
  assert.commandWorked(db.getSiblingDB("test")["input"].insert({"foo": "bar"}))
}

let countOutput = 0
let countCapped = 0
let i = 0

// Need to poll output collection
while (i < 5) {
  sleep(200)
  countOutput = db.getSiblingDB("test")["output"].count()
  countCapped = db.getSiblingDB("stream-logs")["temp-streamA"].count()
  i++
}

assert.eq(countOutput, ITERATIONS);
assert.eq(countOutput, ITERATIONS);
assert.eq(db.getSiblingDB("test")["input"].drop(), true)
assert.eq(db.getSiblingDB("test")["output"].drop(), true)
assert.eq(db.streamA.drop(), true)
assert.eq(db.streamB.drop(), true)