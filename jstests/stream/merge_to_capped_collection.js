// Change stream tests

const agg = [
  {
    $stream: [
      {
        $in: {
          db: "test",
          coll: "input"
        }
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

assert.commandWorked(db.createStream("mergeCappedCollection", agg))
assert.commandWorked(db.getSiblingDB("test")["input"].insert({"foo": "bar"}))

let countOutput = 0
let countCapped = 0
let i = 0

// Need to poll output collection
while (i < 5) {
  sleep(500)
  countOutput = db.getSiblingDB("test")["output"].count()
  countCapped = db.getSiblingDB("stream-logs")["temp-mergeCappedCollection"].count()

  if (countOutput > 0 && countCapped > 0) {
    break
  }

  i++
}

assert.eq(countOutput, 1);
assert.eq(countCapped, 1);
assert.eq(db.getSiblingDB("test")["input"].drop(), true)
assert.eq(db.getSiblingDB("test")["output"].drop(), true)
assert.eq(db.mergeCappedCollection.drop(), true)