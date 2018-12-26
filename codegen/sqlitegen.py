#!/usr/bin/env python3

import argparse
import codegen
from pycparser.c_ast import Typedef, TypeDecl, Struct, Decl, IdentifierType, PtrDecl

TAG = 'sqlite'

parser = argparse.ArgumentParser(description='sqlite code gen')
parser.add_argument('--input', type=str, required=True)
parser.add_argument('--output', type=str, required=True)

annotation_types = [
    'flags',
    'constraints'
]

flag_types = [
    'hidden',
    'searchable'
]

constraint_map = {
    'notnull': 'NOT NULL',
    'primarykey': 'PRIMARY KEY',
    'autoincrement': 'AUTOINCREMENT',
    'unique': 'UNIQUE'
}

typemap = {
    'guint8': 'INTEGER',
    'guint16': 'INTEGER',
    'guint32': 'INTEGER',
    'guint64': 'INTEGER',
    'gint8': 'INTEGER',
    'gint16': 'INTEGER',
    'gint32': 'INTEGER',
    'gint64': 'INTEGER',
    'gsize': 'INTEGER',
    'gboolean': 'INTEGER',
    'gdouble': 'REAL',
    'gchar': 'TEXT'
}

pointertypemap = {
    'guint8': 'BLOB'
}


def __bind_long(pos, field):
    return 'sqlite3_bind_int64(stmt, %d, %s)' % (pos, field)


def __bind_int(pos, field):
    return 'sqlite3_bind_int(stmt, %d, %s)' % (pos, field)


def __bind_double(pos, field):
    return 'sqlite3_bind_double(stmt, %d, %s)' % (pos, field)


def __bind_string(pos, field):
    return 'sqlite3_bind_text(stmt, %d, %s, -1, SQLITE_STATIC)' % (pos, field)


def __bind_blob(pos, field):
    return 'sqlite3_bind_blob(stmt, %d, %s, %slen, NULL)' % (pos, field, field)


def __fetch_long(pos, field):
    return '%s = sqlite3_column_int(stmt, %d)' % (field, pos)


def __fetch_double(pos, field):
    return '%s = sqlite3_column_double(stmt, %d)' % (field, pos)


def __fetch_string(pos, field):
    return '%s = (gchar*) sqlite3_column_text(stmt, %d)' % (field, pos)


def __fetch_blob(pos, field):
    return '%s = sqlite3_column_blob(stmt, %d); %slen = sqlite3_column_bytes(stmt, %d)' % (field, pos, field, pos)


bindmethodmap = {
    'guint64': __bind_long,
    'guint32': __bind_int,
    'guint16': __bind_int,
    'guint8': __bind_int,
    'gint64': __bind_long,
    'gint32': __bind_int,
    'gint16': __bind_int,
    'gint8': __bind_int,
    'gboolean': __bind_int,
    'gdouble': __bind_double
}

bind_pointer_type_map = {
    'gchar': __bind_string,
    'guint8': __bind_blob
}

fetch_type_method_map = {
    'guint64': __fetch_long,
    'guint32': __fetch_long,
    'guint16': __fetch_long,
    'guint8': __fetch_long,
    'gint64': __fetch_long,
    'gint32': __fetch_long,
    'gint16': __fetch_long,
    'gint8': __fetch_long,
    'gboolean': __fetch_long,
    'gdouble': __fetch_double
}

fetch_pointer_type_method_map = {
    'gchar': __fetch_string,
    'guint8': __fetch_blob
}


