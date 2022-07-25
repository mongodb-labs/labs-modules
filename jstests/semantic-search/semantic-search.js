
db.getSiblingDB("test").knowledgesource.drop()

db.getSiblingDB("test").knowledgesource.insert({
  _id: '12345678',
  wikipedia_id: '12345678',
  wikipedia_title: 'The Tunnel (novel)',
  text: [
    'The Tunnel (novel)\n',
    "The Tunnel is William H. Gass's 1995 novel that took 26 years to write and earned him the American Book Award of 1996. It was also a finalist for the PEN/Faulkner award.\n",
    `"The Tunnel" is the work of William Frederick Kohler, a professor of history at an unnamed university in the American Midwest. Kohler's introduction to his major work on World War II, "Guilt and Innocence in Hitler's Germany", the culmination of his years studying the aspects of the Nazi regime in the scope of its causes and effects, turns into "The Tunnel", a brutally honest and subjective depiction of his own life and history and the opposite of the well-argued, researched and objective book he has just completed. When the harsh reality of his work begins to dawn on him, he fears that his wife, Martha, will stumble onto his papers and read his most personal (and cruel) descriptions of his and their life. Because of this fear, he hides the pages of "The Tunnel" inside of "Guilt and Innocence in Hitler's German"y. During this time, he starts to dig a tunnel underneath the basement of his home, eventually hiding the dirt inside the drawers of his wife's collection of antique furniture.\n`,
  ],
  anchors: [
    {
      text: 'William H. Gass',
      href: 'William%20H.%20Gass',
      paragraph_id: 1,
      start: 14,
      end: 29,
      wikipedia_title: 'William H. Gass',
      wikipedia_id: '399251'
    },
    {
      text: 'American Book Award',
      href: 'American%20Book%20Award',
      paragraph_id: 1,
      start: 90,
      end: 109,
      wikipedia_title: 'American Book Awards',
      wikipedia_id: '1537452'
    }
  ],
  categories: '1995 American novels',
  history: {
    revid: 908474548,
    timestamp: '2019-07-29T23:39:17Z',
    parentid: 897412603,
    pre_dump: true,
    pageid: 23035338,
    url: 'https://en.wikipedia.org/w/index.php?title=The%20Tunnel%20(novel)&oldid=908474548'
  },
  wikidata_info: {
    aliases: null,
    wikidata_label: 'The Tunnel',
    description: 'book by William H. Gass',
    wikidata_id: 'Q7770719',
    wikipedia_title: 'The Tunnel (novel)'
  }
})

db.adminCommand({
	"registerEndpoint": "_internalSemanticSearch",
	"endpoint": "http://localhost:5001/semantic_search",
	"method": "POST",
	"httpHeaders": [],
	"as": "response",
	"requestEncoding": "JSON",
	"responseEncoding": "JSON"
})

// const pipeline = [
//   { $semanticSearch: "testing"}
// ];

const pipeline = [
  {$limit: 1},
  {$project: {
    _id: 0,
    query: {$literal: "what powers submarines?"},
    num_results: 3,
  }},
  {$external: {name: "_internalSemanticSearch"}},
  {$unwind: "$response.matches"},

  {$group: {     // <--- this $group stage is new
    _id: "$response.matches.docid",
    matches: {$push: "$response.matches"}
  }},
  {$unset: ["matches.docid", "matches.title"]},  // <--- this $unset stage is new
  {$lookup: {
     from: "knowledgesource",
     localField: "_id",         //  <--- This changed because of $group, now just "_id"
     foreignField: "_id",
     as: "docs"
  }},
  {$unwind: "$docs"},
  {$set: {"docs.__sem_search__": "$matches"}},   //  <--- this changed, now just "$matches"
  {$replaceRoot: {newRoot: "$docs"}},
]

db.getSiblingDB("test").knowledgesource.aggregate(pipeline);