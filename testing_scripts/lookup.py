import pymongo as pm

mongo = pm.MongoClient()
db = mongo.get_database("lookup_test")

db.test.drop()
db.test.insert_many([{"k": x, "v": x % 2 == 0} for x in range(20)])

pipeline = [
    {
        "$lookup": {
            "let": {"foo": 1},
            "pipeline": [
                {"$documents": [{"a": 1}]},
                {"$project": {"foo": "$$foo"}},
                {"$unwind": "$foo"},
            ],
            "as": "foobar",
        }
    }
]

for r in db.test.aggregate(pipeline):
    print(f"{r = }")