class ParsedTable:

    def __init__(self, name: str, struct_type: str):
        self.name = name
        self.struct_type = struct_type
        self.cols = []

    def __find_searchable_cols(self):
        searchablecols = []
        for col in self.cols:
            if 'searchable' in col['flags']:
                searchablecols.append(col)
        return searchablecols

    def __write_sql_create(self, outputfile):
        outputfile.write(
            '#define __SQLITEGEN_%s_TABLE_CREATE "CREATE TABLE IF NOT EXISTS %s ("\\\n' % (
                self.name.upper(), self.name))

        createbody = []
        for col in self.cols:
            createbody.append('\t\t\t"%s %s %s' % (col['name'], col['sql_type'], col['sql_constraints']))
        outputfile.write(',"\\\n'.join(createbody))
        outputfile.write('"\\\n')

        outputfile.write('\t\t");"\n\n')

    def __write_sql_insert(self, outputfile):
        colnames = []
        something = []
        for col in self.cols:
            if 'hidden' in col['flags']:
                continue
            colnames.append(col['name'])
            something.append("?")
        outputfile.write(
            '#define __SQLITEGEN_%s_INSERT "INSERT INTO %s (%s) VALUES (%s);"\n\n' % (
                self.name.upper(), self.name, ",".join(colnames), ",".join(something)))

    def __write_sql_getby(self, outputfile):
        cols = self.__find_searchable_cols()
        for col in cols:
            outputfile.write(
                '#define __SQLITEGEN_%s_GETBY_%s "SELECT * FROM %s WHERE %s = ?;"\n\n' % (
                    self.name.upper(), col['name'].upper(), self.name, col['name']))

    def __write_sql_list(self, outputfile):
        cols = self.__find_searchable_cols()
        for col in cols:
            outputfile.write(
                '#define __SQLITEGEN_%s_LIST_%s "SELECT %s FROM %s;"\n\n' % (
                    self.name.upper(), col['name'].upper(), col['name'], self.name))

    def __write_sql_deleteby(self, outputfile):
        cols = self.__find_searchable_cols()
        for col in cols:
            outputfile.write(
                '#define __SQLITEGEN_%s_DELETEBY_%s "DELETE FROM %s WHERE %s = ?;"\n\n' % (
                    self.name.upper(), col['name'].upper(), self.name, col['name']))

    def __write_c_rowcallback(self, outputfile):

        callbackbackstructname = '__sqlitegen_%s_rowcallback_callback' % self.name

        outputfile.write('struct %s {\n' % callbackbackstructname)
        outputfile.write('\t void (*callback)(const struct %s*, void*);\n' % self.struct_type)
        outputfile.write('\t void* data;\n')
        outputfile.write('};\n\n')

        outputfile.write(
            'static void __attribute__((unused)) __sqlitegen_%s_rowcallback(sqlite3_stmt* stmt, struct %s* callback){\n' % (
                self.name, callbackbackstructname))
        outputfile.write('\tstruct %s %s = {0};\n' % (self.struct_type, self.struct_type))
        pos = 0
        for col in self.cols:
            if 'hidden' in col['flags']:
                continue
            path = ""
            if len(col['path']) != 0:
                path = ".".join(col['path']) + "."
            bind = col['fetch_method'](pos, "%s.%s%s" % (self.struct_type, path, col['field_name']))
            outputfile.write('\t%s;\n' % bind)
            pos += 1

        outputfile.write('\tcallback->callback(&%s, callback->data);\n' % self.struct_type)
        outputfile.write('}\n\n')

    def __write_c_add(self, outputfile):
        outputfile.write(
            'static void __attribute__((unused)) __sqlitegen_%s_add(sqlite3_stmt* stmt, const struct %s* %s){\n' % (
                self.name, self.struct_type, self.struct_type))
        bindpos = 1
        for col in self.cols:
            if 'hidden' in col['flags']:
                continue

            path = ""
            if len(col['path']) != 0:
                path = ".".join(col['path']) + "."
            bind = col['bind_type'](bindpos, "%s->%s%s" % (self.struct_type, path, col['field_name']))
            outputfile.write(
                '\t%s;\n' % bind)
            bindpos += 1
        outputfile.write('}\n')

    def write(self, outputfile):
        self.__write_sql_create(outputfile)
        self.__write_sql_insert(outputfile)
        self.__write_sql_getby(outputfile)
        self.__write_sql_list(outputfile)
        self.__write_sql_deleteby(outputfile)
        self.__write_c_rowcallback(outputfile)
        self.__write_c_add(outputfile)


def __flags_from_field(flags_annotation: codegen.FieldAnnotation):
    if flags_annotation is None:
        return []

    flags = []
    for flag in flags_annotation.parameters:
        assert flag in flag_types
        flags.append(flag)

    print("flags for %s %s" % (flags_annotation.field_name, str(flags)))
    return flags


def __constraints_from_field(constraints_annotation: codegen.FieldAnnotation):
    if constraints_annotation is None:
        return None

    sql_constraints = []
    for constraint in constraints_annotation.parameters:
        sql_constraint = constraint_map.get(constraint)
        assert sql_constraint is not None
        sql_constraints.append(sql_constraint)

    print("constraints for %s %s" % (constraints_annotation.field_name, str(sql_constraints)))
    return sql_constraints


