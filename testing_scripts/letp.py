import pymongo as pm

mongo = pm.MongoClient()
db = mongo.get_database("letp_test")

db.test.drop()
db.test.insert_many([{"k": x, "v": x % 2 == 0} for x in range(20)])

pipeline = [
    {
        "$letp": {
            "vars": {"foo": 100},
            "pipeline": [
                {"$project": {"_id": 0}},
                {"$addFields": {"foo": "$$foo"}},
                {"$addFields": {"foobar": {"$add": ["$$foo", "$k"]}}},
            ],
        }
    }
]

for r in db.test.aggregate(pipeline):
    print(f"{r = }")
