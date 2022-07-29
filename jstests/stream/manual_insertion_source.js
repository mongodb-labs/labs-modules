// Manual insertion tests

const agg = [
    {$merge: {into: {db: "test", coll: "manual-insertion"}}},
];

assert.commandWorked(db.createStream("manualInsertionStream", agg))

assert.commandWorked(db.manualInsertionStream.insert({"foo": "bar"}))

let count = 0
let i = 0

// Need to poll output collection
while (i < 5) {
    sleep(500)
    count = db.getSiblingDB("test")["manual-insertion"].count()

    if (count > 0) {
        break
    }

    i++
}

assert.eq(count, 1);
assert.eq(db.manualInsertionStream.drop(), true)
assert.eq(db.getSiblingDB("test")["manual-insertion"].drop(), true)
