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

assert.commandWorked(db.createStream("changeStreamSource", agg))
assert.commandWorked(db.getSiblingDB("test")["input"].insert({"foo": "bar"}))

let count = 0
let i = 0

// Need to poll output collection
while (i < 5) {
  sleep(500)
  count = db.getSiblingDB("test")["output"].count()

  if (count > 0) {
    break
  }

  i++
}

assert.eq(count, 1);
assert.eq(db.changeStreamSource.drop(), true)
assert.eq(db.getSiblingDB("test")["input"].drop(), true)
assert.eq(db.getSiblingDB("test")["output"].drop(), true)