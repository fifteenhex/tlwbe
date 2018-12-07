#!/usr/bin/env python3

import argparse
from pycparser import parse_file
from pycparser.c_ast import Typedef, TypeDecl, Struct, Decl, IdentifierType, PtrDecl

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

parametermap = {
    'notnull': 'NOT NULL',
    'primarykey': 'PRIMARY KEY',
    'autoincrement': 'AUTOINCREMENT',
    'unique': 'UNIQUE'
}

typemap = {
    'guint8': 'INTEGER',
    'guint32': 'INTEGER',
    'guint64': 'INTEGER',
    'gsize': 'INTEGER',
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
    return '%s = sqlite3_column_text(stmt, %d)' % (field, pos)


def __fetch_blob(pos, field):
    return '%s = sqlite3_column_blob(stmt, %d); %slen = sqlite3_column_bytes(stmt, %d)' % (field, pos, field, pos)


bindmethodmap = {
    'guint64': __bind_long,
    'guint32': __bind_int,
    'guint8': __bind_int,
    'gdouble': __bind_double
}

bind_pointer_type_map = {
    'gchar': __bind_string,
    'guint8': __bind_blob
}

fetch_type_method_map = {
    'guint64': __fetch_long,
    'guint32': __fetch_long,
    'guint8': __fetch_long,
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
        pos = 1
        for col in self.cols:
            if 'hidden' in col['flags']:
                continue

            path = ""
            if len(col['path']) != 0:
                path = ".".join(col['path']) + "."
            bind = col['fetch_method'](pos, "%s.%s%s" % (self.struct_type, path, col['field_name']))
            outputfile.write(
                '\t%s;\n' % bind)
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


def __stuctbyname(name: (str)):
    print('looking for struct %s' % name)
    for child in ast:
        if type(child) is Decl:
            if type(child.type) is Struct:
                if child.type.name == name:
                    print('found struct %s' % name)
                    return child.type
    return None


def __getconstraintfieldname(constraint_field):
    parts = constraint_field.name[2:].split('_')
    return parts[2]


def __flags_from_field(flags_field):
    if flags_field is None:
        return []

    parts = flags_field.name[2:].split('_')

    flags = []
    for flag in parts[3:]:
        assert flag in flag_types
        flags.append(flag)

    print("flags for %s %s" % (__getconstraintfieldname(flags_field), str(flags)))
    return flags


def __constraints_from_field(constraints_field):
    if constraints_field is None:
        return None

    parts = constraints_field.name[2:].split('_')
    constraints = []

    for part in parts[3:]:
        param = parametermap.get(part)
        assert param is not None
        constraints.append(param)

    print("constraints for %s %s" % (__getconstraintfieldname(constraints_field), str(constraints)))
    return constraints


def __add_col(field: TypeDecl, parsedtable: ParsedTable, flags_field=None, constraints_field=None, prefix=None,
              pointer=False,
              path=None):
    parsed_flags = __flags_from_field(flags_field)
    constraints = __constraints_from_field(constraints_field)
    print("add col %s with constraints %s" % (field.declname, str(constraints)))

    field_type = field.type.names[0]

    sql_mapped_type = None
    if pointer:
        sql_mapped_type = pointertypemap.get(field_type)
    if sql_mapped_type is None:
        sql_mapped_type = typemap.get(field_type)
    assert sql_mapped_type is not None

    if pointer:
        bind_mapped_type = bind_pointer_type_map.get(field_type)
        fetch_method = fetch_pointer_type_method_map.get(field_type)
    else:
        bind_mapped_type = bindmethodmap.get(field_type)
        fetch_method = fetch_type_method_map.get(field_type)

    assert bind_mapped_type is not None, (
            "c type %s(pointer: %s) doesn't have a bind mapping" % (field_type, str(pointer)))

    assert fetch_method is not None, (
            "c type %s(pointer: %s) doesn't have a fetch mapping" % (field_type, str(pointer)))

    field_name = field.declname
    if prefix is not None:
        colname = "%s_%s" % (prefix, field_name)
    else:
        colname = field_name

    flattened_constraints = ""
    if constraints is not None:
        flattened_constraints = " ".join(constraints)

    parsedtable.cols.append(
        {'name': colname, 'field_name': field_name, 'path': path, 'flags': parsed_flags, 'sql_type': sql_mapped_type,
         'sql_constraints': flattened_constraints, 'bind_type': bind_mapped_type, 'fetch_method': fetch_method})


def __flattenfield(field, parsedtable: ParsedTable, path: list, flags_field=None, constraints_field=None, prefix=None,
                   pointer=False):
    # field.show()
    if type(field.type) == TypeDecl:
        if type(field.type.type) == IdentifierType:
            __add_col(field.type, parsedtable, flags_field, constraints_field, prefix, pointer, path)
        elif type(field.type.type) == Struct:
            # field.type.type.show()
            path.append(field.name)
            __flattenstruct(__stuctbyname(field.type.type.name), parsedtable, field.name, path=path)
    elif type(field.type) == PtrDecl:
        __flattenfield(field.type, parsedtable, path, flags_field, constraints_field, prefix, True)
    else:
        print("field type %s not handled" % type(field.type))


def __flattenstruct(struct: Struct, parsedtable: ParsedTable, prefix=None, path=[]):
    annotations_flags = {}
    annotations_constraints = {}
    fields = []

    # bucket the fields and the annotations
    for field in struct:
        if field.name.startswith('__sqlitegen'):
            print("found annotation %s" % field.name)
            annotation_type = field.name[2:].split('_')[1]
            assert annotation_type in annotation_types
            field_name = __getconstraintfieldname(field)
            if annotation_type == 'flags':
                annotations_flags[field_name] = field
            elif annotation_type == 'constraints':
                annotations_constraints[field_name] = field
        else:
            fields.append(field)

    for f in fields:
        __flattenfield(f, parsedtable, path.copy(), annotations_flags.get(f.name), annotations_constraints.get(f.name),
                       prefix)


def __walktable(name: str, struct: (Struct)):
    parsedtable = ParsedTable(name, struct.name)
    __flattenstruct(struct, parsedtable)
    return parsedtable


if __name__ == '__main__':
    args = parser.parse_args()
    print("processing %s -> %s" % (args.input, args.output))

    ast = parse_file(args.input, use_cpp=True,
                     cpp_args=['-D__SQLITEGEN', '-I/usr/share/python3-pycparser/fake_libc_include'])
    # ast.show()

    tables = {}

    # first pass to find all of the tables
    for child in ast.ext:
        if type(child) is Typedef:
            if child.name.startswith('__sqlitegen'):
                nameparts = child.name[2:].split('_')
                if nameparts[1] == 'table':
                    if type(child.type.type) is Struct:
                        tablename = nameparts[2]
                        structname = child.type.type.name
                        print('will generate table %s from %s' % (tablename, structname))
                        if tables.get(structname) is None:
                            tables[structname] = []
                        tables[structname].append(tablename)

    outputs = []

    # second pass to find the structs that were pointed at and
    # generate any tables for them
    for child in ast.ext:
        if type(child) is Decl:
            if type(child.type) is Struct:
                tablenames = tables.get(child.type.name)
                if tablenames is not None:
                    for tablename in tablenames:
                        print('found struct for %s' % tables[child.type.name])
                        outputs.append(__walktable(tablename, child.type))

    outputfile = open(args.output, 'w+')
    outputfile.write("//generated by sqlitegen from %s\n" % args.input)
    for t in outputs:
        t.write(outputfile)
