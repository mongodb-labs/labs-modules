import pymongo as pm

mongo = pm.MongoClient()
db = mongo.get_database("letp_test")

db.test.drop()
db.test.insert_many([{"k": x, "v": x % 2 == 0} for x in range(20)])

pipeline = [
    {
        "$letp": {
            "vars": {"foo": 100, "p": {"$const": [{"$project": {"_id": 0}}]}},
            "pipeline": [{"$addFields": {"p": "$$p"}}, {"$eval": "$$p"}],
        }
    }
]

for r in db.test.aggregate(pipeline):
    print(f"{r = }")
