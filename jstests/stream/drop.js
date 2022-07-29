// Drop stream tests

const agg = [
    {$merge: {into: {db: "test", coll: "test"}}},
];

// Create stream
assert.commandWorked(db.createStream("testStream", agg))

// Drop stream
assert.eq(db.testStream.drop(), true)
assert.eq(db.testStream.drop(), false)
