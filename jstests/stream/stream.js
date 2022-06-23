use output

db.streamA.drop();

db.test.drop();

const agg = [
  {
    $stream: [
      {
        $in: {
          connector: "kafka",
          name: "kafkaUserBehavior",
          connectionConfig: {
            booststrapServer: "localhost:9092",
            topic: "json-quickstart",
            format: "json", // or text
          }
        }
      },
      {
        $simpTWindow: 5000
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

db.createStream("streamA", agg)

// db.system.views.find()

// show collections

// db.collection.aggregate(agg)

// db.test.find()
