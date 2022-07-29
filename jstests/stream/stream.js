const agg = [
  {
    $stream: [
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
      {
        $in: {
          db: "input",
          coll: "test2"
        }
      },
      {
        $merge: {
          into: {
            db: "output",
            coll: "test2"
          }
        }
      }
    ]
  }
];

assert.commandWorked(db.createStream("testStream1", agg))
assert.eq(db.testStream1.drop(), true)