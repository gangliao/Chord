## Chord: A Scalable Peer-to-peer Lookup Service for Internet
Applications



## Reference

```bib
@article{10.1145/964723.383071,
author = {Stoica, Ion and Morris, Robert and Karger, David and Kaashoek, M. Frans and Balakrishnan, Hari},
title = {Chord: A Scalable Peer-to-Peer Lookup Service for Internet Applications},
year = {2001},
issue_date = {October 2001},
publisher = {Association for Computing Machinery},
address = {New York, NY, USA},
volume = {31},
number = {4},
issn = {0146-4833},
url = {https://doi.org/10.1145/964723.383071},
doi = {10.1145/964723.383071},
abstract = {A fundamental problem that confronts peer-to-peer applications is to efficiently locate the node that stores a particular data item. This paper presents Chord, a distributed lookup protocol that addresses this problem. Chord provides support for just one operation: given a key, it maps the key onto a node. Data location can be easily implemented on top of Chord by associating a key with each data item, and storing the key/data item pair at the node to which the key maps. Chord adapts efficiently as nodes join and leave the system, and can answer queries even if the system is continuously changing. Results from theoretical analysis, simulations, and experiments show that Chord is scalable, with communication cost and the state maintained by each node scaling logarithmically with the number of Chord nodes.},
journal = {SIGCOMM Comput. Commun. Rev.},
month = aug,
pages = {149–160},
numpages = {12}
}
```
