use output

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
        $out: {
          db: "output",
          coll: "test"
        }
      }
    ]
  }
];

db.collection.aggregate(agg)

db.test.find()
