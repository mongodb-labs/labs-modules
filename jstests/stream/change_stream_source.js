// Manual Insertion tests

// Create Test stream

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

db.createStream("testStream1", agg)

db.getSiblingDB("test")["input"].insert({"_id": "x", "foo": "bar"})

const result = db.getSiblingDB("test")["input"].find({"_id": "x"}).toArray()

assert.eq(result.length, 1);
assert.eq(db.getSiblingDB("test")["input"].drop(), true)
assert.eq(db.getSiblingDB("test")["output"].drop(), true)

// TODO: Drop streams