def __add_col(field: codegen.Field, parsedtable: ParsedTable, flags_annotation=None, constraints_annotation=None,
              prefix=None, pointer=False, path=None):
    parsed_flags = __flags_from_field(flags_annotation)
    constraints = __constraints_from_field(constraints_annotation)
    print("add col %s with constraints %s" % (field.field_name, str(constraints)))

    sql_mapped_type = None
    if pointer:
        sql_mapped_type = pointertypemap.get(field.c_type)
    if sql_mapped_type is None:
        sql_mapped_type = typemap.get(field.c_type)
    assert sql_mapped_type is not None

    if pointer:
        bind_mapped_type = bind_pointer_type_map.get(field.c_type)
        fetch_method = fetch_pointer_type_method_map.get(field.c_type)
    else:
        bind_mapped_type = bindmethodmap.get(field.c_type)
        fetch_method = fetch_type_method_map.get(field.c_type)

    assert bind_mapped_type is not None, (
            "c type %s(pointer: %s) doesn't have a bind mapping" % (field.c_type, str(pointer)))

    assert fetch_method is not None, (
            "c type %s(pointer: %s) doesn't have a fetch mapping" % (field.c_type, str(pointer)))

    if prefix is not None:
        colname = "%s_%s" % (prefix, field.field_name)
    else:
        colname = field.field_name

    flattened_constraints = ""
    if constraints is not None:
        flattened_constraints = " ".join(constraints)

    parsedtable.cols.append(
        {'name': colname, 'field_name': field.field_name, 'path': path, 'flags': parsed_flags,
         'sql_type': sql_mapped_type, 'sql_constraints': flattened_constraints, 'bind_type': bind_mapped_type,
         'fetch_method': fetch_method})


def __flattenfield(ast, field: codegen.Field, parsedtable: ParsedTable, path: list, flags_annotation,
                   constraints_annotation,
                   prefix=None, pointer=False):
    print(field.type)
    if field.type == codegen.FieldType.NORMAL or field.type == codegen.FieldType.POINTER:
        __add_col(field, parsedtable, flags_annotation, constraints_annotation, prefix,
                  field.type == codegen.FieldType.POINTER, path)
    elif field.type == codegen.FieldType.STRUCT:
        pass
        # field.type.type.show()
        path.append(field.field_name)
        __flatten_struct(ast, parsedtable, codegen.struct_by_name(ast, field.field.type.type.name),
                         prefix=field.field_name,
                         path=path)
    else:
        assert False, ('unhandled type %s' % field.type)


def __flatten_struct(ast, parsedtable: ParsedTable, struct: Struct, prefix=None, path=[]):
    fieldsandannotations = codegen.walk_struct(ast, TAG, struct, annotation_types=annotation_types)

    # bucket the annotations
    annotations = {'flags': {}, 'constraints': {}}
    for annotation in fieldsandannotations[1]:
        annotations[annotation.annotation_type][annotation.field_name] = annotation

    for f in fieldsandannotations[0]:
        if f.field_name in annotations['flags']:
            flags = annotations['flags'].pop(f.field_name)
        else:
            flags = None
        if f.field_name in annotations['constraints']:
            constraints = annotations['constraints'].pop(f.field_name)
        else:
            constraints = None

        __flattenfield(ast, f, parsedtable, path.copy(), flags, constraints, prefix)

    # check that we don't have any left overs
    orphans = 0
    for annotation_type in annotations:
        orphans += len(annotations[annotation_type])
    assert orphans == 0


def __walktable(ast, struct: Struct, tables, outputs: list):
    table_names = tables.get(struct.name)
    if table_names is not None:
        print('found struct for %s' % tables[struct.name])
        for table_name in table_names:
            parsedtable = ParsedTable(table_name, struct.name)
            __flatten_struct(ast, parsedtable, struct)
            outputs.append(parsedtable)


def __table_name_from_struct_annotation(tables: dict, annotated_struct: codegen.AnnotatedStruct):
    if annotated_struct.annotation_type == 'table':
        assert len(annotated_struct.parameters) == 1
        table_name = annotated_struct.parameters[0]
        print('will generate table %s from %s' % (table_name, annotated_struct.struct_name))
        if tables.get(annotated_struct.struct_name) is None:
            tables[annotated_struct.struct_name] = []
        tables[annotated_struct.struct_name].append(table_name)


if __name__ == '__main__':
    args = parser.parse_args()
    print("sqlitegen processing %s -> %s" % (args.input, args.output))

    ast = codegen.parsefile('sqlitegen', args.input)
    structs = codegen.find_annotated_structs(TAG, ['table'], ast)

    tables = {}
    for struct in structs:
        __table_name_from_struct_annotation(tables, struct)

    outputs = codegen.find_structs(ast, __walktable, tables)

    outputfile = open(args.output, 'w+')
    outputfile.write("//generated by sqlitegen from %s\n" % args.input)
    for t in outputs:
        t.write(outputfile)

# if type(child.type.type) is Struct:
