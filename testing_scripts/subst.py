import pymongo as pm

mongo = pm.MongoClient()
db = mongo.get_database("letp_test")

db.test.drop()
db.test.insert_many([{"k": x, "v": x % 2 == 0} for x in range(20)])

pipeline = [
    {
        "$letp": {
            "vars": {"op": {"$const": "$multiply"}},
            "pipeline": [
                {"$project": {"_id": 0}},
                {
                    "$expand": {
                        "$subst": {
                            "symbols": ["$$op"],
                            "in": [{"$set": {"foobar": {"$$op": ["$k", 2]}}}],
                        }
                    }
                },
            ],
        }
    }
]

for r in db.test.aggregate(pipeline):
    print(f"{r = }")
