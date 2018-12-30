# jsongen - another horrible abuse of your sanity

This is a code generator that abuses the C pre-processor and pycparser to allow *annotations*
to be added to structs to generate most of the common boilerplate code to parse and build json.

##basics

See [sqlitegen](sqlitegen.md)

##generating a parser or a generator

The generation of a parser or builder is triggered by creating a typedef to a struct with a
special name as demonstrated below. It's possible to have a parser without a generator and
vice-versa.

```
struct <json object> {
	...
}

#ifdef __JSONGEN
	typedef struct <json object> __jsongen_parser;
	typedef struct <json object> __jsongen_builder;
#endif
```