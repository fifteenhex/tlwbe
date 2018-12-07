# sqlitegen - a horrible abuse of your sanity

This is a code generator that abuses the C pre-processor and pycparser to allow *annotations*
to be added to structs and have SQL and most of the common boilerplate code for sqlite generated
for you.

## Basics

Because pycparser doesn't preserve comments (cpp rips them out, pycparser doesn't know what they are)
everything needs to be encoded as valid C that would in-theory compile. To avoid actually having to
compile the meta data you should wrap everything that is only used by sqlitegen in ```#ifdef __SQLITEGEN```.

To avoid upsetting pycparser too much it's also a good idea to keep as little as possible in the files
being processed (i.e. keep this stuff in special headers) and/or wrap stuff that sqlitegen doesn't care about
in ```#ifndef __SQLITEGEN``` so that it's removed before being parsed.

## Creating a table

A table is defined by creating a typedef to a struct with a special name as demostrated below.

```
struct <table> {
	...
}

#ifdef __SQLITEGEN
	typedef struct <table> __sqlitegen_table_<tablename>;
#endif
```

### Annotations

C doesn't have real annotations (GCC has attributes but pycparser doesn't parse those) so these need to
be encoded in some way. The easiest way seemed to be to use normal fields and encoding the parameters into
the field name. It looks fugly but it just about works.

Annotations should be fields in the struct that follow the pattern below.

```
void __sqlitegen_<annotation type>_<parameter 0>_<parameter 1>_<parameter n>
```

#### Adding SQL constraints

Constaints for columns are defined by adding constaints annotations.

```
struct <table> {
	const gchar* <column>;
#ifdef __SQLITEGEN
	void __sqlitegen_constraints_<column>_notnull_primarykey_unique;
#endif
};
```