use config

const streamName = 'streamY'


const agg = [
  {
    $stream: [
      {
        $in: {
          connector: "kafka",
          name: "kafkaUserBehavior",
          connectionConfig: {
            bootstrapServer: "localhost:9092",
            topic: "quickstart",
            format: "json", // or text
          }
        }
      },
      {
        $merge: {
          into: {
            db: "output",
            coll: "test"
          }
        }
      }
    ]
  }
];

db.createStream(streamName, agg)

// use config
db.system.streams.find()

// show collections

// db.collection.aggregate(agg)

// db.test.find()
