import os


Import("env has_option")
Import("get_option")
Import("http_client")

feature_dirs = [
    "external"
]

env = env.Clone()

env.InjectMongoIncludePaths()
env.InjectModule("labs")

# Code in the labs module follows the basic.h practices of the community repo.
env.AppendUnique(
    FORCEINCLUDES=[
        'mongo/platform/basic.h',
    ]
)


env.SConscript(
    dirs=[
        "src/%s" % feature
        for feature in env['MONGO_LABS_FEATURES']
    ],
    exports=[
        'env',
    ],
)