use config

const streamName = 'testStream1'

const agg = [
    // {
    //   $in: {
    //     connector: "kafka",
    //     name: "kafkaUserBehavior",
    //     connectionConfig: {
    //       bootstrapServer: "localhost:9092",
    //       topic: "quickstart",
    //       format: "json", // or text
    //     }
    //   }
    // },
    {$in: {db: "input", coll: "test2"}},
    {$merge: {into: {db: "output", coll: "test2"}}}
];

db.createStream(streamName, agg)

// use config
db.system.streams.find()

// show collections

// db.collection.aggregate(agg)

// db.test.find()
